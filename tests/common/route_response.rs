use serde::Deserialize;

use super::{location::Location, nearest_response::Waypoint};

#[derive(Deserialize, Default, Debug)]
pub struct Maneuver {
    pub bearing_after: u64,
    pub bearing_before: u64,
    pub location: Location,
    pub modifier: Option<String>, // TODO: should be an enum
    pub r#type: String,           // TODO: should be an enum
    pub exit: Option<u64>,
}

#[derive(Debug, Clone, Deserialize)]
#[serde(untagged)]
pub enum Geometry {
    A(String),
    B {
        coordinates: Vec<Location>,
        r#type: String,
    },
}

impl Default for Geometry {
    fn default() -> Self {
        Geometry::A("".to_string())
    }
}

#[derive(Debug, Default, Clone, Deserialize)]
pub struct Intersection {
    pub r#in: Option<u64>,
    pub out: Option<u64>,
    pub entry: Vec<bool>,
    pub bearings: Vec<u64>,
    pub location: Location,
    pub classes: Option<Vec<String>>,
}

#[derive(Deserialize, Default, Debug)]
pub struct Step {
    pub geometry: Geometry,
    pub mode: String,
    pub maneuver: Maneuver,
    pub name: String,
    pub pronunciation: Option<String>,
    pub rotary_name: Option<String>,
    pub r#ref: Option<String>,
    pub duration: f64,
    pub distance: f64,
    pub intersections: Vec<Intersection>,
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

#[derive(Deserialize, Debug, Default)]
pub struct Route {
    pub geometry: Geometry,
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
    pub waypoints: Option<Vec<Waypoint>>,
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
