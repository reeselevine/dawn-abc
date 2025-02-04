#include "src/tint/lang/core/ir/transform/smsg.h"

#include "src/tint/lang/core/ir/disassembler.h"
#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/traverse.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/utils/ice/ice.h"

#include <iostream>

using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::core::ir::transform {

// TODO:
// handle arbitrary size struct fields
// handle control dependencies
// handle interprocedural analysis

namespace {

/// PIMPL state for the transform.
struct State {
    /// The IR module.
    Module& ir;

    /// The IR builder.
    Builder b{ir};

    /// The type manager.
    type::Manager& ty{ir.Types()};

    /// The symbol table.
    SymbolTable& sym{ir.symbols};

    /// Map from structs with non-atomic members to structs with atomic members
    Hashmap<const type::Struct*, const type::Struct*, 4> struct_map{};

    /// Map from a struct to a helper function that will either convert a rewritten type 
    /// to an original type (for loads) or store an original type to a rewritten type.
    Hashmap<const type::Struct*, Function*, 4> convert_helpers{};

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
        [&](const type::NumericScalar *ns) {
          return b.Call(ns, BuiltinFn::kAtomicLoad, source)->Result(0);
        },
        // call a helper function which takes in a pointer to the source type and returns the target type
        [&](const type::Struct *st) {
          auto* helper = convert_helpers.GetOrAdd(st, [&] {
            auto* func = b.Function(st);
            auto* input = b.FunctionParam("tint_input", sourcePtr);
            func->SetParams({input});
            b.Append(func->Block(), [&] {
              auto* sourceStruct = sourcePtr->StoreType()->As<type::Struct>();
              TINT_ASSERT(sourceStruct);
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
        [&](const type::Array *arr) {
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
      } else {
        tint::Switch(toStoreType,
          // store type is atomic, store the value atomically
          [&](const type::Atomic*) {
            b.Call(ty.void_(), BuiltinFn::kAtomicStore, to, from);
          },
          // call a helper which takes in a pointer to the store type and the value to store and stores it
          [&](const type::Struct *st) {
            auto* helper = convert_helpers.GetOrAdd(st, [&] {
              auto* func = b.Function(ty.void_());
              auto* fromInput = b.FunctionParam("tint_from", from->Type());
              auto* toInput = b.FunctionParam("tint_to", to->Type());
              func->SetParams({fromInput, toInput});
              b.Append(func->Block(), [&] {
                auto* fromStruct = from->Type()->As<type::Struct>();
                TINT_ASSERT(fromStruct);
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
          [&](const type::Array *arr) {
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
    void Replace(InstructionResult* res) {
      res->ForEachUseUnsorted([&](Usage use) {
        auto* inst = use.instruction;
        tint::Switch(inst,
          [&](Access* access) {
            auto* newType = ReplaceType(access->Result(0)->Type());
            auto *innerRes = access->Result(0);
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
            auto *innerRes = let->Result(0);
            innerRes->SetType(res->Type());
            Replace(innerRes);
          },
          TINT_ICE_ON_NO_MATCH
        );
      });
    }

    // Rewrite type used by shader. Unhandled types raise an internal compiler error
    const type::Type* RewriteType(const type::Type* type, std::vector<Value*> indexStack) {
      return tint::Switch(type,
        [&](const type::Atomic*) {
          // atomic types are already in the correct format
          // TODO: is this necessary due to short-circuit in VisitIDDInst?
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
        [&](const type::Array* ar) {
          // the index of the array access may be unknown, but it's not needed as all array members have the same type
          indexStack.pop_back();
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
          auto* idx = indexStack.back();
          indexStack.pop_back();
          // struct accesses should always be constants
          auto* newStruct = tint::Switch(idx,
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
              return _newStruct;
            },
            TINT_ICE_ON_NO_MATCH
          );
          return newStruct;
        },
        TINT_ICE_ON_NO_MATCH
      );
    }

    // Rewrite the type of the binding point and then replace usages of the binding point using the updated type
    void RewriteBindingPoint(Var* bp, std::vector<Value*> indexStack) {
      auto* oldPtr = bp->Result(0)->Type()->As<type::Pointer>();
      TINT_ASSERT(oldPtr);

      auto* newPtr = ty.ptr(oldPtr->AddressSpace(), RewriteType(oldPtr->UnwrapPtr(), indexStack), oldPtr->Access());
      bp->Result(0)->SetType(newPtr);
      Replace(bp->Result(0));
    }

    // Visit an instruction which is an index/data dependency of some access
    // Index stack maintains which members of a data structure are accessed, to determine
    // which members of a struct need to be loaded atomically
    // TODO: Control dependencies, maybe don't fit in here?
    void VisitIDDInst(Instruction* inst, std::vector<Value*> indexStack) {
      tint::Switch(inst,
        // we don't need to visit the indices of the access, because they will be handled by a separate visitor
        // however, we do record the indices in the indexStack
        [&](Access *a) {
          auto indices = a->Indices();
          for (auto i = indices.rbegin(); i != indices.rend(); i++) {
            indexStack.push_back(*i);
          }
          VisitIDDValue(a->Object(), indexStack);
        },
	      [&](Binary *i) {
          VisitIDDValue(i->LHS(), indexStack);
          VisitIDDValue(i->RHS(), indexStack);
        },
        // TODO
        [&](Bitcast *bt) {
        },
        [&](CoreBuiltinCall *cbc) {
          // we can short circuit if the value is already loaded atomically
          if (cbc->Func() == BuiltinFn::kAtomicLoad) {
            return;
          }
        },
        // TODO think about jumping into call
        [&](Call *c) {
          for(auto* a: c->Args()) {
            VisitIDDValue(a, indexStack);
          }
        },
	      [&](Let *i) {
          VisitIDDValue(i->Value(), indexStack);
	      },
        // TODO
        [&](LoadVectorElement *lve) {
        },
	      [&](Load *l) { 
          VisitIDDValue(l->From(), indexStack);
        },
        // TODO
        [&](Unary *u) {
        },
        // TODO
        [&](Var *v) {
          if (v->BindingPoint()) {
            // Binding points are where atomic translation might need to take place
            RewriteBindingPoint(v, indexStack);
          } else {
            VisitIDDValue(v->Initializer(), indexStack);
          }
        },
        TINT_ICE_ON_NO_MATCH
	    );
    }

    // Visit index or data dependent values
    void VisitIDDValue(Value* i, std::vector<Value*> indexStack) {
      tint::Switch(i,
        [&](InstructionResult* res) {
          // we need to follow the chain of this index dependency
          VisitIDDInst(res->Instruction(), indexStack);
        },
        [&](Constant* c) {
          // we can safely ignore constants
          return;
        },
        [&](FunctionParam *fp) {
          // currently we are only visiting compute entry point, in which case function params are built-ins (e.g., invocation id)
          // TODO: for interprocedural analysis, need to trace calls of this function and follow input parameter chains
          return;
        },
        [&](Default) {
          TINT_ICE() << "unhandled value:" << ir.NameOf(i).Name() << ", " << i->Type()->FriendlyName() << "\n";
        }
      );
    }

    // Visiting an access simply means visiting its index dependencies
    void VisitAccess(Access *a) {
      for (auto i : a->Indices()) {
        std::vector<Value*> indexStack;
        VisitIDDValue(i, indexStack);
      }
    }

    // Traverse the compute entry point looking for accesses.
    // TODO: Interprocedural analysis
    void VisitComputeEntryPoint(Function* f) {
      Traverse(f->Block(), [&](Access* a) {
        VisitAccess(a);
      });
    }

    /// Process the module.
    Result<SuccessType> Process() {
      auto before = Disassembler(ir);
      std::cout << "// Shader Before:\n";
      std::cout << before.Plain();
      std::cout << "\n\n";

      bool entryPointFound = false;
      for(auto *f: ir.DependencyOrderedFunctions()) {
        if (f->Stage() == Function::PipelineStage::kCompute) {
          if (entryPointFound) {
            TINT_ICE() << "multiple compute entry points found\n";
          }
          entryPointFound = true;
          VisitComputeEntryPoint(f);
        }
      }

      auto after = Disassembler(ir);
      std::cout << "// Shader After:\n";
      std::cout << after.Plain();
      std::cout << "\n\n";
      return Success;
    }
};

}  // namespace

Result<SuccessType> SMSG(Module& ir) {
    auto result = ValidateAndDumpIfNeeded(ir, "core.SMSG");
    if (result != Success) {
        return result;
    }

    return State{ir}.Process();
}

} // namespace tint::core::ir::transform
