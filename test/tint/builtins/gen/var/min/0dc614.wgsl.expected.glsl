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
uvec4 min_0dc614() {
  uvec4 arg_0 = uvec4(1u);
  uvec4 arg_1 = uvec4(1u);
  uvec4 res = min(arg_0, arg_1);
  return res;
}
void main() {
  v.inner = min_0dc614();
}
//
// compute_main
//
#version 310 es

layout(binding = 0, std430)
buffer prevent_dce_block_1_ssbo {
  uvec4 inner;
} v;
uvec4 min_0dc614() {
  uvec4 arg_0 = uvec4(1u);
  uvec4 arg_1 = uvec4(1u);
  uvec4 res = min(arg_0, arg_1);
  return res;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  v.inner = min_0dc614();
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
uvec4 min_0dc614() {
  uvec4 arg_0 = uvec4(1u);
  uvec4 arg_1 = uvec4(1u);
  uvec4 res = min(arg_0, arg_1);
  return res;
}
VertexOutput vertex_main_inner() {
  VertexOutput v = VertexOutput(vec4(0.0f), uvec4(0u));
  v.pos = vec4(0.0f);
  v.prevent_dce = min_0dc614();
  return v;
}
void main() {
  VertexOutput v_1 = vertex_main_inner();
  gl_Position = v_1.pos;
  gl_Position.y = -(gl_Position.y);
  gl_Position.z = ((2.0f * gl_Position.z) - gl_Position.w);
  tint_interstage_location0 = v_1.prevent_dce;
  gl_PointSize = 1.0f;
}
