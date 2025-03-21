//
// fragment_main
//

RWByteAddressBuffer prevent_dce : register(u0);
Texture2DArray<uint4> arg_0 : register(t0, space1);
uint4 textureLoad_7c90e5() {
  int2 arg_1 = (int(1)).xx;
  int arg_2 = int(1);
  int arg_3 = int(1);
  int v = arg_2;
  int v_1 = arg_3;
  int2 v_2 = int2(arg_1);
  int v_3 = int(v);
  uint4 res = uint4(arg_0.Load(int4(v_2, v_3, int(v_1))));
  return res;
}

void fragment_main() {
  prevent_dce.Store4(0u, textureLoad_7c90e5());
}

//
// compute_main
//

RWByteAddressBuffer prevent_dce : register(u0);
Texture2DArray<uint4> arg_0 : register(t0, space1);
uint4 textureLoad_7c90e5() {
  int2 arg_1 = (int(1)).xx;
  int arg_2 = int(1);
  int arg_3 = int(1);
  int v = arg_2;
  int v_1 = arg_3;
  int2 v_2 = int2(arg_1);
  int v_3 = int(v);
  uint4 res = uint4(arg_0.Load(int4(v_2, v_3, int(v_1))));
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store4(0u, textureLoad_7c90e5());
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


Texture2DArray<uint4> arg_0 : register(t0, space1);
uint4 textureLoad_7c90e5() {
  int2 arg_1 = (int(1)).xx;
  int arg_2 = int(1);
  int arg_3 = int(1);
  int v = arg_2;
  int v_1 = arg_3;
  int2 v_2 = int2(arg_1);
  int v_3 = int(v);
  uint4 res = uint4(arg_0.Load(int4(v_2, v_3, int(v_1))));
  return res;
}

VertexOutput vertex_main_inner() {
  VertexOutput v_4 = (VertexOutput)0;
  v_4.pos = (0.0f).xxxx;
  v_4.prevent_dce = textureLoad_7c90e5();
  VertexOutput v_5 = v_4;
  return v_5;
}

vertex_main_outputs vertex_main() {
  VertexOutput v_6 = vertex_main_inner();
  vertex_main_outputs v_7 = {v_6.prevent_dce, v_6.pos};
  return v_7;
}

