//
// fragment_main
//

RWByteAddressBuffer prevent_dce : register(u0);
Texture3D<uint4> arg_0 : register(t0, space1);
uint4 textureLoad_a9a9f5() {
  int3 arg_1 = (int(1)).xxx;
  int arg_2 = int(1);
  int v = arg_2;
  int3 v_1 = int3(arg_1);
  uint4 res = uint4(arg_0.Load(int4(v_1, int(v))));
  return res;
}

void fragment_main() {
  prevent_dce.Store4(0u, textureLoad_a9a9f5());
}

//
// compute_main
//

RWByteAddressBuffer prevent_dce : register(u0);
Texture3D<uint4> arg_0 : register(t0, space1);
uint4 textureLoad_a9a9f5() {
  int3 arg_1 = (int(1)).xxx;
  int arg_2 = int(1);
  int v = arg_2;
  int3 v_1 = int3(arg_1);
  uint4 res = uint4(arg_0.Load(int4(v_1, int(v))));
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store4(0u, textureLoad_a9a9f5());
}

//
// vertex_main
//
struct VertexOutput {
  float4 pos;
  uint4 prevent_dce;
};

struct vertex_main_outputs {
  nointerpolation uint4 VertexOutput_prevent_dce : TEXCOORD0;
  float4 VertexOutput_pos : SV_Position;
};


Texture3D<uint4> arg_0 : register(t0, space1);
uint4 textureLoad_a9a9f5() {
  int3 arg_1 = (int(1)).xxx;
  int arg_2 = int(1);
  int v = arg_2;
  int3 v_1 = int3(arg_1);
  uint4 res = uint4(arg_0.Load(int4(v_1, int(v))));
  return res;
}

VertexOutput vertex_main_inner() {
  VertexOutput v_2 = (VertexOutput)0;
  v_2.pos = (0.0f).xxxx;
  v_2.prevent_dce = textureLoad_a9a9f5();
  VertexOutput v_3 = v_2;
  return v_3;
}

vertex_main_outputs vertex_main() {
  VertexOutput v_4 = vertex_main_inner();
  vertex_main_outputs v_5 = {v_4.prevent_dce, v_4.pos};
  return v_5;
}

