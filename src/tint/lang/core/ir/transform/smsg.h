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

// The result of running the SMSG transform.
struct SMSGResult {
  // Whether or not this transform applies (i.e., a compute entrypoint was found in the shader)
  bool processed = false;
  // The time it took to process the shader.
  double time = 0.0;
  // The entry point that was processed.
  std::string entry_point = "";
  // The number of storage buffer type rewrites.
  uint storage_rewrites = 0;
  // The number of workgroup buffer type rewrites.
  uint workgroup_rewrites = 0;
  // The number of added atomic loads.
  uint atomic_loads = 0;
  // The number of added atomic stores.
  uint atomic_stores = 0;
  // The number of f32 rewrites that we would have done if we could.
  uint f32_rewrites = 0;
  // The number of f32 atomic loads/stores we would have added if we could.
  uint f32_replacements = 0;
};


struct SMSGConfig {

  // Should storage buffer bindings types be rewritten?
  bool rewrite_storage = true;

  TINT_REFLECT(SMSGConfig, rewrite_storage);
};

/// SMSG is a transform that prevents out-of-bounds memory accesses.
/// @param module the module to transform
/// @returns success or failure
Result<SMSGResult> SMSG(Module& module, const SMSGConfig& config);

}  // namespace tint::core::ir::transform

#endif  // SRC_TINT_LANG_CORE_IR_TRANSFORM_SMSG_H_
