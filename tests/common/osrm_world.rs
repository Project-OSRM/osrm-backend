use crate::Point;
use cucumber::World;
use log::debug;
use std::{
    collections::HashMap,
    fs::{create_dir_all, File},
    path::PathBuf,
};

use super::{osm::OSMNode, osm_db::OSMDb};

#[derive(Debug, Default, World)]
pub struct OSRMWorld {
    pub feature_path: Option<PathBuf>,
    pub scenario_id: String,
    pub feature_digest: String,
    pub osrm_digest: String,
    pub osm_id: u64,
    pub profile: String,

    pub known_osm_nodes: HashMap<char, Point>,
    pub known_locations: HashMap<char, Point>,

    pub osm_db: OSMDb,
    pub extraction_parameters: Vec<String>,
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
        let routed_path = path.parent().expect("cannot get parent path").join("build").join("osrm-routed");
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

    pub fn add_osm_node(&mut self, name: char, location: Point, id: Option<u64>) {
        if self.known_osm_nodes.contains_key(&name) {
            panic!("duplicate node: {name}");
        }
        let id = match id {
            Some(id) => id,
            None => self.make_osm_id(),
        };
        let node = OSMNode {
            id,
            lat: location.y(),
            lon: location.x(),
            tags: HashMap::from([("name".to_string(), name.to_string())]),
        };

        self.known_osm_nodes.insert(name, location);
        self.osm_db.add_node(node);
    }

    pub fn get_location(&self, name: char) -> Point {
        match name {
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
        .clone()
    }

    pub fn add_location(&mut self, name: char, location: Point) {
        if self.known_locations.contains_key(&name) {
            panic!("duplicate location: {name}")
        }
        self.known_locations.insert(name, location);
    }
}
