#include "src/tint/lang/core/ir/transform/smsg.h"

#include "src/tint/lang/core/ir/analysis/loop_analysis.h"
#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
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


    /// Process the module.
    void Process() {
      std::cout << "SMSG::Process()" << std::endl;
    }
};

}  // namespace

Result<SuccessType> SMSG(Module& ir) {
    auto result = ValidateAndDumpIfNeeded(ir, "core.SMSG");
    if (result != Success) {
        return result;
    }

    State{ir}.Process();

    return Success;
}

} // namespace tint::core::ir::transform
