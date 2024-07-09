use super::location::Location;
use crate::common::fbresult_flatbuffers::osrm::engine::api::fbresult::FBResult;
use serde::Deserialize;

#[derive(Deserialize, Debug)]
pub struct Waypoint {
    pub hint: String,
    pub nodes: Option<Vec<u64>>,
    pub distance: f32,
    pub name: String,
    location: Location,
}

impl Waypoint {
    pub fn location(&self) -> &Location {
        &self.location
    }
}

#[derive(Deserialize, Debug)]
pub struct NearestResponse {
    pub code: String,
    pub waypoints: Vec<Waypoint>,
    pub data_version: Option<String>,
}

impl NearestResponse {
    // TODO: the from_* functions should be a) into_. or b) implement From<_> trait
    pub fn from_json_reader(reader: impl std::io::Read) -> Self {
        let response = match serde_json::from_reader::<_, NearestResponse>(reader) {
            Ok(response) => response,
            Err(e) => panic!("parsing error {e}"),
        };
        response
    }

    pub fn from_flatbuffer(mut reader: impl std::io::Read) -> Self {
        let mut buffer = Vec::new();
        if let Err(e) = reader.read_to_end(&mut buffer) {
            panic!("cannot read from strem: {e}");
        };
        let decoded: Result<FBResult, flatbuffers::InvalidFlatbuffer> =
            flatbuffers::root::<FBResult>(&buffer);
        let decoded: FBResult = match decoded {
            Ok(d) => d,
            Err(e) => panic!("Error during parsing: {e} {:?}", buffer),
        };
        let code = match decoded.code() {
            Some(e) => e.message().expect("code exists but is not unwrappable"),
            None => "",
        };
        let data_version = match decoded.data_version() {
            Some(s) => s,
            None => "",
        };

        let waypoints = decoded
            .waypoints()
            .expect("waypoints should be at least an empty list")
            .iter()
            .map(|wp| {
                let hint = wp.hint().expect("hint is missing").to_string();
                let location = wp.location().expect("waypoint must have a location");
                let location = Location {
                    latitude: location.latitude() as f64,
                    longitude: location.longitude() as f64,
                };
                let nodes = wp.nodes().expect("waypoint mus have nodes");
                let nodes = Some(vec![nodes.first(), nodes.second()]);
                let distance = wp.distance();

                Waypoint {
                    hint,
                    nodes,
                    distance,
                    name: "".into(),
                    location,
                }
            })
            .collect();

        Self {
            code: code.into(),
            waypoints,
            data_version: Some(data_version.into()),
        }
    }
}
