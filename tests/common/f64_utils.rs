pub fn approx_equal(a: f32, b: f32, dp: u8) -> bool {
    let p = 10f32.powi(-(dp as i32));
    (a - b).abs() < p
}

pub fn aprox_equal_within_percentage_range(actual: f64, expectation: f64, percentage: f64) -> bool {
    assert!(percentage.is_sign_positive() && percentage <= 100.);
    let factor = 0.01 * percentage as f64;
    actual >= expectation - (factor * expectation) && actual <= expectation + (factor * expectation)
}

pub fn approx_equal_within_offset_range(actual: f64, expectation: f64, offset: f64) -> bool {
    assert!(offset >= 0., "offset must be positive");
    actual >= expectation - offset && actual <= expectation + offset
}

// TODO: test coverage
