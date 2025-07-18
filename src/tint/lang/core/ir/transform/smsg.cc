#include "src/tint/lang/core/ir/transform/smsg.h"

#include "src/tint/lang/core/ir/disassembler.h"
#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/traverse.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/utils/ice/ice.h"

#include <iostream>
#include <chrono>

using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::core::ir::transform {

  // TODO:
  // right now we end up doing unnecessary analysis on code added by the pass. Consider tracking added functions/statements
  // and avoid analyzing them.
  // optimization: dynamically determine if buffer is read-only
  // optimization: smarter handling of loops

  namespace {

    /// PIMPL state for the transform.
    struct State {

      /// The SMSG config.
      const SMSGConfig& config;

      /// The IR module.
      Module& ir;

      /// The IR builder.
      Builder b{ ir };

      /// The type manager.
      type::Manager& ty{ ir.Types() };

      /// The symbol table.
      SymbolTable& sym{ ir.symbols };

      /// Map from structs with non-atomic members to structs with atomic members
      Hashmap<const type::Struct*, const type::Struct*, 4> struct_map{};

      /// Map from a struct to a helper function that will either convert a rewritten type 
      /// to an original type (for loads) or store an original type to a rewritten type.
      Hashmap<const type::Struct*, Function*, 4> convert_helpers{};

      // The set of visited control instructions. If a control has been visited, that means it already
      // is in the slice of some access chain, and we should not visit it again.
      Hashset<const ControlInstruction*, 4> visited_ctrls{};

      // The set of visited values. If a value has been visited, that means it already
      // is in the slice of some access chain, and we should not visit it again.
      Hashset<const Value*, 4> visited_values{};

      // Maps functions to whether or not they store to some pointer, so that control dependencies of calls to these
      // functions can be analyzed. There is currently no other way to match stores in void functions
      // to where the function is called from.
      Hashmap<Function*, bool, 4> void_function_stores{};

      // To avoid infinite recursion if a variable is stored back to itself, we maintain a set of variables which
      // we have already visited.
      Hashset<const Var*, 4> visited_vars{};

      // The entry point if this is a compute shader
      std::string entry_point = "";

      // The number of storage bufer rewrites needed. Note that rewriting an array counts as one rewrite
      uint storage_rewrites = 0;

      // The number of workgroup buffer rewrites needed.
      uint workgroup_rewrites = 0;

      // The number of loads converted. Note these are _static_ loads, so loads in a loop will end up causing more runtime loads.
      uint atomic_loads = 0;

      // The number of stores converted.
      uint atomic_stores = 0;

      // The number of f32 types we would convert, if we could
      uint f32_rewrites = 0;

      // The number of f32 types we would replace, if we could
      uint f32_replacements = 0;

      // Replace type used in instruction result 
      const type::Type* ReplaceType(const type::Type* type) {
        return tint::Switch(type,
          [&](const type::Pointer* ptr) {
            return ty.ptr(ptr->AddressSpace(), ReplaceType(ptr->UnwrapPtr()), ptr->Access());
          },
          [&](const type::Array* ar) {
            auto newElemType = ReplaceType(ar->ElemType());
            auto typeAndCount = ar->Count();
            return tint::Switch(typeAndCount,
              [&](const type::ConstantArrayCount* count) {
                return ty.array(newElemType, count->value, ar->Stride());
              },
              [&](const type::RuntimeArrayCount*) {
                return ty.runtime_array(newElemType, ar->Stride());
              },
              TINT_ICE_ON_NO_MATCH);
          },
          [&](const type::Struct* st) {
            return *struct_map.Get(st).value;
          },
          [&](const type::I32* i32) {
            // wrap the i32 in an atomic type
            return ty.Get<type::Atomic>(i32);
          },
          [&](const type::U32* u32) {
            // wrap the u32 in an atomic type
            return ty.Get<type::Atomic>(u32);
          },
          [&](const type::F32* f32) {
            // we'd wrap the type if we could :(
            f32_replacements++;
            return f32;
          },
          [&](const type::Atomic* at) {
            // If the type is already atomic just return it
            return at;
          },
          TINT_ICE_ON_NO_MATCH
        );
      }

      // Converts load instruction to load the target type, given an input value
      Value* ConvertLoad(Value* source, const type::Type* targetType) {
        auto* sourcePtr = source->Type()->As<type::Pointer>();
        TINT_ASSERT(sourcePtr);
        // types match, can load directly
        if (targetType == sourcePtr->StoreType()) {
          return b.Load(source)->Result(0);
        }
        return tint::Switch(targetType,
          // type is scalar, load atomically
          [&](const type::NumericScalar* ns) {
            atomic_loads++;
            return b.Call(ns, BuiltinFn::kAtomicLoad, source)->Result(0);
          },
          // call a helper function which takes in a pointer to the source type and returns the target type
          [&](const type::Struct* st) {
            auto* helper = convert_helpers.GetOrAdd(st, [&] {
              auto* sourceStruct = sourcePtr->StoreType()->As<type::Struct>();
              TINT_ASSERT(sourceStruct);
              auto* func = b.Function("tint_convert_" + sourceStruct->Name().Name() + "_" + st->Name().Name(), st);
              auto* input = b.FunctionParam("tint_input", sourcePtr);
              func->SetParams({ input });
              b.Append(func->Block(), [&] {
                uint32_t index = 0;
                Vector<Value*, 4> args;
                for (auto* member : st->Members()) {
                  auto* accessType = ty.ptr(sourcePtr->AddressSpace(), sourceStruct->Element(index), sourcePtr->Access());
                  auto* extractRes = b.Access(accessType, input, u32(index))->Result(0);
                  args.Push(ConvertLoad(extractRes, member->Type()));
                  index++;
                }
                b.Return(func, b.Construct(st, std::move(args)));
                });
              return func;
              });
            return b.Call(helper, source)->Result(0);
          },
          // loop through the pointer to the source type and convert each element to the target type
          [&](const type::Array* arr) {
            auto* fromType = ty.ptr(sourcePtr->AddressSpace(), sourcePtr->StoreType()->Elements().type, sourcePtr->Access());
            auto* newArray = b.Var(ty.ptr(fluent_types::function, arr));
            b.LoopRange(ty, 0_u, u32(arr->ConstantCount().value()), 1_u, [&](Value* idx) {
              // Convert arr[idx] and store to newArray[idx];
              auto* from = b.Access(fromType, source, idx)->Result(0);
              auto* to = b.Access(ty.ptr(fluent_types::function, arr->ElemType()), newArray, idx);
              b.Store(to, ConvertLoad(from, arr->ElemType()));
              });
            return b.Load(newArray)->Result(0);
          },
          TINT_ICE_ON_NO_MATCH
        );
      }

      // Converts store instruction to store to the target, given a source value
      void ConvertStore(Value* from, Value* to) {
        auto* toPtr = to->Type()->As<type::Pointer>();
        TINT_ASSERT(toPtr);
        auto* toStoreType = toPtr->StoreType();
        // types match, can store directly
        if (toStoreType == from->Type()) {
          b.Store(to, from);
        }
        else {
          tint::Switch(toStoreType,
            // store type is atomic, store the value atomically
            [&](const type::Atomic*) {
              atomic_stores++;
              b.Call(ty.void_(), BuiltinFn::kAtomicStore, to, from);
            },
            // call a helper which takes in a pointer to the store type and the value to store and stores it
            [&](const type::Struct* st) {
              auto* helper = convert_helpers.GetOrAdd(st, [&] {
                auto* fromStruct = from->Type()->As<type::Struct>();
                TINT_ASSERT(fromStruct);
                auto* func = b.Function("tint_convert_" + fromStruct->Name().Name() + "_" + st->Name().Name(), ty.void_());
                auto* fromInput = b.FunctionParam("tint_from", from->Type());
                auto* toInput = b.FunctionParam("tint_to", to->Type());
                func->SetParams({ fromInput, toInput });
                b.Append(func->Block(), [&] {
                  uint32_t index = 0;
                  auto* fromStructVarRes = b.Var("tint_from_ptr", fromInput)->Result(0);
                  auto* fromStructPtr = fromStructVarRes->Type()->As<type::Pointer>();
                  TINT_ASSERT(fromStructPtr);

                  for (auto* member : st->Members()) {
                    auto* toAccessType = ty.ptr(toPtr->AddressSpace(), member->Type(), toPtr->Access());
                    auto* toAccessRes = b.Access(toAccessType, toInput, u32(index))->Result(0);

                    auto* fromAccessType = ty.ptr(fromStructPtr->AddressSpace(), fromStruct->Element(index), fromStructPtr->Access());
                    auto* fromAccessRes = b.Access(fromAccessType, fromStructVarRes, u32(index))->Result(0);
                    auto* fromLoadRes = b.Load(fromAccessRes)->Result(0);
                    ConvertStore(fromLoadRes, toAccessRes);
                    index++;
                  }
                  b.Return(func);
                  });
                return func;
                });
              b.Call(helper, from, to);
            },
            // loop through each element in the array to store it at that offset in the target
            [&](const type::Array* arr) {
              auto* fromArr = from->Type()->As<type::Array>();
              TINT_ASSERT(fromArr);
              auto* fromArrVarRes = b.Var("tint_from_arr_ptr", from)->Result(0);
              auto* fromArrPtr = fromArrVarRes->Type()->As<type::Pointer>();
              auto* fromAccessType = ty.ptr(fromArrPtr->AddressSpace(), fromArr->ElemType(), fromArrPtr->Access());
              auto* toAccessType = ty.ptr(toPtr->AddressSpace(), arr->ElemType(), toPtr->Access());
              b.LoopRange(ty, 0_u, u32(arr->ConstantCount().value()), 1_u, [&](Value* idx) {
                auto* fromAccessRes = b.Access(fromAccessType, fromArrVarRes, idx)->Result(0);
                auto* loadRes = b.Load(fromAccessRes)->Result(0);
                auto* toAccessRes = b.Access(toAccessType, to, idx)->Result(0);
                ConvertStore(loadRes, toAccessRes);
                });
            },
            TINT_ICE_ON_NO_MATCH
          );
        }
      }

      // Handle usages of an instruction result given new type
      void Replace(Value* res) {
        res->ForEachUseUnsorted([&](Usage use) {
          auto* inst = use.instruction;
          tint::Switch(inst,
            [&](Access* access) {
              auto* newType = ReplaceType(access->Result(0)->Type());
              auto* innerRes = access->Result(0);
              innerRes->SetType(newType);
              Replace(innerRes);
            },
            [&](Load* load) {
              b.InsertBefore(load, [&] {
                auto* converted = ConvertLoad(load->From(), load->Result(0)->Type());
                load->Result(0)->ReplaceAllUsesWith(converted);
                });
              load->Destroy();
            },
            [&](Store* store) {
              b.InsertBefore(store, [&] {
                ConvertStore(store->From(), store->To());
                });
              store->Destroy();
            },
            [&](Let* let) {
              // let instructions pass the type through
              auto* innerRes = let->Result(0);
              innerRes->SetType(res->Type());
              Replace(innerRes);
            },
            [&](UserCall* uc) {
              // recurse into a user call and replace the type of this value
              const VectorRef<FunctionParam*> fnParams = uc->Target()->Params();
              size_t i = 0;
              for (auto* a : uc->Args()) {
                if (a == res) {
                  auto* param = fnParams[i];
                  param->SetType(res->Type());
                  Replace(param);
                }
                i++;
              }
            }
            // other types of usages we don't need to replace
          );
          });
      }

      // Rewrite type used by shader. Unhandled types raise an internal compiler error
      const type::Type* RewriteType(const type::Type* type, std::vector<Value*> indexStack) {
        return tint::Switch(type,
          [&](const type::Atomic*) {
            // atomic types are already in the correct format
            // TODO: is this necessary due to short-circuit in VisitIDD?
            return type;
          },
          // If we have reached an i32/u32 we need to wrap it in an atomic type
          [&](const type::I32* i32) {
            // wrap the i32 in an atomic type
            return ty.Get<type::Atomic>(i32);
          },
          [&](const type::U32* u32) {
            // wrap the u32 in an atomic type
            return ty.Get<type::Atomic>(u32);
          },
          [&](const type::F32* f32) {
            // we'd wrap this if we could :(
            f32_rewrites++;
            return f32;
          },
          [&](const type::Array* ar) {
            // the index of the array access may be unknown, but it's not needed as all array members have the same type
            //std::cout << "Rewriting array type: " << ar->FriendlyName() << std::endl;
            if (!indexStack.empty()) {
              indexStack.pop_back();
            }
            auto newElemType = RewriteType(ar->ElemType(), indexStack);
            auto typeAndCount = ar->Count();
            return tint::Switch(typeAndCount,
              [&](const type::ConstantArrayCount* count) {
                return ty.array(newElemType, count->value, ar->Stride());
              },
              [&](const type::RuntimeArrayCount*) {
                return ty.runtime_array(newElemType, ar->Stride());
              },
              TINT_ICE_ON_NO_MATCH);
          },
          [&](const type::Struct* st) {
            // If we've already rewritten this type of struct we can use the existing one.
            // TODO: what if the struct is used in different ways by different entry points?
            if (!struct_map.Contains(st)) {
              //std::cout << "Rewriting struct type: " << st->FriendlyName() << std::endl;
              if (indexStack.empty()) {
                // Assume we have to rewrite the entire struct
                auto members = st->Members();
                // the size here doesn't matter, the vector will be resized to handle the correct length
                Vector<const type::StructMember*, 4> newMembers(members);
                for (auto* m : members) {
                  // recursively rewrite the type of each member
                  auto* newMemberType = RewriteType(m->Type(), indexStack);
                  auto* newMember = ty.Get<type::StructMember>(m->Name(), newMemberType, m->Index(), m->Offset(), m->Align(), m->Size(), m->Attributes());
                  // update the new members with the new member 
                  newMembers[m->Index()] = newMember;
                }
                // create a new struct with the new members
                auto* _newStruct = ty.Struct(sym.New(st->Name().Name()), newMembers);
                for (auto flag : st->StructFlags()) {
                  _newStruct->SetStructFlag(flag);
                }
                // add the mapping from the initial struct to the new struct
                struct_map.Add(st, _newStruct);
              }
              else {
                auto* idx = indexStack.back();
                indexStack.pop_back();
                // struct accesses should always be constants
                tint::Switch(idx,
                  [&](Constant* c) {
                    auto members = st->Members();
                    // the size here doesn't matter, the vector will be resized to handle the correct length
                    Vector<const type::StructMember*, 4> newMembers(members);
                    // get the old member and recursively rewrite it
                    auto newMemberIdx = c->Value()->ValueAs<uint32_t>();
                    auto* oldMember = members[newMemberIdx];
                    auto* newMemberType = RewriteType(oldMember->Type(), indexStack);
                    auto* newMember = ty.Get<type::StructMember>(oldMember->Name(), newMemberType, oldMember->Index(), oldMember->Offset(), oldMember->Align(), oldMember->Size(), oldMember->Attributes());
                    // update the new members with the new member 
                    newMembers[newMemberIdx] = newMember;
                    // create a new struct with the new members
                    auto* _newStruct = ty.Struct(sym.New(st->Name().Name()), newMembers);
                    for (auto flag : st->StructFlags()) {
                      _newStruct->SetStructFlag(flag);
                    }
                    // add the mapping from the initial struct to the new struct
                    struct_map.Add(st, _newStruct);
                  },
                  TINT_ICE_ON_NO_MATCH
                );
              }
            }
            return *struct_map.Get(st).value;
          },
          TINT_ICE_ON_NO_MATCH
        );
      }

      // Rewrite the type of the binding point and then replace usages of the binding point using the updated type
      void RewriteRootVar(Var* bp, std::vector<Value*> indexStack) {
        auto* oldPtr = bp->Result(0)->Type()->As<type::Pointer>();
        TINT_ASSERT(oldPtr);
        //std::cout << "Rewriting binding point: " << ir.NameOf(bp).Name() << " with type: " << oldPtr->FriendlyName() << std::endl;
        auto* newPtr = ty.ptr(oldPtr->AddressSpace(), RewriteType(oldPtr->UnwrapPtr(), indexStack), oldPtr->Access());
        bp->Result(0)->SetType(newPtr);
        Replace(bp->Result(0));
      }

      // Checks whether a variable type needs to be rewritten, either because it is a read/write binding point and rewriting
      // storage buffers is enabled or because it is a root block workgroup memory variable.
      bool NeedsRewrite(Var* var) {
        auto* ptr = var->Result(0)->Type()->As<type::Pointer>();
        if (var->BindingPoint() && ptr->Access() == core::Access::kReadWrite && config.rewrite_storage) {
          storage_rewrites++;
          return true;
        }
        if (var->Block() == ir.root_block && ptr && ptr->AddressSpace() == AddressSpace::kWorkgroup) {
          workgroup_rewrites++;
          return true;
        }
        return false;
      }

      // Follow a variable forwards, searching for stores (data dependencies) in this function
      // or in function calls recursively.
      void VisitForwardReference(Value* value, std::vector<Value*> indexStack, std::vector<tint::Slice<Value* const>> fnArgsStack) {
        value->ForEachUseUnsorted([&](Usage use) {
          tint::Switch(use.instruction,
            [&](Store* s) {
              VisitCD(s, fnArgsStack);
              std::vector<Value*> storeIndexStack;
              VisitSliceValue(s->From(), storeIndexStack, fnArgsStack);
            },
            [&](StoreVectorElement* sve) {
              // Check if this stores to the same vector element that we were slicing back, or any element if the vector is loaded
              // entirely
              if (indexStack.empty() || sve->Index() == indexStack.back()) {
                std::vector<Value*> newIndexStack;
                VisitSliceValue(sve->Value(), newIndexStack, fnArgsStack);
                VisitCD(sve, fnArgsStack);
              }
            },
            [&](UserCall* uc) {
              const VectorRef<FunctionParam*> fnParams = uc->Target()->Params();
              size_t i = 0;
              VisitCD(uc, fnArgsStack);
              for (auto* a : uc->Args()) {
                if (a == value) {
                  std::vector<Slice<Value* const>> newArgsStack(fnArgsStack);
                  newArgsStack.push_back(uc->Args());
                  VisitForwardReference(fnParams[i], indexStack, newArgsStack);
                }
                i++;
              }
            },
            [&](CoreBuiltinCall* cbc) {
              // if this is an atomic store then we need to visit its control/data dependencies
              if (cbc->Func() == BuiltinFn::kAtomicStore) {
                VisitCD(cbc, fnArgsStack);
                for (auto* a : cbc->Args()) {
                  std::vector<Value*> newIndexStack;
                  VisitSliceValue(a, newIndexStack, fnArgsStack);
                }
              }
            },
            [&](Access* a) {
              VisitForwardReference(a->Result(0), indexStack, fnArgsStack);
            }
            // other types of usages we don't need to track
          );
          });
      }

      // Visit an instruction which is an index/data dependency of some access
      // Index stack maintains which members of a data structure are accessed, to determine
      // which members of a struct need to be loaded atomically
      // Function args stack maintains a stack of argument values passed into the function, so that values can be visited
      // based on how they are used in the function
      void VisitIDD(Instruction* inst, std::vector<Value*> indexStack, std::vector<tint::Slice<Value* const>> fnArgsStack) {

        // We must also visit the control dependencies of this instruction.
        // This is done before reverse-slicing the instruction, because if this is a load, it may be destroyed during the slicing
        // Note: visiting control dependencies can also cause a load to be destroyed, but since (I think) a load is never
        // the last instruction in a block, this currently works. A better solution might be to keep track of whether this load
        // is destroyed and handle it explicitly. 
        VisitCD(inst, fnArgsStack);

        tint::Switch(inst,
          // We record the indices in the index stack.
          // We don't need to visit its indices, as they will be handled by visiting this access directly.
          [&](Access* a) {
            auto indices = a->Indices();
            for (auto i = indices.rbegin(); i != indices.rend(); i++) {
              indexStack.push_back(*i);
            }
            //std::cout << "Index stack size: " << indexStack.size() << std::endl;
            VisitSliceValue(a->Object(), indexStack, fnArgsStack);
          },
          [&](Binary* i) {
            // data dependencies will have their own stack
            std::vector<Value*> lhsStack;
            VisitSliceValue(i->LHS(), lhsStack, fnArgsStack);
            std::vector<Value*> rhsStack;
            VisitSliceValue(i->RHS(), rhsStack, fnArgsStack);
          },
          [&](CoreBuiltinCall* cbc) {
            // we can short circuit if the value is already loaded atomically
            if (cbc->Func() == BuiltinFn::kAtomicLoad) {
              return;
            }
            else {
              // each argument will have its own stack
              for (auto* a : cbc->Args()) {
                std::vector<Value*> fnIndexStack;
                VisitSliceValue(a, fnIndexStack, fnArgsStack);
              }
            }
          },
          // For a user call, we add the arguments to the function argument stack and visit all return statements
          // within the function, slicing backwards from there.
          [&](UserCall* uc) {
            Traverse(uc->Target()->Block(), [&](Return* ret) {
              std::vector<Value*> fnIndexStack;
              std::vector<Slice<Value* const>> newArgsStack(fnArgsStack);
              newArgsStack.push_back(uc->Args());
              VisitSliceValue(ret->Value(), fnIndexStack, newArgsStack);
              });
          },
          // All other types of calls we treat as opaque
          [&](Call* c) {
            // each argument will have its own stack
            for (auto* a : c->Args()) {
              std::vector<Value*> fnIndexStack;
              VisitSliceValue(a, fnIndexStack, fnArgsStack);
            }
          },
          [&](Let* i) {
            // lets are pass through, so keep the same index stack
            VisitSliceValue(i->Value(), indexStack, fnArgsStack);
          },
          [&](LoadVectorElement* lve) {
            // We keep track of the vector element loaded in the index stack, so that it can be traced forward later on
            std::vector<Value*> vectorElement;
            vectorElement.push_back(lve->Index());
            VisitSliceValue(lve->From(), vectorElement, fnArgsStack);
          },
          // if this loads a compound type (e.g. nested arrays/structs), then the index stack 
          // will still apply to the value loaded from. Otherwise, the index stack will be empty.
          [&](Load* l) {
            VisitSliceValue(l->From(), indexStack, fnArgsStack);
          },
          // the index stack of the value is independent
          [&](Unary* u) {
            std::vector<Value*> unaryStack;
            VisitSliceValue(u->Val(), unaryStack, fnArgsStack);
          },
          // Compound if statements (e.g. (a || b)) are broken down into a series of ifs where the condition to the next
          // statement is the result of the previous one. To handle this, we need to visit both the condition
          // of each statement and the evaluation of the conditions, which are the arguments in the "exit_if" statements.
          [&](If* _if) {
            std::vector<Value*> condStack;
            VisitSliceValue(_if->Condition(), condStack, fnArgsStack);
            _if->ForeachBlock([&](Block* block) {
              Traverse(block, [&](ExitIf* exif) {
                for (auto* arg : exif->Args()) {
                  std::vector<Value*> exifArgIndexStack;
                  VisitSliceValue(arg, exifArgIndexStack, fnArgsStack);
                }
                });
              });
          },
          [&](Swizzle* swiz) {
            std::vector<Value*> swizStack;
            VisitSliceValue(swiz->Object(), swizStack, fnArgsStack);
          },
          [&](Var* v) {
            if (NeedsRewrite(v)) {
              RewriteRootVar(v, indexStack);
            }
            else if (v->Initializer() != nullptr) {
              // if the var initializes a compound type, then the index stack still applies
              // otherwise, this must be a symple type, in which case the index stack will still be empty
              VisitSliceValue(v->Initializer(), indexStack, fnArgsStack);
            }
            if (!visited_vars.Contains(v)) {
              // This may be a declaration of a private/function scoped variable. If this variable is stored
              // to directly elsewhere in the program, then we need to visit the control/data depedencies of the store.
              // Note that if this is a compound variable and it is stored to partially, then it will be accessed
              // prior to the store, and will already be visited by the root traversal.
              visited_vars.Add(v);
              VisitForwardReference(v->Result(0), indexStack, fnArgsStack);
            }
          },
          TINT_ICE_ON_NO_MATCH
        );
      }

      // visit control dependencies of an instruction
      void VisitCD(Instruction* inst, std::vector<tint::Slice<Value* const>> fnArgsStack) {
        auto* ctrl = inst->Block()->Parent();
        if (ctrl == nullptr || visited_ctrls.Contains(ctrl)) {
          // no control instruction guarding this block or already visited
          return;
        }
        else {
          // if, loop, switch
          visited_ctrls.Add(ctrl);
          tint::Switch(ctrl,
            // if/switch blocks are guarded by their condition.
            [&](If* ifInst) {
              std::vector<Value*> indexStack;
              VisitSliceValue(ifInst->Condition(), indexStack, fnArgsStack);
            },
            [&](Switch* sw) {
              std::vector<Value*> indexStack;
              VisitSliceValue(sw->Condition(), indexStack, fnArgsStack);
            },
            // A loop is split into an initializer, a body, and a continuing block.
            // The only place the loop can be exited is from the loop body, and only by calling
            // an exit_loop instruction inside a conditional. Therefore, the instructions/accesses
            // involved in this control dependency are the transitive instructions referenced by 
            // conditionals to exit the loop. Note that there may be multiple exit_loop instructions,
            // due to tint adding protections against infinite loops. Only one of these really matters,
            // and it's probably the last one by convention, but we traverse them all for safety.
            [&](Loop* loop) {
              Traverse(loop->Body(), [&](ExitLoop* exit) {
                VisitCD(exit, fnArgsStack);
                });
            },
            TINT_ICE_ON_NO_MATCH
          );
        }
      }

      // Visit control, index or data dependent values
      void VisitSliceValue(Value* i, std::vector<Value*> indexStack, std::vector<tint::Slice<Value* const>> fnArgsStack) {
        if (visited_values.Contains(i)) {
          // already visited this value
          return;
        }
        else {
          tint::Switch(i,
            [&](InstructionResult* res) {
              // we need to follow the chain of this index dependency
              VisitIDD(res->Instruction(), indexStack, fnArgsStack);
            },
            [&](Constant*) {
              // we can safely ignore constants
              return;
            },
            [&](BlockParam*) {
              // we currently ignore block parameters. TODO: Do we need to handle them in some way?
              return;
            },
            [&](FunctionParam* fp) {
              // If this is a compute entry point, the only function parameters are built-ins (e.g., invocation id)
              if (fp->Function()->Stage() == Function::PipelineStage::kCompute) {
                return;
                // Otherwise we map the parameter to the value its called from during this traversal, and continue
                // slicing from that value.
              }
              else {
                std::vector<tint::Slice<Value* const>> newArgsStack(fnArgsStack);
                tint::Slice<Value* const> curArgs = newArgsStack.back();
                newArgsStack.pop_back();
                VisitSliceValue(curArgs[fp->Index()], indexStack, newArgsStack);
              }
              return;
            },
            [&](Default) {
              TINT_ICE() << "unhandled value:" << ir.NameOf(i).Name() << ", " << i->Type()->FriendlyName() << "\n";
            }
          );
        }
      }

      // Traverse a function looking for accesses, as well as recursively traversing any functions called from this one.
      // TODO: If an access is part of the conditional calculation for a for-loop body, then right now it is considered
      // a control dependency of itself and is made atomic. However, this isn't really necessary, it's an artifact of how
      // the IR is structured. It seems like an access can be ignored if it occurs before the last exit-loop instruction, 
      // but this is not implemented yet.
      void VisitFunction(Function* f, std::vector<tint::Slice<Value* const>> fnArgsStack) {
        if (!void_function_stores.Contains(f)) {
          Traverse(f->Block(), [&](Store*) {
            void_function_stores.Add(f, true);
            });
          // adds to the same key do not overwrite previous adds
          void_function_stores.Add(f, false);
        }
        Traverse(f->Block(), [&](Access* access) {
          for (auto idx : access->Indices()) {
            std::vector<Value*> indexStack;
            VisitSliceValue(idx, indexStack, fnArgsStack);
          }
          VisitCD(access, fnArgsStack);
          });
        Traverse(f->Block(), [&](UserCall* uc) {
          std::vector<Slice<Value* const>> newArgsStack(fnArgsStack);
          newArgsStack.push_back(uc->Args());
          VisitFunction(uc->Target(), newArgsStack);
          // The called function stores, so we need to handle its control dependencies in this function
          if (void_function_stores.Get(uc->Target())) {
            VisitCD(uc, fnArgsStack);
          }
          });
      }

      /// Process the module.
      void Process() {
        auto before = Disassembler(ir);
        //std::cout << "// Shader Before:\n";
        //std::cout << before.Plain();
        //std::cout << "\n\n";

        bool entryPointFound = false;
        for (auto* f : ir.DependencyOrderedFunctions()) {
          if (f->Stage() == Function::PipelineStage::kCompute) {
            if (entryPointFound) {
              TINT_ICE() << "multiple compute entry points found\n";
            }
            entryPointFound = true;
            entry_point = ir.NameOf(f).Name();
            std::vector<Slice<Value* const>> fnArgsStack;
            fnArgsStack.push_back(Slice<Value* const>());
            VisitFunction(f, fnArgsStack);
          }
        }

        auto after = Disassembler(ir);
        //std::cout << "// Shader After:\n";
        //std::cout << after.Plain();
        //std::cout << "\n\n";

      }
    };

  }  // namespace

  Result<SMSGResult> SMSG(Module& ir, const SMSGConfig& config) {
    auto result = ValidateAndDumpIfNeeded(ir, "core.SMSG");
    if (result != Success) {
      return result.Failure();
    }
    auto start = std::chrono::high_resolution_clock::now();
    auto state = State{ config, ir };
    state.Process();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::microseconds elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    SMSGResult smsgResult;
    if (state.entry_point != "") {
      smsgResult.processed = true;
      smsgResult.time = elapsed.count();
      smsgResult.entry_point = state.entry_point;
      smsgResult.storage_rewrites = state.storage_rewrites;
      smsgResult.workgroup_rewrites = state.workgroup_rewrites;
      smsgResult.atomic_loads = state.atomic_loads;
      smsgResult.atomic_stores = state.atomic_stores;
      smsgResult.f32_rewrites = state.f32_rewrites;
      smsgResult.f32_replacements = state.f32_replacements;

      // print smsg result 
      std::cout << "// SMSG Result:\n";
      std::cout << "//   Processed: " << smsgResult.processed << "\n";
      std::cout << "//   Time: " << smsgResult.time << " microseconds\n";
      std::cout << "//   Entry Point: " << smsgResult.entry_point << "\n";
      std::cout << "//   Storage Rewrites: " << smsgResult.storage_rewrites << "\n";
      std::cout << "//   Workgroup Rewrites: " << smsgResult.workgroup_rewrites << "\n";
      std::cout << "//   Atomic Loads: " << smsgResult.atomic_loads << "\n";
      std::cout << "//   Atomic Stores: " << smsgResult.atomic_stores << "\n";
      std::cout << "//   F32 Rewrites: " << smsgResult.f32_rewrites << "\n";
      std::cout << "//   F32 Replacements: " << smsgResult.f32_replacements << "\n";
    }
    return smsgResult;
  }

} // namespace tint::core::ir::transform
