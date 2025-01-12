#ifndef SRC_DAWN_NATIVE_TIMESTAMP_INFO_H_
#define SRC_DAWN_NATIVE_TIMESTAMP_INFO_H_

#include <string>
#include <vector>

#include <dawn/native/wgpu_structs_autogen.h>


namespace dawn::native {

typedef struct TimestampInfo {
  ComputePassTimestampWrites timestampWrites;
  BufferBase * queryBuffer;
  BufferBase * stagingBuffer;
  size_t shaderHash;
  char* entryPoint;
  bool internalTimestampWrites;
} TimestampInfo;

}

#endif  // SRC_DAWN_NATIVE_TIMESTAMP_INFO_H_