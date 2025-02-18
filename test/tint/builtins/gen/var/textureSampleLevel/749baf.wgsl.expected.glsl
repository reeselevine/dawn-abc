//
// fragment_main
//
#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer f_prevent_dce_block_ssbo {
  float inner;
} v;
uniform highp sampler2DShadow arg_0_arg_1;
float textureSampleLevel_749baf() {
  vec2 arg_2 = vec2(1.0f);
  int arg_3 = 1;
  int v_1 = arg_3;
  vec3 v_2 = vec3(arg_2, 0.0f);
  float res = textureLodOffset(arg_0_arg_1, v_2, float(v_1), ivec2(1));
  return res;
}
void main() {
  v.inner = textureSampleLevel_749baf();
}
//
// compute_main
//
#version 310 es

layout(binding = 0, std430)
buffer prevent_dce_block_1_ssbo {
  float inner;
} v;
uniform highp sampler2DShadow arg_0_arg_1;
float textureSampleLevel_749baf() {
  vec2 arg_2 = vec2(1.0f);
  int arg_3 = 1;
  int v_1 = arg_3;
  vec3 v_2 = vec3(arg_2, 0.0f);
  float res = textureLodOffset(arg_0_arg_1, v_2, float(v_1), ivec2(1));
  return res;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  v.inner = textureSampleLevel_749baf();
}
//
// vertex_main
//
#version 310 es


struct VertexOutput {
  vec4 pos;
  float prevent_dce;
};

uniform highp sampler2DShadow arg_0_arg_1;
layout(location = 0) flat out float tint_interstage_location0;
float textureSampleLevel_749baf() {
  vec2 arg_2 = vec2(1.0f);
  int arg_3 = 1;
  int v = arg_3;
  vec3 v_1 = vec3(arg_2, 0.0f);
  float res = textureLodOffset(arg_0_arg_1, v_1, float(v), ivec2(1));
  return res;
}
VertexOutput vertex_main_inner() {
  VertexOutput v_2 = VertexOutput(vec4(0.0f), 0.0f);
  v_2.pos = vec4(0.0f);
  v_2.prevent_dce = textureSampleLevel_749baf();
  return v_2;
}
void main() {
  VertexOutput v_3 = vertex_main_inner();
  gl_Position = v_3.pos;
  gl_Position.y = -(gl_Position.y);
  gl_Position.z = ((2.0f * gl_Position.z) - gl_Position.w);
  tint_interstage_location0 = v_3.prevent_dce;
  gl_PointSize = 1.0f;
}
