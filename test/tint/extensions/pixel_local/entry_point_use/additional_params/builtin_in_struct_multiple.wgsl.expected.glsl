SKIP: FAILED


enable chromium_experimental_pixel_local;

struct PixelLocal {
  a : u32,
  b : i32,
  c : f32,
}

var<pixel_local> P : PixelLocal;

struct In {
  @builtin(position)
  pos : vec4f,
  @builtin(front_facing)
  ff : bool,
  @builtin(sample_index)
  si : u32,
}

@fragment
fn f(tint_symbol : In) {
  P.a += u32(tint_symbol.pos.x);
}

Failed to generate: C:\src\dawn\test\tint\extensions\pixel_local\entry_point_use\additional_params\builtin_in_struct_multiple.wgsl:2:8 error: GLSL backend does not support extension 'chromium_experimental_pixel_local'
enable chromium_experimental_pixel_local;
       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


tint executable returned error: exit status 1
