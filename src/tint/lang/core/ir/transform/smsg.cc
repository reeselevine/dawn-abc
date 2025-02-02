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
    core::type::Manager& ty{ir.Types()};

    // Rewrite type to atomic
    // TODO: handle structs
    const type::Type* RewriteType(const type::Type* type) {
      return tint::Switch(type,
        [&](const type::Pointer* ptr) {
          return ty.ptr(ptr->AddressSpace(), RewriteType(ptr->UnwrapPtr()), ptr->Access());
        },
        [&](const type::Array* ar) {
          auto newElemType = RewriteType(ar->ElemType());
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

    void ForwardNewTypes(InstructionResult* res) {
      res->ForEachUseUnsorted([&](Usage use) {
        auto* inst = use.instruction;
        tint::Switch(inst,
          [&](Access* access) {
            auto* newType = RewriteType(access->Result(0)->Type());
            auto *innerRes = access->Result(0);
            innerRes->SetType(newType);
            ForwardNewTypes(innerRes);
          },
          [&](Load* load) {
            auto* replacement = b.CallWithResult(load->DetachResult(), BuiltinFn::kAtomicLoad, load->From());
            load->ReplaceWith(replacement);
            load->Destroy();
          },
          TINT_ICE_ON_NO_MATCH
        );
      });
    }

    const type::Type* TranslateBindingType(const type::Type* type) {
      return tint::Switch(type,
        [&](const type::Atomic*) {
          // atomic types are already in the correct format
          return type;
        },
        [&](const type::I32* i32) {
          // wrap the i32 in an atomic type
          return ty.Get<type::Atomic>(i32);
        },
        [&](const type::U32* u32) {
          // wrap the u32 in an atomic type
          return ty.Get<type::Atomic>(u32);
        },
        [&](const type::Array* ar) {
          auto newElemType = TranslateBindingType(ar->ElemType());
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
        TINT_ICE_ON_NO_MATCH
      );
    }

    void TranslateBindingPoint(Var* bp) {
      auto* oldPtr = bp->Result(0)->Type()->As<type::Pointer>();
      TINT_ASSERT(oldPtr);

      auto* newPtr = ty.ptr(oldPtr->AddressSpace(), TranslateBindingType(oldPtr->UnwrapPtr()), oldPtr->Access());
      bp->Result(0)->SetType(newPtr);
      ForwardNewTypes(bp->Result(0));
    }

    // Visit an instruction which is an index/data dependency of some access
    // TODO: Control dependencies, maybe don't fit in here?
    void VisitIDDInst(Instruction* inst) {
      tint::Switch(inst,
        // TODO
        // we can ignore the indices of the access, because they will be handled by a separate visitor
        [&](Access *a) {
          VisitIDDValue(a->Object());
        },
	      [&](Binary *i) {
          VisitIDDValue(i->LHS());
          VisitIDDValue(i->RHS());
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
            VisitIDDValue(a);
          }
        },
	      [&](Let *i) {
          VisitIDDValue(i->Value());
	      },
        // TODO
        [&](LoadVectorElement *lve) {
        },
	      [&](Load *l) { 
          VisitIDDValue(l->From());
        },
        // TODO
        [&](Unary *u) {
        },
        // TODO
        [&](Var *v) {
          if (v->BindingPoint()) {
            // Binding points are where atomic translation might need to take place
            TranslateBindingPoint(v);
          } else {
            VisitIDDValue(v->Initializer());
          }
        },
        TINT_ICE_ON_NO_MATCH
	    );
    }

    // Visit index or data dependent values
    void VisitIDDValue(Value* i) {
      if (auto* r = i->As<InstructionResult>()) {
        // we need to follow the chain of this index dependency
        VisitIDDInst(r->Instruction());
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
        VisitIDDValue(i);
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
