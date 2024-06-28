use super::{
    nearest_response::NearestResponse, osm::OSMNode, osm_db::OSMDb, route_response::RouteResponse,
};
use crate::{common::local_task::LocalTask, Location};
use core::panic;
use cucumber::World;
use log::debug;
use std::{
    collections::HashMap,
    fs::{create_dir_all, File},
    io::Write,
    path::PathBuf,
    time::Duration,
};

const DEFAULT_ORIGIN: Location = Location {
    longitude: 1.0f32,
    latitude: 1.0f32,
};
const DEFAULT_GRID_SIZE: f32 = 100.;
const WAY_SPACING: f32 = 100.;

#[derive(Debug, World)]
pub struct OSRMWorld {
    pub feature_path: Option<PathBuf>,
    pub scenario_id: String,
    pub feature_digest: String,
    pub osrm_digest: String,
    pub osm_id: u64,
    pub profile: String,

    pub known_osm_nodes: HashMap<char, Location>,
    pub known_locations: HashMap<char, Location>,

    pub osm_db: OSMDb,
    pub extraction_parameters: Vec<String>,

    pub request_with_flatbuffers: bool,
    pub query_options: HashMap<String, String>,

    pub grid_size: f32,
    pub origin: Location,
    pub way_spacing: f32,

    task: LocalTask,
    agent: ureq::Agent,
}

impl Default for OSRMWorld {
    fn default() -> Self {
        Self {
            feature_path: Default::default(),
            scenario_id: Default::default(),
            feature_digest: Default::default(),
            osrm_digest: Default::default(),
            osm_id: Default::default(),
            profile: Default::default(),
            known_osm_nodes: Default::default(),
            known_locations: Default::default(),
            osm_db: Default::default(),
            extraction_parameters: Default::default(),
            request_with_flatbuffers: Default::default(),
            query_options: HashMap::from([
                // default parameters // TODO: check if necessary
                ("steps".into(), "true".into()),
                ("alternatives".into(), "false".into()),
                ("annotations".into(), "true".into()),
            ]),

            grid_size: DEFAULT_GRID_SIZE,
            origin: DEFAULT_ORIGIN,
            way_spacing: WAY_SPACING,
            task: LocalTask::default(),
            agent: ureq::AgentBuilder::new()
                .timeout_read(Duration::from_secs(5))
                .timeout_write(Duration::from_secs(5))
                .build(),
        }
    }
}

impl OSRMWorld {
    pub fn feature_cache_path(&self) -> PathBuf {
        let full_path = self.feature_path.clone().unwrap();
        let path = full_path
            .ancestors()
            .find(|p| p.ends_with("features"))
            .expect(".feature files reside in a directory tree with the root name 'features'");

        let suffix = full_path.strip_prefix(path).unwrap();
        let path = path.parent().unwrap();
        debug!("suffix: {suffix:?}");
        let cache_path = path
            .join("test")
            .join("cache")
            .join(suffix)
            .join(&self.feature_digest);

        debug!("{cache_path:?}");
        if !cache_path.exists() {
            create_dir_all(&cache_path).expect("cache path could not be created");
        } else {
            debug!("not creating cache dir");
        }
        cache_path
    }

    pub fn routed_path(&self) -> PathBuf {
        let full_path = self.feature_path.clone().unwrap();
        let path = full_path
            .ancestors()
            .find(|p| p.ends_with("features"))
            .expect(".feature files reside in a directory tree with the root name 'features'");
        let routed_path = path
            .parent()
            .expect("cannot get parent path")
            .join("build")
            .join("osrm-routed");
        assert!(routed_path.exists(), "osrm-routed binary not found");
        routed_path
    }

    pub fn set_scenario_specific_paths_and_digests(&mut self, path: Option<PathBuf>) {
        self.feature_path.clone_from(&path);

        let file = File::open(path.clone().unwrap())
            .unwrap_or_else(|_| panic!("filesystem broken? can't open file {:?}", path));
        self.feature_digest = chksum_md5::chksum(file)
            .expect("md5 could not be computed")
            .to_hex_lowercase();
    }

    pub fn make_osm_id(&mut self) -> u64 {
        // number implicitly starts a 1. This is in line with previous implementations
        self.osm_id += 1;
        self.osm_id
    }

