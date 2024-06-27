use serde::Deserialize;

use super::{location::Location, nearest_response::Waypoint};

#[derive(Deserialize, Default, Debug)]
pub struct Maneuver {
    pub bearing_after: f64,
    pub bearing_before: f64,
    pub location: Location,
    pub modifier: Option<String>, // TODO: should be an enum
    pub r#type: String,           // TODO: should be an enum
}

#[derive(Deserialize, Default, Debug)]
pub struct Step {
    pub geometry: String,
    pub mode: String,
    pub maneuver: Maneuver,
    pub name: String,
    pub pronunciation: Option<String>,
    pub r#ref: Option<String>,
    pub duration: f64,
    pub distance: f64,
}

// #[derive(Deserialize, Debug)]
// pub struct Annotation {
//     pub nodes: Option<Vec<u64>>,
// }

#[derive(Debug, Default, Deserialize)]
pub struct Leg {
    pub summary: String,
    pub weight: f64,
    pub duration: f64,
    pub steps: Vec<Step>,
    pub distance: f64,
    // pub annotation: Option<Vec<Annotation>>,
}

#[derive(Deserialize, Debug)]
pub struct Route {
    pub geometry: String,
    pub weight: f64,
    pub duration: f64,
    pub legs: Vec<Leg>,
    pub weight_name: String,
    pub distance: f64,
}

#[derive(Debug, Default, Deserialize)]
pub struct RouteResponse {
    pub code: String,
    pub routes: Vec<Route>,
    pub waypoints: Vec<Waypoint>,
    pub data_version: Option<String>,
}

impl RouteResponse {
    pub fn from_json_reader(reader: impl std::io::Read) -> Self {
        let response = match serde_json::from_reader::<_, Self>(reader) {
            Ok(response) => response,
            Err(e) => panic!("parsing error {e}"),
        };
        response
    }

    pub fn from_string(input: &str) -> Self {
        // println!("{input}");
        let response = match serde_json::from_str(input) {
            Ok(response) => response,
            Err(e) => panic!("parsing error {e} => {input}"),
        };
        response
    }
}
