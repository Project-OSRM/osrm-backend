use std::default;

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

#[derive(Deserialize, Default, Debug)]
pub struct Step {
    pub geometry: Geometry,
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
        let response = match serde_json::from_str(input) {
            Ok(response) => response,
            Err(e) => panic!("parsing error {e} => {input}"),
        };
        response
    }
}

// #[cfg(test)]
// mod tests {
//     use super::RouteResponse;

//     #[test]
//     fn parse_geojson() {
//         let input = r#"{"code":"Ok","routes":[{"geometry":{"coordinates":[[1.00009,1],[1.000269,1]],"type":"LineString"},"weight":1.9,"duration":1.9,"legs":[{"annotation":{"speed":[10.5],"weight":[1.9],"nodes":[1,2],"duration":[1.9],"distance":[19.92332315]},"summary":"abc","weight":1.9,"duration":1.9,"steps":[{"geometry":{"coordinates":[[1.00009,1],[1.000269,1]],"type":"LineString"},"maneuver":{"location":[1.00009,1],"bearing_after":90,"bearing_before":0,"modifier":"right","type":"depart"},"mode":"driving","name":"abc","intersections":[{"out":0,"entry":[true],"bearings":[90],"location":[1.00009,1]}],"driving_side":"right","weight":1.9,"duration":1.9,"distance":19.9},{"geometry":{"coordinates":[[1.000269,1],[1.000269,1]],"type":"LineString"},"maneuver":{"location":[1.000269,1],"bearing_after":0,"bearing_before":90,"modifier":"right","type":"arrive"},"mode":"driving","name":"abc","intersections":[{"in":0,"entry":[true],"bearings":[270],"location":[1.000269,1]}],"driving_side":"right","weight":0,"duration":0,"distance":0}],"distance":19.9}],"weight_name":"duration","distance":19.9}],"waypoints":[{"name":"abc","hint":"AAAAgAEAAIAKAAAAHgAAAAAAAAAoAAAA6kYgQWyG70EAAAAA6kYgQgoAAAAeAAAAAAAAACgAAAABAACAmkIPAEBCDwCaQg8Ai0EPAAAArwUAAAAA","distance":20.01400211,"location":[1.00009,1]},{"name":"abc","hint":"AAAAgAEAAIAdAAAACwAAAAAAAAAoAAAAbIbvQepGIEEAAAAA6kYgQh0AAAALAAAAAAAAACgAAAABAACATUMPAEBCDwBNQw8Ai0EPAAAArwUAAAAA","distance":20.01400211,"location":[1.000269,1]}]} "#;
//         let result = RouteResponse::from_string(&input);

//     }
// }
