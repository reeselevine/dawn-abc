//
// fragment_main
//
#version 460
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer f_prevent_dce_block_ssbo {
  uvec2 inner;
} v;
uniform highp usamplerCubeArray arg_0;
uvec2 textureDimensions_a1598a() {
  uvec2 res = uvec2(textureSize(arg_0, 0).xy);
  return res;
}
void main() {
  v.inner = textureDimensions_a1598a();
}
//
// compute_main
//
#version 460

layout(binding = 0, std430)
buffer prevent_dce_block_1_ssbo {
  uvec2 inner;
} v;
uniform highp usamplerCubeArray arg_0;
uvec2 textureDimensions_a1598a() {
  uvec2 res = uvec2(textureSize(arg_0, 0).xy);
  return res;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  v.inner = textureDimensions_a1598a();
}
//
// vertex_main
//
#version 460


struct VertexOutput {
  vec4 pos;
  uvec2 prevent_dce;
};

uniform highp usamplerCubeArray arg_0;
layout(location = 0) flat out uvec2 tint_interstage_location0;
uvec2 textureDimensions_a1598a() {
  uvec2 res = uvec2(textureSize(arg_0, 0).xy);
  return res;
}
VertexOutput vertex_main_inner() {
  VertexOutput v = VertexOutput(vec4(0.0f), uvec2(0u));
  v.pos = vec4(0.0f);
  v.prevent_dce = textureDimensions_a1598a();
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
