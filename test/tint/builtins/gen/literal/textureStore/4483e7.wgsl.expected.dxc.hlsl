//
// fragment_main
//
RWTexture1D<int4> arg_0 : register(u0, space1);

void textureStore_4483e7() {
  arg_0[1u] = (1).xxxx;
}

void fragment_main() {
  textureStore_4483e7();
  return;
}
//
// compute_main
//
RWTexture1D<int4> arg_0 : register(u0, space1);

void textureStore_4483e7() {
  arg_0[1u] = (1).xxxx;
}

[numthreads(1, 1, 1)]
void compute_main() {
  textureStore_4483e7();
  return;
}
