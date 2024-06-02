extern crate clap;

mod common;

use crate::common::osrm_world::OSRMWorld;
use cheap_ruler::CheapRuler;
use clap::Parser;
use common::cli_arguments::Args;
use common::lexicographic_file_walker::LexicographicFileWalker;
use common::nearest_response::NearestResponse;
use common::osm::OSMWay;
use common::task_starter::TaskStarter;
use core::panic;
use cucumber::{self, gherkin::Step, given, when, World};
use futures::{future, FutureExt};
use geo_types::{point, Point};
use std::fs::{create_dir_all, File};
use std::io::{Read, Write};
use std::path::PathBuf;
use std::time::Duration;
use std::{env, fs};
use ureq::Agent;

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
                        if !world.known_osm_nodes.contains_key(&name) {
                            // TODO: this check is probably not necessary since it is also checked below implicitly
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
    let cache_path = cache_path.join(&world.osrm_digest);
    if cache_path.exists() {
        println!("{cache_path:?} exists");
    } else {
        println!("{cache_path:?} does not exist");
    }

    // parse query data
    let t = &step.table.as_ref().expect("no query table specified");
    let test_cases: Vec<_> = t
        .rows
        .iter()
        .skip(1)
        .map(|row| {
            assert_eq!(
                row.len(),
                2,
                "test case broken: row needs to have two entries"
            );
            let query = row.get(0).unwrap();
            let expected = row.get(1).unwrap();
            assert_eq!(query.len(), 1);
            assert_eq!(expected.len(), 1);
            (
                query.chars().next().unwrap(),
                expected.chars().next().unwrap(),
            )
        })
        .collect();

    let data_path = cache_path.join(world.scenario_id.to_owned() + ".osrm");

    let routed_path = path.join("build").join("osrm-routed");
    if !routed_path.exists() {
        panic!("osrm-routed binary not found");
    }

    // TODO: this should not require a temporary and behave like the API of std::process
    let mut task = TaskStarter::new(routed_path.to_str().unwrap());
    task.arg(data_path.to_str().unwrap());
    task.spawn_wait_till_ready("running and waiting for requests");
    assert!(task.is_ready());

    // TODO: move to generic http request handling struct
    let agent: Agent = ureq::AgentBuilder::new()
        .timeout_read(Duration::from_secs(5))
        .timeout_write(Duration::from_secs(5))
        .build();

    // parse and run test cases
    for (query, expected) in test_cases {
        let query_location = world.get_location(query);
        let expected_location = world.get_location(expected);

        // println!("{query_location:?} => {expected_location:?}");
        // run queries
        let url = format!(
            "http://localhost:5000/nearest/v1/{}/{},{}",
            world.profile,
            query_location.x(),
            query_location.y()
        );
        let call = agent.get(&url).call();

        let body = match call {
            Ok(response) => response.into_string().expect("response not parseable"),
            Err(e) => panic!("http error: {e}"),
        };
        // println!("body: {body}");

        let v: NearestResponse = match serde_json::from_str(&body) {
            Ok(v) => v,
            Err(e) => panic!("parsing error {e}"),
        };

        let result_location = v.waypoints[0].location();
        // check results
        assert!(approx_equal(result_location.x(), expected_location.x(), 5));
        assert!(approx_equal(result_location.y(), expected_location.y(), 5));
    }

    // if let Err(e) = child.kill() {
    //     panic!("shutdown failed: {e}");
    // }
}

pub fn approx_equal(a: f64, b: f64, dp: u8) -> bool {
    let p = 10f64.powi(-(dp as i32));
    (a - b).abs() < p
}

// TODO: move to different file
fn get_file_as_byte_vec(path: &PathBuf) -> Vec<u8> {
    println!("opening {path:?}");
    let mut f = File::open(path).expect("no file found");
    let metadata = fs::metadata(path).expect("unable to read metadata");
    let mut buffer = vec![0; metadata.len() as usize];
    match f.read(&mut buffer) {
        Ok(l) => assert_eq!(metadata.len() as usize, l, "data was not completely read"),
        Err(e) => panic!("Error: {e}"),
    }

    buffer
}

fn main() {
    let args = Args::parse();
    println!("name: {:?}", args);

    // create OSRM digest before any tests are executed since cucumber-rs doesn't have @beforeAll
    let exe_path = env::current_exe().expect("failed to get current exe path");
    let path = exe_path
    .ancestors()
    .find(|p| p.ends_with("target"))
    .expect("compiled cucumber test executable resides in a directory tree with the root name 'target'")
    .parent().unwrap(); // TODO: Remove after migration to Rust build dir
    let build_path = path.join("build"); // TODO: Remove after migration to Rust build dir
    let mut dependencies = Vec::new();

    // FIXME: the following iterator gymnastics port the exact order and behavior of the JavaScript implementation
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

    let files: Vec<PathBuf> = fs::read_dir(build_path)
        .unwrap()
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
            .cloned()
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
        if data.is_empty() {
            continue;
        }
        md5.update(data);
        // println!("md5: {}", md5.digest().to_hex_lowercase());
    }

    futures::executor::block_on(
        OSRMWorld::cucumber()
            .max_concurrent_scenarios(1)
            .before(move |feature, _rule, scenario, world| {
                world.scenario_id = common::scenario_id::scenario_id(scenario);
                world.set_scenario_specific_paths_and_digests(feature.path.clone());
                world.osrm_digest = md5.digest().to_hex_lowercase();

                // TODO: clean up cache if needed

                future::ready(()).boxed()
            })
            .run_and_exit("features/nearest/pick.feature"),
    );
}
