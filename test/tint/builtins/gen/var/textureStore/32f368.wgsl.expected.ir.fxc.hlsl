//
// fragment_main
//

RWTexture2DArray<float4> arg_0 : register(u0, space1);
void textureStore_32f368() {
  int2 arg_1 = (int(1)).xx;
  int arg_2 = int(1);
  float4 arg_3 = (1.0f).xxxx;
  int2 v = arg_1;
  float4 v_1 = arg_3;
  arg_0[int3(v, int(arg_2))] = v_1;
}

void fragment_main() {
  textureStore_32f368();
}

//
// compute_main
//

RWTexture2DArray<float4> arg_0 : register(u0, space1);
void textureStore_32f368() {
  int2 arg_1 = (int(1)).xx;
  int arg_2 = int(1);
  float4 arg_3 = (1.0f).xxxx;
  int2 v = arg_1;
  float4 v_1 = arg_3;
  arg_0[int3(v, int(arg_2))] = v_1;
}

[numthreads(1, 1, 1)]
void compute_main() {
  textureStore_32f368();
}

