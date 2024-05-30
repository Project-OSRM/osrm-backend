mod osm;

use core::panic;
use std::collections::{HashMap, HashSet, VecDeque};
use std::fs::{create_dir_all, DirEntry, File};
use std::io::{self, Read, Write};
use std::path::PathBuf;
use std::{env, fs};

use cheap_ruler::CheapRuler;
use cucumber::{self, gherkin::Step, given, when, World};
use futures::{future, FutureExt};
use geo_types::{point, Point};
use osm::{OSMDb, OSMNode, OSMWay};
use walkdir::WalkDir;

#[derive(Debug, Default, World)]
struct OSRMWorld {
    feature_path: Option<PathBuf>,
    scenario_id: String,
    feature_digest: String,
    osrm_digest: String,
    osm_id: u64,
    profile: String,

    known_osm_nodes: HashSet<char>,
    known_locations: HashMap<char, Point>,

    osm_db: OSMDb,
}

impl OSRMWorld {
    fn set_scenario_specific_paths_and_digests(&mut self, path: Option<PathBuf>) {
        self.feature_path = path.clone();

        let file = File::open(path.clone().unwrap())
            .expect(&format!("filesystem broken? can't open file {:?}", path));
        self.feature_digest = chksum_md5::chksum(file)
            .expect("md5 could not be computed")
            .to_hex_lowercase();
    }

    fn make_osm_id(&mut self) -> u64 {
        // number implicitly starts a 1. This is in line with previous implementations
        self.osm_id += 1;
        self.osm_id
    }

