//
// fragment_main
//
#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer f_prevent_dce_block_ssbo {
  uvec4 inner;
} v;
uvec4 firstTrailingBit_110f2c() {
  uvec4 arg_0 = uvec4(1u);
  uvec4 v_1 = arg_0;
  uvec4 res = mix((mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u))) | (mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u))) | (mix(uvec4(0u), uvec4(4u), equal((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) & uvec4(15u)), uvec4(0u))) | (mix(uvec4(0u), uvec4(2u), equal(((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(4u), equal((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) & uvec4(15u)), uvec4(0u)))) & uvec4(3u)), uvec4(0u))) | mix(uvec4(0u), uvec4(1u), equal((((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(4u), equal((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) & uvec4(15u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(2u), equal(((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(4u), equal((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) & uvec4(15u)), uvec4(0u)))) & uvec4(3u)), uvec4(0u)))) & uvec4(1u)), uvec4(0u))))))), uvec4(4294967295u), equal(((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(4u), equal((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) & uvec4(15u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(2u), equal(((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(4u), equal((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) & uvec4(15u)), uvec4(0u)))) & uvec4(3u)), uvec4(0u)))), uvec4(0u)));
  return res;
}
void main() {
  v.inner = firstTrailingBit_110f2c();
}
//
// compute_main
//
#version 310 es

layout(binding = 0, std430)
buffer prevent_dce_block_1_ssbo {
  uvec4 inner;
} v;
uvec4 firstTrailingBit_110f2c() {
  uvec4 arg_0 = uvec4(1u);
  uvec4 v_1 = arg_0;
  uvec4 res = mix((mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u))) | (mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u))) | (mix(uvec4(0u), uvec4(4u), equal((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) & uvec4(15u)), uvec4(0u))) | (mix(uvec4(0u), uvec4(2u), equal(((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(4u), equal((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) & uvec4(15u)), uvec4(0u)))) & uvec4(3u)), uvec4(0u))) | mix(uvec4(0u), uvec4(1u), equal((((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(4u), equal((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) & uvec4(15u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(2u), equal(((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(4u), equal((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) & uvec4(15u)), uvec4(0u)))) & uvec4(3u)), uvec4(0u)))) & uvec4(1u)), uvec4(0u))))))), uvec4(4294967295u), equal(((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(4u), equal((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) & uvec4(15u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(2u), equal(((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(4u), equal((((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v_1 >> mix(uvec4(0u), uvec4(16u), equal((v_1 & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) & uvec4(15u)), uvec4(0u)))) & uvec4(3u)), uvec4(0u)))), uvec4(0u)));
  return res;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  v.inner = firstTrailingBit_110f2c();
}
//
// vertex_main
//
#version 310 es


struct VertexOutput {
  vec4 pos;
  uvec4 prevent_dce;
};

layout(location = 0) flat out uvec4 tint_interstage_location0;
uvec4 firstTrailingBit_110f2c() {
  uvec4 arg_0 = uvec4(1u);
  uvec4 v = arg_0;
  uvec4 res = mix((mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u))) | (mix(uvec4(0u), uvec4(8u), equal(((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u))) | (mix(uvec4(0u), uvec4(4u), equal((((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) & uvec4(15u)), uvec4(0u))) | (mix(uvec4(0u), uvec4(2u), equal(((((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(4u), equal((((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) & uvec4(15u)), uvec4(0u)))) & uvec4(3u)), uvec4(0u))) | mix(uvec4(0u), uvec4(1u), equal((((((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(4u), equal((((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) & uvec4(15u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(2u), equal(((((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(4u), equal((((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) & uvec4(15u)), uvec4(0u)))) & uvec4(3u)), uvec4(0u)))) & uvec4(1u)), uvec4(0u))))))), uvec4(4294967295u), equal(((((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(4u), equal((((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) & uvec4(15u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(2u), equal(((((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(4u), equal((((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) >> mix(uvec4(0u), uvec4(8u), equal(((v >> mix(uvec4(0u), uvec4(16u), equal((v & uvec4(65535u)), uvec4(0u)))) & uvec4(255u)), uvec4(0u)))) & uvec4(15u)), uvec4(0u)))) & uvec4(3u)), uvec4(0u)))), uvec4(0u)));
  return res;
}
VertexOutput vertex_main_inner() {
  VertexOutput v_1 = VertexOutput(vec4(0.0f), uvec4(0u));
  v_1.pos = vec4(0.0f);
  v_1.prevent_dce = firstTrailingBit_110f2c();
  return v_1;
}
void main() {
  VertexOutput v_2 = vertex_main_inner();
  gl_Position = v_2.pos;
  gl_Position.y = -(gl_Position.y);
  gl_Position.z = ((2.0f * gl_Position.z) - gl_Position.w);
  tint_interstage_location0 = v_2.prevent_dce;
  gl_PointSize = 1.0f;
}
