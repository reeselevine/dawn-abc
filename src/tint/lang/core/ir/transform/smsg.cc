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

    const type::Type* RewriteBindingType(const type::Type* type) {
      return tint::Switch(type,
        [&](const type::I32* i32) {
          return ty.Get<type::Atomic>(i32);
        },
        [&](const type::U32* u32) {
          return ty.Get<type::Atomic>(u32);
        },
        [&](const type::Array* ar) {
          auto newElemType = RewriteBindingType(ar->ElemType());
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

    void HandleBindingPoint(Var* bp) {
      auto* oldPtr = bp->Result(0)->Type()->As<type::Pointer>();
      TINT_ASSERT(oldPtr);

      auto* newPtr = ty.ptr(oldPtr->AddressSpace(), RewriteBindingType(oldPtr->UnwrapPtr()), oldPtr->Access());
      bp->Result(0)->SetType(newPtr);
      bp->Result(0)->ForEachUseUnsorted([&](Usage use) {
        auto* inst = use.instruction;
        auto* access = inst->As<Access>();
        if (!access) {
          TINT_ICE() << "binding point usage is not an access\n";
        }
        auto* result = access->Result(0);
        tint::Switch(newPtr->UnwrapPtr(),
          [&](const type::Array* ar) {
            auto* newAccessType = ty.ptr(newPtr->AddressSpace(), ar->ElemType(), newPtr->Access());
            result->SetType(newAccessType);
            result->ForEachUseUnsorted([&](Usage innerUse) {
              auto* innerInst = innerUse.instruction;
              auto* innerAccess = innerInst->As<Load>();
              if (!innerAccess) {
                TINT_ICE() << "access usage is not a load\n";
              }
              auto* replacement = b.CallWithResult(innerAccess->DetachResult(), BuiltinFn::kAtomicLoad, innerAccess->From());
              innerInst->ReplaceWith(replacement);
              innerInst->Destroy();
            });
          }
        );
      });
    }

    void VisitInstruction(Instruction* inst) {
      tint::Switch(inst,
        // TODO
        // we can ignore the indices of the access, because they will be handled by a separate visitor
        [&](Access *a) {
          VisitValue(a->Object());
        },
	      [&](Binary *i) {
          VisitValue(i->LHS());
          VisitValue(i->RHS());
        },
        // TODO
        [&](Bitcast *bt) {
        },
        // TODO think about recursion into call
        [&](Call *c) {
          for(auto* a: c->Args()) {
            VisitValue(a);
          }
        },
	      [&](Let *i) {
          VisitValue(i->Value());
	      },
        // TODO
        [&](LoadVectorElement *lve) {
        },
	      [&](Load *l) { 
          VisitValue(l->From());
        },
        // TODO
        [&](Unary *u) {
        },
        // TODO
        [&](Var *v) {
          if (v->BindingPoint()) {
            std::cout << "root var\n";
            HandleBindingPoint(v);
          } else {
            VisitValue(v->Initializer());
          }
        },
        TINT_ICE_ON_NO_MATCH
	    );
    }

    void VisitValue(Value* i) {
      if (auto* c = i->As<Constant>()) {
        std::cout << c->Value()->ValueAs<uint32_t>() << "\n";
      } else if (auto* r = i->As<InstructionResult>()) {
        VisitInstruction(r->Instruction());
      } else {
        TINT_ICE() << "unhandled value\n";
      }
    }

    void VisitAccess(Access *a) {
      std::cout << ir.NameOf(a->Object()).Name() << "\n"; 
      for (auto i : a->Indices()) {
        VisitValue(i);
      }
    }

    void VisitComputeEntryPoint(Function* f) {
      std::cout << ir.NameOf(f).Name() << std::endl;
      if(f->Stage() != Function::PipelineStage::kCompute)
        return;
      Traverse(f->Block(), [&](Access* a) {
        std::cout << "access: \n";
        VisitAccess(a);
      });
    }

    /// Process the module.
    Result<SuccessType> Process() {
      auto before = Disassembler(ir);
      std::cout << "// Shader Before:\n";
      std::cout << before.Plain();
      std::cout << "\n\n";

      for(auto *f: ir.DependencyOrderedFunctions()) {
        VisitComputeEntryPoint(f);
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
