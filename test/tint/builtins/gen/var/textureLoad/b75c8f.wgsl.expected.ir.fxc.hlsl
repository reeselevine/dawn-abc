//
// fragment_main
//

RWByteAddressBuffer prevent_dce : register(u0);
RWTexture2D<int4> arg_0 : register(u0, space1);
int4 textureLoad_b75c8f() {
  uint2 arg_1 = (1u).xx;
  int4 res = int4(arg_0.Load(int3(int2(arg_1), int(0))));
  return res;
}

void fragment_main() {
  prevent_dce.Store4(0u, asuint(textureLoad_b75c8f()));
}

//
// compute_main
//

RWByteAddressBuffer prevent_dce : register(u0);
RWTexture2D<int4> arg_0 : register(u0, space1);
int4 textureLoad_b75c8f() {
  uint2 arg_1 = (1u).xx;
  int4 res = int4(arg_0.Load(int3(int2(arg_1), int(0))));
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store4(0u, asuint(textureLoad_b75c8f()));
}

