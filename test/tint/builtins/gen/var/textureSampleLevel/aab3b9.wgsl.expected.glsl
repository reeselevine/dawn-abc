//
// fragment_main
//
#version 460
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer f_prevent_dce_block_ssbo {
  vec4 inner;
} v;
uniform highp samplerCubeArray arg_0_arg_1;
vec4 textureSampleLevel_aab3b9() {
  vec3 arg_2 = vec3(1.0f);
  uint arg_3 = 1u;
  float arg_4 = 1.0f;
  vec3 v_1 = arg_2;
  float v_2 = arg_4;
  vec4 v_3 = vec4(v_1, float(arg_3));
  vec4 res = textureLod(arg_0_arg_1, v_3, float(v_2));
  return res;
}
void main() {
  v.inner = textureSampleLevel_aab3b9();
}
//
// compute_main
//
#version 460

layout(binding = 0, std430)
buffer prevent_dce_block_1_ssbo {
  vec4 inner;
} v;
uniform highp samplerCubeArray arg_0_arg_1;
vec4 textureSampleLevel_aab3b9() {
  vec3 arg_2 = vec3(1.0f);
  uint arg_3 = 1u;
  float arg_4 = 1.0f;
  vec3 v_1 = arg_2;
  float v_2 = arg_4;
  vec4 v_3 = vec4(v_1, float(arg_3));
  vec4 res = textureLod(arg_0_arg_1, v_3, float(v_2));
  return res;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  v.inner = textureSampleLevel_aab3b9();
}
//
// vertex_main
//
#version 460


struct VertexOutput {
  vec4 pos;
  vec4 prevent_dce;
};

uniform highp samplerCubeArray arg_0_arg_1;
layout(location = 0) flat out vec4 tint_interstage_location0;
vec4 textureSampleLevel_aab3b9() {
  vec3 arg_2 = vec3(1.0f);
  uint arg_3 = 1u;
  float arg_4 = 1.0f;
  vec3 v = arg_2;
  float v_1 = arg_4;
  vec4 v_2 = vec4(v, float(arg_3));
  vec4 res = textureLod(arg_0_arg_1, v_2, float(v_1));
  return res;
}
VertexOutput vertex_main_inner() {
  VertexOutput v_3 = VertexOutput(vec4(0.0f), vec4(0.0f));
  v_3.pos = vec4(0.0f);
  v_3.prevent_dce = textureSampleLevel_aab3b9();
  return v_3;
}
void main() {
  VertexOutput v_4 = vertex_main_inner();
  gl_Position = v_4.pos;
  gl_Position.y = -(gl_Position.y);
  gl_Position.z = ((2.0f * gl_Position.z) - gl_Position.w);
  tint_interstage_location0 = v_4.prevent_dce;
  gl_PointSize = 1.0f;
}
