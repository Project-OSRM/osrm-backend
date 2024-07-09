#[derive(Debug)]
pub enum Offset {
    Absolute(f64),
    Percentage(f64),
}

// #[cfg(test)]
// mod tests {
//     use crate::extract_number_and_offset;

//     #[test]
//     fn extract_number_and_offset() {
//         let (value, result) = extract_number_and_offset("m", "300 +- 1m");
//         assert_eq!(value, 300.);
//         assert_eq!(offset, 1);
//     }
// }
