//
// fragment_main
//

RWTexture2DArray<float4> arg_0 : register(u0, space1);
void textureStore_042b06() {
  uint2 arg_1 = (1u).xx;
  uint arg_2 = 1u;
  float4 arg_3 = (1.0f).xxxx;
  uint2 v = arg_1;
  float4 v_1 = arg_3;
  arg_0[uint3(v, uint(arg_2))] = v_1;
}

void fragment_main() {
  textureStore_042b06();
}

//
// compute_main
//

RWTexture2DArray<float4> arg_0 : register(u0, space1);
void textureStore_042b06() {
  uint2 arg_1 = (1u).xx;
  uint arg_2 = 1u;
  float4 arg_3 = (1.0f).xxxx;
  uint2 v = arg_1;
  float4 v_1 = arg_3;
  arg_0[uint3(v, uint(arg_2))] = v_1;
}

[numthreads(1, 1, 1)]
void compute_main() {
  textureStore_042b06();
}

