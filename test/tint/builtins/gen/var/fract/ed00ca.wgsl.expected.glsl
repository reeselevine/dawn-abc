//
// fragment_main
//
#version 310 es
precision highp float;
precision highp int;

void fract_ed00ca() {
  vec2 res = vec2(0.25f);
}
void main() {
  fract_ed00ca();
}
//
// compute_main
//
#version 310 es

void fract_ed00ca() {
  vec2 res = vec2(0.25f);
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  fract_ed00ca();
}
//
// vertex_main
//
#version 310 es


struct VertexOutput {
  vec4 pos;
};

void fract_ed00ca() {
  vec2 res = vec2(0.25f);
}
VertexOutput vertex_main_inner() {
  VertexOutput v = VertexOutput(vec4(0.0f));
  v.pos = vec4(0.0f);
  fract_ed00ca();
  return v;
}
void main() {
  gl_Position = vertex_main_inner().pos;
  gl_Position.y = -(gl_Position.y);
  gl_Position.z = ((2.0f * gl_Position.z) - gl_Position.w);
  gl_PointSize = 1.0f;
}