    fn add_osm_node(&mut self, name: char, location: Point, id: Option<u64>) {
        if self.known_osm_nodes.contains(&name) {
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

        self.known_osm_nodes.insert(name);
        self.osm_db.add_node(node);
    }

    fn add_location(&mut self, name: char, location: Point) {
        if self.known_locations.contains_key(&name) {
            panic!("duplicate location: {name}")
        }
        self.known_locations.insert(name, location);
    }
}

const DEFAULT_ORIGIN: [f64; 2] = [1., 1.]; // TODO: move to world?
const DEFAULT_GRID_SIZE: f64 = 100.; // TODO: move to world?

fn offset_origin_by(dx: f64, dy: f64) -> geo_types::Point {
    let ruler = CheapRuler::new(DEFAULT_ORIGIN[1], cheap_ruler::DistanceUnit::Meters);
    ruler.offset(
        &point!(DEFAULT_ORIGIN),
        dx * DEFAULT_GRID_SIZE,
        dy * DEFAULT_GRID_SIZE,
    ) //TODO: needs to be world's gridSize, not the local one
}

#[given(expr = "the profile \"{word}\"")]
fn set_profile(world: &mut OSRMWorld, profile: String) {
    println!(
        "using profile: {profile} on scenario: {}",
        world.scenario_id
    );
    world.profile = profile;
}
#[given(expr = "the node map")]
fn set_node_map(world: &mut OSRMWorld, step: &Step) {
    if let Some(docstring) = step.docstring() {
        // TODO: refactor into a function
        docstring
            .split('\n')
            .enumerate()
            .for_each(|(row_index, row)| {
                row.chars()
                    .enumerate()
                    .filter(|(_column_index, charater)| *charater != ' ')
                    .for_each(|(column_index, name)| {
                        // This ports the logic from previous implementations.
                        let location =
                            offset_origin_by(column_index as f64 * 0.5, -(row_index as f64 - 1.));
                        match name {
                            '0'...'9' => world.add_location(name, location),
                            'a'...'z' => world.add_osm_node(name, location, None),
                            _ => unreachable!("node name not in [0..9][a..z]"),
                        }
                    });
            });
    } else {
        panic!("node map not found");
    }
}

#[given(regex = "the ways")]
fn set_ways(world: &mut OSRMWorld, step: &Step) {
    // println!("using profile: {profile}");
    if let Some(table) = step.table.as_ref() {
        if table.rows.is_empty() {
            panic!("empty way table provided")
        }
        // store a reference to the headers for convenient lookup
        let headers = table.rows.first().unwrap();

        // iterate over the following rows and build ways one by one
        table.rows.iter().skip(1).for_each(|row| {
            let mut way = OSMWay {
                id: world.make_osm_id(),
                ..Default::default()
            };
            way.tags.insert("highway".into(), "primary".into()); // default may get overwritten below
            row.iter().enumerate().for_each(|(column_index, token)| {
                let header = headers[column_index].as_str();
                if header == "nodes" {
                    assert!(
                        token.len() >= 2,
                        "ways must be defined by token of at least length two giving"
                    );
                    way.tags.insert("name".into(), token.clone());
                    token.chars().for_each(|name| {
                        if !world.known_osm_nodes.contains(&name) {
                            // TODO: this check is probably not necessary since it is also checked below
                            panic!("referenced unknown node {name} in way {token}");
                        }
                        if let Some((_, node)) = world.osm_db.find_node(name) {
                            way.add_node(node.clone());
                        } else {
                            panic!("node is known, but not found in osm_db");
                        }
                    })
                } else if !token.is_empty() {
                    way.tags.insert(header.into(), token.clone());
                }
            });
            world.osm_db.add_way(way);
        });
    } else {
        println!("no table found {step:#?}");
    }

    // println!("{}", world.osm_db.to_xml())
}

#[when("I request nearest I should get")]
fn request_nearest(world: &mut OSRMWorld, step: &Step) {
    // if .osm file does not exist
    //    write osm file
    // TODO: move to cache_file/path(.) function in OSRMWorld
    let full_path = world.feature_path.clone().unwrap();
    let path = full_path
        .ancestors()
        .find(|p| p.ends_with("features"))
        .expect(".feature files reside in a directory tree with the root name 'features'");

    let suffix = full_path.strip_prefix(path).unwrap();
    let path = path.parent().unwrap();
    println!("suffix: {suffix:?}");
    let cache_path = path
        .join("test")
        .join("cache")
        .join(suffix)
        .join(&world.feature_digest);

    println!("{cache_path:?}");
    if !cache_path.exists() {
        create_dir_all(&cache_path).expect("cache path could not be created");
    } else {
        println!("not creating cache dir");
    }

    let osm_file = cache_path.join(world.scenario_id.clone() + ".osm");
    if !osm_file.exists() {
        println!("writing to osm file: {osm_file:?}");
        let mut file = File::create(osm_file).expect("could not create OSM file");
        file.write_all(world.osm_db.to_xml().as_bytes())
            .expect("could not write OSM file");
    } else {
        println!("not writing to OSM file {osm_file:?}");
    }

    // if extracted file does not exist
    //    extract osm file (partition, preprocess)

    // parse table from Step and build query list
    // run queries (in parallel) and validate results
    todo!("nearest {step:?}");
}

// TODO: move to different file
fn get_file_as_byte_vec(path: &PathBuf) -> Vec<u8> {
    println!("opening {path:?}");
    let mut f = File::open(&path).expect("no file found");
    let metadata = fs::metadata(&path).expect("unable to read metadata");
    let mut buffer = vec![0; metadata.len() as usize];
    f.read(&mut buffer).expect("buffer overflow");

    buffer
}

// TODO: port into toolbox-rs
struct LexicographicFileWalker {
    dirs: VecDeque<PathBuf>,
    files: VecDeque<PathBuf>,
}

impl LexicographicFileWalker {
    pub fn new(path: &PathBuf) -> Self {
        let mut dirs = VecDeque::new();

        if path.is_dir() {
            dirs.push_back(path.clone());
        }

        Self {
            dirs,
            files: VecDeque::new(),
        }
    }
}

impl Iterator for LexicographicFileWalker {
    type Item = PathBuf;

