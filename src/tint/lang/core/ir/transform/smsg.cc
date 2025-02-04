#include "src/tint/lang/core/ir/transform/smsg.h"

#include "src/tint/lang/core/ir/disassembler.h"
#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/traverse.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/utils/ice/ice.h"

#include <iostream>

namespace tint::core::ir::transform {

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

    /// Map from a type to a helper function that will convert its rewritten form back to it.
    Hashmap<const type::Type*, Function*, 4> convert_helpers{};

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

    Function* BuildHelper(const type::Type* targetType, const type::Pointer* inputType) {
      return convert_helpers.GetOrAdd(targetType, [&] {
        auto* func = b.Function(targetType);
        auto* input = b.FunctionParam("tint_input", inputType);
        func->SetParams({input});
        b.Append(func->Block(), [&] {
          tint::Switch(targetType,
            [&](const type::Struct* st) {
              auto* inputStruct = inputType->StoreType()->As<type::Struct>();
              uint32_t index = 0;
              Vector<Value*, 4> args;
              for (auto* member : st->Members()) {
                // types are the same, can load directly
                auto* accessType = ty.ptr(inputType->AddressSpace(), inputStruct->Element(index), inputType->Access());
                auto* extractRes = b.Access(accessType, input, u32(index))->Result(0);
                if (member->Type() == inputStruct->Element(index)) {
                  args.Push(b.Load(extractRes)->Result(0));
                // otherwise need to convert the type
                } else {
                  tint::Switch(member->Type(),
                    // type is a scalar, load it atomically
                    [&](const type::NumericScalar* ns) {
                      args.Push(b.Call(ns, BuiltinFn::kAtomicLoad, extractRes)->Result(0));
                    },
                    // type is a nested struct/array, recursively build the helper
                    [&](Default) {
                      auto* helper = BuildHelper(member->Type(), accessType);
                      args.Push(b.Call(helper, extractRes)->Result(0));
                    }
                  );
                }
                index++;
              }
              b.Return(func, b.Construct(st, std::move(args)));
            },
            TINT_ICE_ON_NO_MATCH
          );
        });
        return func;
      });
    }


    /// Convert a value that contains atomic types to an instruction result with the original type.
    Call* Convert(InstructionResult* result, Value* source) {
      return tint::Switch(result->Type(),
        [&](const type::NumericScalar*) {
          return b.CallWithResult(result, BuiltinFn::kAtomicLoad, source);
        },
        [&](Default) {
          auto* helper = BuildHelper(result->Type(), source->Type()->As<type::Pointer>());
          return b.CallWithResult(result, helper, source);
        }
      );
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
            auto* newCall = Convert(load->DetachResult(), load->From());
            load->ReplaceWith(newCall);
            load->Destroy();
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
      if (auto* r = i->As<InstructionResult>()) {
        // we need to follow the chain of this index dependency
        VisitIDDInst(r->Instruction(), indexStack);
      } else if (auto* c = i->As<Constant>()) {
        // we can safely ignore constants
        return;
      } else {
        TINT_ICE() << "unhandled value\n";
      }
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
