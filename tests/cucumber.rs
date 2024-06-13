mod common;

use cheap_ruler::CheapRuler;
use clap::Parser;
use common::{
    cli_arguments::Args, dot_writer::DotWriter, f64_utils::approx_equal,
    hash_util::md5_of_osrm_executables, location::Location, osm::OSMWay, osrm_world::OSRMWorld,
};
use core::panic;
use cucumber::{
    gherkin::{Step, Table},
    given, when, World, WriterExt,
};
use futures::{future, FutureExt};
use geo_types::Point;
use log::debug;
use std::collections::HashMap;

fn offset_origin_by(dx: f32, dy: f32, origin: Location, grid_size: f32) -> Location {
    let ruler = CheapRuler::new(origin.latitude, cheap_ruler::DistanceUnit::Meters);
    let loc = ruler.offset(
        &Point::new(origin.longitude, origin.latitude),
        dx * grid_size,
        dy * grid_size,
    ); //TODO: needs to be world's gridSize, not the local one
    Location {
        latitude: loc.y(),
        longitude: loc.x(),
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
                        let location = offset_origin_by(
                            column_index as f32 * 0.5,
                            -(row_index as f32 - 1.),
                            world.origin,
                            world.grid_size,
                        );
                        match name {
                            '0'...'9' => world.add_location(name, location),
                            'a'...'z' => world.add_osm_node(name, location, None),
                            _ => unreachable!("node name not in [0..9][a..z]: {docstring}"),
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

#[given(expr = "a grid size of {float} meters")]
fn set_grid_size(world: &mut OSRMWorld, meters: f32) {
    world.grid_size = meters;
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
}

fn parse_table_from_steps(table: &Option<&Table>) -> Vec<HashMap<String, String>> {
    // parse query data
    let table = table.expect("no query table specified");
    // the following lookup allows to define lat lon columns in any order
    let header = table.rows.first().expect("node locations table empty");
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
    test_cases
    // TODO: also return the header
}

#[when(regex = r"^I request nearest( with flatbuffers|) I should get$")]
fn request_nearest(world: &mut OSRMWorld, step: &Step, state: String) {
    let request_with_flatbuffers = state == " with flatbuffers";

    world.write_osm_file();
    world.extract_osm_file();

    // parse query data
    let test_cases = parse_table_from_steps(&step.table.as_ref());

    // run test cases
    for test_case in &test_cases {
        let query_location = world.get_location(
            test_case
                .get("in")
                .expect("node name is one char long")
                .chars()
                .next()
                .expect("node name is one char long"),
        );

        let response = world.nearest(&query_location, request_with_flatbuffers);

        let expected_location = &world.get_location(
            test_case
                .get("out")
                .expect("node name is one char long")
                .chars()
                .next()
                .expect("node name is one char long"),
        );

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

#[when(regex = r"^I route( with flatbuffers|) I should get$")]
fn request_route(world: &mut OSRMWorld, step: &Step, state: String) {
    let request_with_flatbuffers = state == " with flatbuffers";
    world.write_osm_file();
    world.extract_osm_file();
    // TODO: preprocess

    let test_cases = parse_table_from_steps(&step.table.as_ref());
    for test_case in &test_cases {
        let from_location = world.get_location(
            test_case
                .get("from")
                .expect("node name is one char long")
                .chars()
                .next()
                .expect("node name is one char long"),
        );
        let to_location = world.get_location(
            test_case
                .get("to")
                .expect("node name is one char long")
                .chars()
                .next()
                .expect("node name is one char long"),
        );

        let response = world.route(&from_location, &to_location, request_with_flatbuffers);

        if test_case.contains_key("route") {
            // NOTE: the following code ports logic from JavaScript that checks only properties of the first route
            let route = response
                .routes
                .first()
                .expect("no route returned")
                .legs
                .first()
                .expect("legs required")
                .steps
                .iter()
                .map(|step| step.name.clone())
                .collect::<Vec<String>>()
                .join(",");

            assert_eq!(*test_case.get("route").expect("msg"), route);
        }

        if test_case.contains_key("pronunciations") {
            let pronunciations = response
                .routes
                .first()
                .expect("no route returned")
                .legs
                .first()
                .expect("legs required")
                .steps
                .iter()
                .map(|step| match &step.pronunciation {
                    Some(p) => p.clone(),
                    None => "".to_string(),
                })
                .collect::<Vec<String>>()
                .join(",");
            assert_eq!(
                *test_case.get("pronunciations").expect("msg"),
                pronunciations
            );
        }

        if test_case.contains_key("ref") {
            let refs = response
                .routes
                .first()
                .expect("no route returned")
                .legs
                .first()
                .expect("legs required")
                .steps
                .iter()
                .map(|step| match &step.r#ref {
                    Some(p) => p.clone(),
                    None => "".to_string(),
                })
                .collect::<Vec<String>>()
                .join(",");
            assert_eq!(*test_case.get("ref").expect("msg"), refs);
        }
        // TODO: more checks need to be implemented

        // TODO: check for unchecked test columns
    }

    // unimplemented!("route");
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
            .filter_run("features/car/names.feature", |_, _, sc| {
                !sc.tags.iter().any(|t| t == "todo")
            }),
    );
}