    fn next(&mut self) -> Option<Self::Item> {
        if self.dirs.is_empty() && self.files.is_empty() {
            return None;
        }
        while self.files.is_empty() && !self.dirs.is_empty() {
            assert!(!self.dirs.is_empty());
            let current_dir = self.dirs.pop_front().unwrap();
            let mut temp_dirs = Vec::new();

            for entry in fs::read_dir(current_dir).unwrap() {
                let entry = entry.unwrap();
                let path = entry.path();
                if path.is_dir() {
                    temp_dirs.push(path.clone());
                } else {
                    self.files.push_back(path.clone());
                }
            }
            self.files.make_contiguous().sort();
            temp_dirs.sort();
            self.dirs.extend(temp_dirs.into_iter());
        }
        return self.files.pop_front();
    }
}

fn main() {
    // create OSRM digest before any tests are executed since cucumber-rs doesn't have @beforeAll
    let exe_path = env::current_exe().expect("failed to get current exe path");
    let path = exe_path
    .ancestors()
    .find(|p| p.ends_with("target"))
    .expect("compiled cucumber test executable resides in a directory tree with the root name 'target'")
    .parent().unwrap(); // TODO: Remove after migration to Rust build dir
    let build_path = path.join("build"); // TODO: Remove after migration to Rust build dir
    let mut dependencies = Vec::new();

    // FIXME: the following iterator gymnastics port the exact behavior of the JavaScript implementation
    let names = [
        "osrm-extract",
        "osrm-contract",
        "osrm-customize",
        "osrm-partition",
        "osrm_extract",
        "osrm_contract",
        "osrm_customize",
        "osrm_partition",
    ];

    let files: Vec<PathBuf> = fs::read_dir(&build_path)
        .unwrap()
        .into_iter()
        .filter_map(|e| e.ok())
        .map(|dir_entry| dir_entry.path())
        .collect();

    let iter = names.iter().map(|name| {
        files
            .iter()
            .find(|path_buf| {
                path_buf
                    .file_stem()
                    .unwrap()
                    .to_str()
                    .unwrap()
                    .contains(name)
            })
            .map(|e| e.clone())
            .expect("file exists and is usable")
    });

    dependencies.extend(iter);

    let profiles_path = path.join("profiles");
    println!("{profiles_path:?}");

    dependencies.extend(
        LexicographicFileWalker::new(&profiles_path)
            .filter(|pb| !pb.to_str().unwrap().contains("examples"))
            .filter(|pathbuf| match pathbuf.extension() {
                Some(ext) => ext.to_str().unwrap() == "lua",
                None => false,
            }),
    );
    let mut md5 = chksum_md5::new();
    println!("md5: {}", md5.digest().to_hex_lowercase());

    for path_buf in dependencies {
        let data = get_file_as_byte_vec(&path_buf);
        if data.len() == 0 {
            continue;
        }
        md5.update(data);
        // println!("md5: {}", md5.digest().to_hex_lowercase());
    }

    futures::executor::block_on(
        OSRMWorld::cucumber()
            .before(move |_feature, _rule, scenario, world| {
                // TODO: move to function call below
                // ports the following logic:
                // let name = scenario.getName().toLowerCase().replace(/[/\-'=,():*#]/g, '')
                // .replace(/\s/g, '_').replace(/__/g, '_').replace(/\.\./g, '.')
                // .substring(0, 64);
                let mut s = scenario
                    .name
                    .to_ascii_lowercase()
                    .replace(
                        &['/', '\\', '-', '\'', '=', ',', '(', ')', ':', '*', '#'][..],
                        "",
                    )
                    .chars()
                    .map(|x| match x {
                        ' ' => '_',
                        _ => x,
                    })
                    .collect::<String>()
                    .replace(r"\", "_")
                    .replace("__", "_")
                    .replace("..", ".");
                s.truncate(64);

                world.scenario_id = format!("{}_{}", scenario.position.line, s);
                world.set_scenario_specific_paths_and_digests(_feature.path.clone());
                world.osrm_digest = md5.digest().to_hex_lowercase();

                // TODO: clean up cache if needed

                future::ready(()).boxed()
            })
            .run_and_exit("features/nearest/pick.feature"),
    );
}
