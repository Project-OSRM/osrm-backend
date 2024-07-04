use serde::Deserialize;

#[derive(Clone, Copy, Debug, Default, Deserialize)]
pub struct Location {
    // Note: The order is important since we derive Deserialize
    pub longitude: f32,
    pub latitude: f32,
}