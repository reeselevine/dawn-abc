// flags:  --hlsl-shader-model 62
enable f16;
var<private> t : f32;
fn m() -> mat4x4<f32> {
    t = t + 1.0f;
    return mat4x4<f32>(1.0f, 2.0f, 3.0f, 4.0f,
                       5.0f, 6.0f, 7.0f, 8.0f,
                       9.0f, 10.0f, 11.0f, 12.0f,
                       13.0f, 14.0f, 15.0f, 16.0f);
}
fn f() {
    var v : mat4x4<f16> = mat4x4<f16>(m());
}