    pub fn add_osm_node(&mut self, name: char, location: Location, id: Option<u64>) {
        if self.known_osm_nodes.contains_key(&name) {
            panic!("duplicate node: {name}");
        }
        let id = match id {
            Some(id) => id,
            None => self.make_osm_id(),
        };
        let node = OSMNode {
            id,
            location,
            tags: HashMap::from([("name".to_string(), name.to_string())]),
        };

        self.known_osm_nodes.insert(name, location);
        self.osm_db.add_node(node);
    }

    pub fn get_location(&self, name: char) -> Location {
        *match name {
            // TODO: move lookup to world
            '0'..='9' => self
                .known_locations
                .get(&name)
                .expect("test case specifies unknown location: {name}"),
            'a'..='z' => self
                .known_osm_nodes
                .get(&name)
                .expect("test case specifies unknown osm node: {name}"),
            _ => unreachable!("nodes have to be name in [0-9][a-z]"),
        }
    }

    pub fn add_location(&mut self, name: char, location: Location) {
        if self.known_locations.contains_key(&name) {
            panic!("duplicate location: {name}")
        }
        self.known_locations.insert(name, location);
    }

    pub fn write_osm_file(&self) {
        let osm_file = self
            .feature_cache_path()
            .join(self.scenario_id.clone() + ".osm");
        if !osm_file.exists() {
            debug!("writing to osm file: {osm_file:?}");
            let mut file = File::create(osm_file).expect("could not create OSM file");
            file.write_all(self.osm_db.to_xml().as_bytes())
                .expect("could not write OSM file");
        } else {
            debug!("not writing to OSM file {osm_file:?}");
        }
    }

    pub fn extract_osm_file(&self) {
        let cache_path = self.artefact_cache_path();
        if cache_path.exists() {
            debug!("{cache_path:?} exists");
        } else {
            unimplemented!("{cache_path:?} does not exist");
        }
    }

    pub fn artefact_cache_path(&self) -> PathBuf {
        self.feature_cache_path().join(&self.osrm_digest)
    }

    fn start_routed(&mut self) {
        if self.task.is_ready() {
            // task running already
            return;
        }
        let data_path = self
            .artefact_cache_path()
            .join(self.scenario_id.to_owned() + ".osrm");

        self.task = LocalTask::new(self.routed_path().to_string_lossy().into())
            .arg(data_path.to_str().expect("data path unwrappable"));
        self.task
            .spawn_wait_till_ready("running and waiting for requests");
        assert!(self.task.is_ready());
    }

    pub fn nearest(
        &mut self,
        query_location: &Location,
        // request_with_flatbuffers: bool,
    ) -> NearestResponse {
        self.start_routed();

        let mut url = format!(
            "http://localhost:5000/nearest/v1/{}/{:?},{:?}",
            self.profile, query_location.longitude, query_location.latitude
        );
        if self.request_with_flatbuffers {
            url += ".flatbuffers";
        }

        if !self.query_options.is_empty() {
            let options = self
                .query_options
                .iter()
                .map(|(key, value)| format!("{key}={value}"))
                .collect::<Vec<String>>()
                .join("&");
            url += "?";
            url += &options;
        }

        // panic!("url: {url}");
        let call = self.agent.get(&url).call();

        let body = match call {
            Ok(response) => response.into_reader(),
            Err(e) => panic!("http error: {e}"),
        };

        let response = match self.request_with_flatbuffers {
            true => NearestResponse::from_flatbuffer(body),
            false => NearestResponse::from_json_reader(body),
        };
        response
    }

    pub fn route(&mut self, waypoints: &[Location]) -> RouteResponse {
        self.start_routed();

        let waypoint_string = waypoints
            .iter()
            .map(|location| format!("{:?},{:?}", location.longitude, location.latitude))
            .collect::<Vec<String>>()
            .join(";");

        let mut url = format!(
            "http://localhost:5000/route/v1/{}/{waypoint_string}",
            self.profile,
        );
        if self.request_with_flatbuffers {
            url += ".flatbuffers";
        }
        if !self.query_options.is_empty() {
            let options = self
                .query_options
                .iter()
                .map(|(key, value)| format!("{key}={value}"))
                .collect::<Vec<String>>()
                .join("&");
            url += "?";
            url += &options;
        }
        // println!("url: {url}");
        let call = self.agent.get(&url).call();

        let body = match call {
            Ok(response) => response.into_reader(),
            Err(_e) => return RouteResponse::default(),
        };

        let text = std::io::read_to_string(body).unwrap();
        let response = match self.request_with_flatbuffers {
            true => unimplemented!("RouteResponse::from_flatbuffer(body)"),
            false => RouteResponse::from_string(&text),
        };
        response
    }
}
