use geo_types::{point, Point};
use serde::Deserialize;

#[derive(Deserialize, Debug)]
pub struct Waypoint {
    pub hint: String,
    pub nodes: Vec<u64>,
    pub distance: f64,
    pub name: String,
    location: [f64; 2],
}

impl Waypoint {
    pub fn location(&self) -> Point {
        point!(self.location)
    }
}

#[derive(Deserialize, Debug)]
pub struct NearestResponse {
    pub code: String,
    pub waypoints: Vec<Waypoint>,
    pub data_version: Option<String>,
}
