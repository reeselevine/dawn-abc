#version 310 es


struct tint_push_constant_struct {
  uint tint_first_instance;
};

layout(location = 0) uniform tint_push_constant_struct tint_push_constants;
vec4 main_inner(uint vertex_index, uint instance_index) {
  uint foo = (vertex_index + instance_index);
  return vec4(0.0f);
}
void main() {
  uint v = uint(gl_VertexID);
  uint v_1 = uint(gl_InstanceID);
  gl_Position = main_inner(v, (v_1 + tint_push_constants.tint_first_instance));
  gl_Position.y = -(gl_Position.y);
  gl_Position.z = ((2.0f * gl_Position.z) - gl_Position.w);
  gl_PointSize = 1.0f;
}
