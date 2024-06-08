// extern crate clap;

mod common;

use cheap_ruler::CheapRuler;
use clap::Parser;
use common::{
    cli_arguments::Args, dot_writer::DotWriter, f64_utils::approx_equal,
    hash_util::md5_of_osrm_executables, location::Location, nearest_response::NearestResponse,
    osm::OSMWay, osrm_world::OSRMWorld, task_starter::TaskStarter,
};
use core::panic;
use cucumber::{gherkin::Step, given, when, World, WriterExt};
use futures::{future, FutureExt};
use geo_types::point;
use log::debug;
use std::{collections::HashMap, fs::File, io::Write, time::Duration};
use ureq::Agent;

const DEFAULT_ORIGIN: [f64; 2] = [1., 1.]; // TODO: move to world?
const DEFAULT_GRID_SIZE: f64 = 100.; // TODO: move to world?

fn offset_origin_by(dx: f64, dy: f64) -> Location {
    let ruler = CheapRuler::new(DEFAULT_ORIGIN[1], cheap_ruler::DistanceUnit::Meters);
    let loc = ruler.offset(
        &point!(DEFAULT_ORIGIN),
        dx * DEFAULT_GRID_SIZE,
        dy * DEFAULT_GRID_SIZE,
    ); //TODO: needs to be world's gridSize, not the local one
    Location {
        latitude: loc.y() as f32,
        longitude: loc.x() as f32,
    }
}

#[given(expr = "the profile \"{word}\"")]
fn set_profile(world: &mut OSRMWorld, profile: String) {
    debug!(
        "using profile: {profile} on scenario: {}",
        world.scenario_id
    );
    world.profile = profile;
}

