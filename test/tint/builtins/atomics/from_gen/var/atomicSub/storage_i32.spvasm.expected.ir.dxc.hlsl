//
// fragment_main
//

RWByteAddressBuffer sb_rw : register(u0);
void atomicSub_051100() {
  int arg_1 = int(0);
  int res = int(0);
  arg_1 = int(1);
  int x_20 = arg_1;
  int v = int(0);
  sb_rw.InterlockedAdd(int(0u), (int(0) - x_20), v);
  int x_13 = v;
  res = x_13;
}

void fragment_main_1() {
  atomicSub_051100();
}

void fragment_main() {
  fragment_main_1();
}

//
// compute_main
//

RWByteAddressBuffer sb_rw : register(u0);
void atomicSub_051100() {
  int arg_1 = int(0);
  int res = int(0);
  arg_1 = int(1);
  int x_20 = arg_1;
  int v = int(0);
  sb_rw.InterlockedAdd(int(0u), (int(0) - x_20), v);
  int x_13 = v;
  res = x_13;
}

void compute_main_1() {
  atomicSub_051100();
}

[numthreads(1, 1, 1)]
void compute_main() {
  compute_main_1();
}

