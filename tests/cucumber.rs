mod common;

use cheap_ruler::CheapRuler;
use clap::Parser;
use common::{
    cli_arguments::Args,
    dot_writer::DotWriter,
    f64_utils::{
        approx_equal, approx_equal_within_offset_range, aprox_equal_within_percentage_range,
    },
    hash_util::md5_of_osrm_executables,
    location::Location,
    osm::OSMWay,
    osrm_world::OSRMWorld,
    route_response,
};
use core::panic;
use cucumber::{
    gherkin::{Step, Table},
    given, then, when,
    writer::summarize,
    World, WriterExt,
};
use futures::{future, FutureExt};
use geo_types::Point;
use log::debug;
use std::{
    collections::{HashMap, HashSet},
    iter::zip,
};

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

#[given(expr = "the query options")]
fn set_query_options(world: &mut OSRMWorld, step: &Step) {
    let table = parse_option_table(&step.table.as_ref());
    world.query_options.extend(table.into_iter());
}

#[given(expr = "the node locations")]
fn set_node_locations(world: &mut OSRMWorld, step: &Step) {
    let table = step.table().expect("cannot get table");
    let header = table.rows.first().expect("node locations table empty");
    assert!(header.len() >= 3, "header needs to define three columns");
    assert!(
        header.contains(&"node".to_string()),
        "a column needs to be 'node' indicating the one-letter name"
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
        assert!(
            row.len() >= 3,
            "nod locations must at least specify three tables: node, lat, and lon"
        );
        assert_eq!(row[0].len(), 1, "node name not in [0..9][a..z]");
        let name = &row[0].chars().next().expect("node name cannot be empty"); // the error is unreachable
        let lon = &row[header_lookup["lon"]];
        let lat = &row[header_lookup["lat"]];
        let location = Location {
            latitude: lat.parse::<f32>().expect("lat {lat} needs to be a f64"),
            longitude: lon.parse::<f32>().expect("lon {lon} needs to be a f64"),
        };
        let id = match header_lookup.get("id") {
            Some(index) => {
                let id = row[*index]
                    .parse::<u64>()
                    .expect("id of a node must be u64 number");
                Some(id)
            }
            _ => None,
        };
        match name {
            '0'...'9' => world.add_location(*name, location),
            'a'...'z' => world.add_osm_node(*name, location, id),
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
                            _ => {} // TODO: unreachable!("node name not in [0..9][a..z]: {docstring}"),
                                    //       tests contain random characters.
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

fn parse_option_table(table: &Option<&Table>) -> HashMap<String, String> {
    let table = table.expect("no query table specified");
    let result = table
        .rows
        .iter()
        .map(|row| {
            assert_eq!(2, row.len());
            (row[0].clone(), row[1].clone())
        })
        .collect();
    result
}

fn parse_table_from_steps(table: &Option<&Table>) -> (Vec<String>, Vec<HashMap<String, String>>) {
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
    (header.clone(), test_cases)
}

#[when(regex = r"^I request nearest( with flatbuffers|) I should get$")]
fn request_nearest(world: &mut OSRMWorld, step: &Step, state: String) {
    world.request_with_flatbuffers = state == " with flatbuffers";

    world.write_osm_file();
    world.extract_osm_file();

    // parse query data
    let (_, test_cases) = parse_table_from_steps(&step.table.as_ref());

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

        let response = world.nearest(&query_location);

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

#[then(expr = "routability should be")]
fn routability(world: &mut OSRMWorld, step: &Step) {
    world.write_osm_file();
    world.extract_osm_file();
    // TODO: preprocess

    let (header, test_cases) = parse_table_from_steps(&step.table.as_ref());
    // TODO: rename forw/backw to forw/backw_speed
    let supported_headers = HashSet::<_>::from([
        "forw",
        "backw",
        "bothw",
        "forw_rate",
        "backw_rate",
        "bothw_rate",
    ]);
    if 0 == header
        .iter()
        .filter(|title| supported_headers.contains(title.as_str()))
        .count()
    {
        panic!(
            r#"*** routability table must contain either "forw", "backw", "bothw", "forw_rate" or "backw_mode" column"#
        );
    }

    test_cases
        .iter()
        .enumerate()
        .for_each(|(index, test_case)| {
            let source = offset_origin_by(
                1. + world.way_spacing * index as f32,
                0.,
                world.origin,
                world.grid_size,
            );
            let target = offset_origin_by(
                3. + world.way_spacing * index as f32,
                0.,
                world.origin,
                world.grid_size,
            );
            test_case
                .iter()
                .filter(|(title, _)| supported_headers.contains(title.as_str()))
                .for_each(|(title, expectation)| {
                    let forward = title.starts_with("forw");
                    // println!("{direction}: >{expectation}<");
                    let response = match forward {
                        true => world.route(&vec![source, target]),
                        false => world.route(&vec![target, source]),
                    };
                    if expectation.is_empty() {
                        // if !response.routes.is_empty() {
                        //     println!("> {title} {expectation}");
                        //     println!("{response:?}");
                        // }

                        assert!(
                            response.routes.is_empty()
                                || response.routes.first().unwrap().distance == 0.,
                            "no route expected when result column {title} is unset"
                        );
                    } else if expectation.contains("km/h") {
                        assert!(
                            !response.routes.is_empty(),
                            "route expected when result column is set"
                        );

                        let (expected_speed, offset) =
                            extract_number_and_offset("km/h", expectation);
                        let route = response.routes.first().unwrap();
                        let actual_speed = route.distance / route.duration * 3.6;
                        assert!(
                            aprox_equal_within_percentage_range(
                                actual_speed,
                                expected_speed,
                                offset
                            ),
                            "{actual_speed} and {expected_speed} differ by more than {offset}"
                        );
                    } else if title.ends_with("_rate") {
                        assert!(!response.routes.is_empty());
                        let expected_rate = expectation
                            .parse::<f64>()
                            .expect("rate needs to be a number");
                        let route = response.routes.first().unwrap();
                        let actual_rate = route.distance / route.weight;
                        assert!(
                            aprox_equal_within_percentage_range(actual_rate, expected_rate, 1),
                            "{actual_rate} and {expected_rate} differ by more than 1%"
                        );
                    } else {
                        unimplemented!("{title} = {expectation}");
                    }
                });
        });
    // unimplemented!("{test_cases:#?}");
}

fn extract_number_and_offset(unit: &str, expectation: &str) -> (f64, u8) {
    let tokens: Vec<_> = expectation
        .split(unit)
        .map(|token| token.trim())
        .filter(|token| !token.is_empty())
        .collect();
    // println!("{tokens:?}");
    let number = tokens[0]
        .parse::<f64>()
        .expect("{expectation} needs to define a speed");
    let offset = match tokens.len() {
        1 => 5u8, // TODO: the JS fuzzy matcher has a default margin of 5% for absolute comparsions. This is imprecise
        2 => tokens[1]
            .replace("+-", "")
            .trim()
            .parse()
            .expect(&format!("{} needs to specify a number", tokens[1])),
        _ => unreachable!("expectations can't be parsed"),
    };
    (number, offset)
}

fn extract_number_vector_and_offset(unit: &str, expectation: &str) -> (Vec<f64>, u8) {
    let expectation = expectation.replace(",", "");
    let tokens: Vec<_> = expectation
        .split(unit)
        .map(|token| token.trim())
        .filter(|token| !token.is_empty())
        .collect();
    let numbers = tokens
        .iter()
        .filter(|token| !token.contains("+-"))
        .map(|token| {
            token
                .parse::<f64>()
                .expect("input needs to specify a number followed by unit")
        })
        .collect();

    // panic!("{tokens:?}");
    let offset = match tokens.len() {
        1 => 5u8, // TODO: the JS fuzzy matcher has a default margin of 5% for absolute comparsions. This is imprecise
        _ => tokens
            .last()
            .expect("offset needs to be specified")
            .replace("+-", "")
            .trim()
            .parse()
            .expect(&format!("{} needs to specify a number", tokens[1])),
        // _ => unreachable!("expectations can't be parsed"),
    };
    (numbers, offset)
}

enum WaypointsOrLocation {
    Waypoints,
    Locations,
    // Undefined,
}

pub fn get_location_specification(test_case: &HashMap<String, String>) -> WaypointsOrLocation {
    assert!(
        test_case.contains_key("from")
            && test_case.contains_key("to")
            && !test_case.contains_key("waypoints")
            || !test_case.contains_key("from")
                && !test_case.contains_key("to")
                && test_case.contains_key("waypoints"),
        "waypoints need to be specified by either from/to columns or a waypoint column, but not both"
    );

    if test_case.contains_key("from")
        && test_case.contains_key("to")
        && !test_case.contains_key("waypoints")
    {
        return WaypointsOrLocation::Locations;
    }

    if !test_case.contains_key("from")
        && !test_case.contains_key("to")
        && test_case.contains_key("waypoints")
    {
        return WaypointsOrLocation::Waypoints;
    }
    unreachable!("waypoints need to be specified by either from/to columns or a waypoint column, but not both");
    // WaypointsOrLocation::Undefined
}

#[given(expr = r"skip waypoints")]
fn skip_waypoints(world: &mut OSRMWorld, step: &Step) {
    // TODO: adapt test to use query options
    // only used in features/testbot/basic.feature
    world
        .query_options
        .insert("skip_waypoints".into(), "true".into());
}

#[when(regex = r"^I route( with flatbuffers|) I should get$")]
fn request_route(world: &mut OSRMWorld, step: &Step, state: String) {
    world.request_with_flatbuffers = state == " with flatbuffers";
    world.write_osm_file();
    world.extract_osm_file();
    // TODO: preprocess

    let (_, test_cases) = parse_table_from_steps(&step.table.as_ref());
    for test_case in &test_cases {
        let waypoints = match get_location_specification(&test_case) {
            WaypointsOrLocation::Waypoints => {
                let locations: Vec<Location> = test_case
                    .get("waypoints")
                    .expect("locations specified as waypoints")
                    .split(",")
                    .into_iter()
                    .map(|name| {
                        assert!(name.len() == 1, "node names need to be of length one");
                        world.get_location(name.chars().next().unwrap())
                    })
                    .collect();
                locations
            }
            WaypointsOrLocation::Locations => {
                let from_location = world.get_location(
                    test_case
                        .get("from")
                        .expect("test case doesn't have a 'from' column")
                        .chars()
                        .next()
                        .expect("from node name is one char long"),
                );
                let to_location = world.get_location(
                    test_case
                        .get("to")
                        .expect("test case doesn't have a 'to' column")
                        .chars()
                        .next()
                        .expect("to node name is one char long"),
                );
                vec![from_location, to_location]
            }
        };

        if let Some(bearings) = test_case.get("bearings").cloned() {
            // TODO: change test cases to provide proper query options
            world
                .query_options
                .insert("bearings".into(), bearings.replace(" ", ";"));
        }

        let response = world.route(&waypoints);

        test_case
            .iter()
            .map(|(column_title, expectation)| (column_title.as_str(), expectation.as_str()))
            .for_each(|(case, expectation)| match case {
                "from" | "to" | "bearings" | "waypoints" | "#" => {}, // ignore input and comment columns
                "route" => {
                    let route = if expectation.is_empty() {
                        assert!(response.routes.is_empty());
                        String::new()
                    } else {
                        response
                            .routes
                            .first()
                            .expect("no route returned")
                            .legs
                            .iter()
                            .map(|leg| {
                                leg.steps
                                .iter()
                                .map(|step| step.name.clone())
                                .collect::<Vec<String>>()
                                .join(",")
                            }).collect::<Vec<String>>()
                            .join(",")

                    };

                    assert_eq!(expectation, route);
                },
                "pronunciations" => {
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
                    assert_eq!(expectation, pronunciations);
                },
                "ref" => {
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
                    assert_eq!(expectation, refs);
                },
                "speed" => {
                    let route = response.routes.first().expect("no route returned");
                    let actual_speed = route.distance / route.duration * 3.6;
                    let (expected_speed, offset) = extract_number_and_offset("km/h", expectation);
                    // println!("{actual_speed} == {expected_speed} +- {offset}");
                    assert!(
                        aprox_equal_within_percentage_range(actual_speed, expected_speed, offset),
                        "actual time {actual_speed} not equal to expected value {expected_speed}"
                    );
                },
                "modes" => {
                    let route = response.routes.first().expect("no route returned");
                    let actual_modes = route
                        .legs
                        .iter()
                        .map(|leg| {
                            leg.steps
                                .iter()
                                .map(|step| step.mode.clone())
                                .collect::<Vec<String>>()
                                .join(",")
                        })
                        .collect::<Vec<String>>()
                        .join(",");
                    assert_eq!(actual_modes, expectation);
                },
                "turns" => {
                    let route = response.routes.first().expect("no route returned");
                    let actual_turns = route
                        .legs
                        .iter()
                        .map(|leg| {
                    leg.steps
                                .iter()
                                .map(|step| {
                                    let prefix = step.maneuver.r#type.clone();
                                    if prefix == "depart" || prefix == "arrive" {
                                        // TODO: this reimplements the behavior that depart and arrive are not checked for their modifier
                                        //       check if tests shall be adapted, since this is reported by the engine
                                        return prefix;
                                    }
                                    let suffix = match &step.maneuver.modifier {
                                        Some(modifier) => " ".to_string() + &modifier,
                                        _ => "".into(),
                                    };
                                    prefix + &suffix
                                })
                                .collect::<Vec<String>>()
                                .join(",")
                        })
                        .collect::<Vec<String>>()
                        .join(",");
                    assert_eq!(actual_turns, expectation);
                },
                "time" => {
                    let actual_time = response.routes.first().expect("no route returned").duration;
                    let (expected_time, offset) = extract_number_and_offset("s", expectation);
                    // println!("{actual_time} == {expected_time} +- {offset}");
                    assert!(
                        approx_equal_within_offset_range(actual_time, expected_time, offset as f64),
                        "actual time {actual_time} not equal to expected value {expected_time}"
                    );
                },
                "times" => {
                    // TODO: go over steps

                    let actual_times : Vec<f64>= response.routes.first().expect("no route returned").legs.iter().map(|leg| {
                        leg.steps.iter().filter(|step| step.duration > 0.).map(|step| step.duration).collect::<Vec<f64>>()
                    }).flatten().collect();
                    let (expected_times, offset) = extract_number_vector_and_offset("s", expectation);
                    // println!("{actual_times:?} == {expected_times:?} +- {offset}");
                    assert_eq!(actual_times.len(), expected_times.len(), "times mismatch: {actual_times:?} != {expected_times:?} +- {offset}");

                    zip(actual_times, expected_times).for_each(|(actual_time, expected_time)| {
                        assert!(approx_equal_within_offset_range(actual_time, expected_time, offset as f64),
                            "actual time {actual_time} not equal to expected value {expected_time}");
                    });
                },
                "distances" => {
                    let actual_distances = response.routes.first().expect("no route returned").legs.iter().map(|leg| {
                        leg.steps.iter().filter(|step| step.distance > 0.).map(|step| step.distance).collect::<Vec<f64>>()
                    }).flatten().collect::<Vec<f64>>();
                    let (expected_distances, offset) = extract_number_vector_and_offset("m", expectation);
                    assert_eq!(expected_distances.len(), actual_distances.len(), "distances mismatch {expected_distances:?} != {actual_distances:?} +- {offset}");

                    zip(actual_distances, expected_distances).for_each(|(actual_distance, expected_distance)| {
                        assert!(approx_equal_within_offset_range(actual_distance, expected_distance, offset as f64),
                            "actual distance {actual_distance} not equal to expected value {expected_distance}");
                    });
                },
                "weight" => {
                    let actual_weight = response.routes.first().expect("no route returned").weight;
                    let (expected_weight, offset) = extract_number_and_offset("s", expectation);
                    assert!(
                        approx_equal_within_offset_range(
                            actual_weight,
                            expected_weight,
                            offset as f64
                        ),
                        "actual time {actual_weight} not equal to expected value {expected_weight}"
                    );
                },
                "distance" => {
                    let actual_distance = response.routes.first().expect("no route returned").distance;
                    let (expected_distance, offset) = extract_number_and_offset("m", expectation);
                    assert!(
                        approx_equal_within_offset_range(
                            actual_distance,
                            expected_distance,
                            offset as f64
                        ),
                        "actual time {actual_distance} not equal to expected value {expected_distance}"
                    );
                },
                "summary" => {
                    let actual_summary = response.routes.first().expect("no route returned").legs.iter().map(|leg| {
                        leg.summary.clone()
                    }).collect::<Vec<String>>().join(",");
                    assert_eq!(actual_summary,expectation, "summary mismatch");
                },
                "data_version" => {
                    let expected_data_version = match test_case.get("data_version") {
                        Some(s) if !s.is_empty() => Some(s),
                        _ => None,
                    };
                    assert_eq!(
                        expected_data_version,
                        response.data_version.as_ref(),
                        "data_version does not match"
                    );
                },
                "waypoints_count" => {
                    let expected_waypoint_count = match test_case.get("waypoints_count") {
                        Some(s) if !s.is_empty() => s.parse::<usize>().expect("waypoint_count is a number"),
                        _ => 0,
                    };
                    let actual_waypoint_count = match &response.waypoints {
                        Some(w) => w.len(),
                        None => 0,
                    };
                    assert_eq!(
                        expected_waypoint_count,
                        actual_waypoint_count,
                        "waypoint_count does not match"
                    );
                },
                "geometry" => {
                    let expected_geometry = test_case.get("geometry").expect("no geometry found");
                    match &response.routes.first().expect("no route").geometry {
                        route_response::Geometry::A(actual_geometry) => {
                            assert_eq!(
                                expected_geometry,
                                actual_geometry,
                                "geometry does not match"
                            );
                        },
                        route_response::Geometry::B { coordinates: _, r#type: _ } => unimplemented!("geojson comparison"),
                    }


                },
                // "classes" => {},
                // TODO: more checks need to be implemented
                _ => {
                    let msg = format!("case {case} = {expectation} not implemented");
                    unimplemented!("{msg}");
                }
            });
    }
}
fn main() {
    let args = Args::parse();
    debug!("{args:?}");
    let digest = md5_of_osrm_executables().digest().to_hex_lowercase();

    futures::executor::block_on(
        OSRMWorld::cucumber()
            .max_concurrent_scenarios(1)
            .before(move |feature, _rule, scenario, world| {
                world.scenario_id = common::scenario_id::scenario_id(scenario);
                world.set_scenario_specific_paths_and_digests(feature.path.clone());
                world.osrm_digest = digest.clone();

                // TODO: clean up cache if needed? Or do in scenarios?

                future::ready(()).boxed()
            })
            // .with_writer(DotWriter::default().normalized())
            // .filter_run("features/testbot/geometry.feature", |_, _, sc| {
            .filter_run("features", |_, _, sc| !sc.tags.iter().any(|t| t == "todo")),
    );
}
