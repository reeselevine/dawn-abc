//
// fragment_main
//
#version 460
precision highp float;
precision highp int;

layout(binding = 0, rg32ui) uniform highp writeonly uimage2DArray arg_0;
void textureStore_4d359d() {
  ivec2 v = ivec2(uvec2(1u));
  imageStore(arg_0, ivec3(v, int(1)), uvec4(1u));
}
void main() {
  textureStore_4d359d();
}
//
// compute_main
//
#version 460

layout(binding = 0, rg32ui) uniform highp writeonly uimage2DArray arg_0;
void textureStore_4d359d() {
  ivec2 v = ivec2(uvec2(1u));
  imageStore(arg_0, ivec3(v, int(1)), uvec4(1u));
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  textureStore_4d359d();
}
