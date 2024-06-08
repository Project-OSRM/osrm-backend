pub fn approx_equal(a: f32, b: f32, dp: u8) -> bool {
    let p = 10f32.powi(-(dp as i32));
    (a - b).abs() < p
}