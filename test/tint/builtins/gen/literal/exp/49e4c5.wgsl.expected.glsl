//
// fragment_main
//
#version 310 es
precision highp float;
precision highp int;

void exp_49e4c5() {
  float res = 2.71828174591064453125f;
}
void main() {
  exp_49e4c5();
}
//
// compute_main
//
#version 310 es

void exp_49e4c5() {
  float res = 2.71828174591064453125f;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  exp_49e4c5();
}
//
// vertex_main
//
#version 310 es


struct VertexOutput {
  vec4 pos;
};

void exp_49e4c5() {
  float res = 2.71828174591064453125f;
}
VertexOutput vertex_main_inner() {
  VertexOutput v = VertexOutput(vec4(0.0f));
  v.pos = vec4(0.0f);
  exp_49e4c5();
  return v;
}
void main() {
  gl_Position = vertex_main_inner().pos;
  gl_Position.y = -(gl_Position.y);
  gl_Position.z = ((2.0f * gl_Position.z) - gl_Position.w);
  gl_PointSize = 1.0f;
}