#[given(expr = "the node locations")]
fn set_node_locations(world: &mut OSRMWorld, step: &Step) {
    let table = step.table().expect("cannot get table");
    let header = table.rows.first().expect("node locations table empty");
    assert_eq!(header.len(), 3, "header needs to define three columns");
    assert_eq!(
        header[0], "node",
        "first column needs to be 'node' indicating the one-letter name"
    );
    // the following lookup allows to define lat lon columns in any order
    let header_lookup: HashMap<&str, usize> = header
        .iter()
        .enumerate()
        .map(|(index, name)| (name.as_str(), index))
        .collect();
    ["lat", "lon"].iter().for_each(|dim| {
        assert!(
            header_lookup.contains_key(*dim),
            "table must define a {dim} column"
        );
    });

    table.rows.iter().skip(1).for_each(|row| {
        assert_eq!(3, row.len());
        assert_eq!(row[0].len(), 1, "node name not in [0..9][a..z]");
        let name = &row[0].chars().next().expect("node name cannot be empty"); // the error is unreachable
        let lon = &row[header_lookup["lon"]];
        let lat = &row[header_lookup["lat"]];
        let location = Location {
            latitude: lat.parse::<f32>().expect("lat {lat} needs to be a f64"),
            longitude: lon.parse::<f32>().expect("lon {lon} needs to be a f64"),
        };
        match name {
            '0'...'9' => world.add_location(*name, location),
            'a'...'z' => world.add_osm_node(*name, location, None),
            _ => unreachable!("node name not in [0..9][a..z]"),
        }
    });
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

#[given(expr = r#"the extract extra arguments {string}"#)]
fn extra_parameters(world: &mut OSRMWorld, parameters: String) {
    world.extraction_parameters.push(parameters);
}

#[given(regex = "the ways")]
fn set_ways(world: &mut OSRMWorld, step: &Step) {
    // debug!("using profile: {profile}");
    if let Some(table) = step.table.as_ref() {
        if table.rows.is_empty() {
            panic!("empty way table provided")
        }
        // store a reference to the headers for convenient lookup
        let headers = table.rows.first().expect("table has a first row");

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
        debug!("no table found {step:#?}");
    }

    // debug!("{}", world.osm_db.to_xml())
}

// #[when("I request nearest I should get")]
#[when(regex = r"^I request nearest( with flatbuffers|) I should get$")]
fn request_nearest(world: &mut OSRMWorld, step: &Step, state: String) {
    let request_with_flatbuffers = state == " with flatbuffers";
    // if .osm file does not exist
    //    write osm file

    // TODO: the OSRMWorld instance should have a function to write the .osm file
    let osm_file = world
        .feature_cache_path()
        .join(world.scenario_id.clone() + ".osm");
    if !osm_file.exists() {
        debug!("writing to osm file: {osm_file:?}");
        let mut file = File::create(osm_file).expect("could not create OSM file");
        file.write_all(world.osm_db.to_xml().as_bytes())
            .expect("could not write OSM file");
    } else {
        debug!("not writing to OSM file {osm_file:?}");
    }

    // if extracted file does not exist
    let cache_path = world.feature_cache_path().join(&world.osrm_digest);
    if cache_path.exists() {
        debug!("{cache_path:?} exists");
    } else {
        debug!("{cache_path:?} does not exist");
    }

    // parse query data
    let table = &step.table.as_ref().expect("no query table specified");
    // the following lookup allows to define lat lon columns in any order
    let header = table.rows.first().expect("node locations table empty");
    // TODO: move to common functionality
    let test_cases: Vec<_> = table
        .rows
        .iter()
        .skip(1)
        .map(|row| {
            let row_map: HashMap<String, String> = row
                .iter()
                .enumerate()
                .map(|(column_index, value)| {
                    let key = header[column_index].clone();
                    (key, value.clone())
                })
                .collect();
            row_map
        })
        .collect();

    let data_path = cache_path.join(world.scenario_id.to_owned() + ".osrm");

    // TODO: this should not require a temporary and behave like the API of std::process
    let mut task = TaskStarter::new(world.routed_path().to_str().expect("task can be started"));
    task.arg(data_path.to_str().expect("data path unwrappable"));
    task.spawn_wait_till_ready("running and waiting for requests");
    assert!(task.is_ready());

    // TODO: move to generic http request handling struct
    let agent: Agent = ureq::AgentBuilder::new()
        .timeout_read(Duration::from_secs(5))
        .timeout_write(Duration::from_secs(5))
        .build();

    // parse and run test cases
    for test_case in test_cases {
        let query_location = world.get_location(
            test_case
                .get("in")
                .expect("node name is one char long")
                .chars()
                .next()
                .expect("node name is one char long"),
        );
        let expected_location = world.get_location(
            test_case
                .get("out")
                .expect("node name is one char long")
                .chars()
                .next()
                .expect("node name is one char long"),
        );

        // debug!("{query_location:?} => {expected_location:?}");
        // run queries
        let mut url = format!(
            "http://localhost:5000/nearest/v1/{}/{:?},{:?}",
            world.profile, query_location.longitude, query_location.latitude
        );
        if request_with_flatbuffers {
            url += ".flatbuffers";
        }
        let call = agent.get(&url).call();

        let body = match call {
            Ok(response) => response.into_reader(),
            Err(e) => panic!("http error: {e}"),
        };

        let response = match request_with_flatbuffers {
            true => NearestResponse::from_flatbuffer(body),
            false => NearestResponse::from_json_reader(body),
        };

        if test_case.contains_key("out") {
            // check that result node is (approximately) equivalent
            let result_location = response.waypoints[0].location();
            assert!(approx_equal(
                result_location.longitude,
                expected_location.longitude,
                5
            ));
            assert!(approx_equal(
                result_location.latitude,
                expected_location.latitude,
                5
            ));
        }
        if test_case.contains_key("data_version") {
            assert_eq!(
                test_case.get("data_version"),
                response.data_version.as_ref()
            );
        }
    }
}

fn main() {
    let args = Args::parse();
    debug!("arguments: {:?}", args);

    futures::executor::block_on(
        OSRMWorld::cucumber()
            .max_concurrent_scenarios(1)
            .before(move |feature, _rule, scenario, world| {
                world.scenario_id = common::scenario_id::scenario_id(scenario);
                world.set_scenario_specific_paths_and_digests(feature.path.clone());
                world.osrm_digest = md5_of_osrm_executables().digest().to_hex_lowercase();

                // TODO: clean up cache if needed? Or do in scenarios?

                future::ready(()).boxed()
            })
            .with_writer(DotWriter::default().normalized())
            .run("features/nearest/"),
    );
}
