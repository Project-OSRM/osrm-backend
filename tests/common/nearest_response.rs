use geo_types::{point, Point};
use serde::Deserialize;

#[derive(Deserialize, Debug)]
pub struct Waypoint {
    hint: String,
    nodes: Vec<u64>,
    distance: f64,
    name: String,
    location: [f64; 2],
}

impl Waypoint {
    pub fn location(&self) -> Point {
        point!(self.location)
    }
}

#[derive(Deserialize, Debug)]
pub struct NearestResponse {
    code: String,
    pub waypoints: Vec<Waypoint>,
}
