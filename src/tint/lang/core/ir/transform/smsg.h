#ifndef SRC_TINT_LANG_CORE_IR_TRANSFORM_SMSG_H_
#define SRC_TINT_LANG_CORE_IR_TRANSFORM_SMSG_H_

#include <string>
#include <unordered_set>

#include "src/tint/api/common/binding_point.h"
#include "src/tint/utils/reflection.h"
#include "src/tint/utils/result/result.h"

// Forward declarations.
namespace tint::core::ir {
class Module;
}

namespace tint::core::ir::transform {

struct SMSGConfig {

  // Should storage buffer bindings types be rewritten?
  bool rewrite_storage = true;

  TINT_REFLECT(SMSGConfig, rewrite_storage);
};

/// SMSG is a transform that prevents out-of-bounds memory accesses.
/// @param module the module to transform
/// @returns success or failure
Result<SuccessType> SMSG(Module& module, const SMSGConfig& config);

}  // namespace tint::core::ir::transform

#endif  // SRC_TINT_LANG_CORE_IR_TRANSFORM_SMSG_H_
