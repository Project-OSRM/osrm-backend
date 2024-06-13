#![allow(clippy::derivable_impls, clippy::all)]
extern crate flatbuffers;
 
pub mod cli_arguments;
pub mod dot_writer;
pub mod f64_utils;
pub mod file_util;
pub mod hash_util;
pub mod http_request;
pub mod lexicographic_file_walker;
pub mod location;
pub mod nearest_response;
pub mod osm;
pub mod osm_db;
pub mod osrm_world;
pub mod scenario_id;
pub mod task_starter;

// flatbuffer
#[allow(dead_code, unused_imports)]
#[path = "../../target/flatbuffers/position_generated.rs"]
pub mod position_flatbuffers;
#[allow(dead_code, unused_imports)]
#[path = "../../target/flatbuffers/waypoint_generated.rs"]
pub mod waypoint_flatbuffers;
#[allow(dead_code, unused_imports)]
#[path = "../../target/flatbuffers/table_generated.rs"]
pub mod table_flatbuffers;
#[allow(dead_code, unused_imports)]
#[path = "../../target/flatbuffers/route_generated.rs"]
pub mod route_flatbuffers;
#[allow(dead_code, unused_imports)]
#[path = "../../target/flatbuffers/fbresult_generated.rs"]
pub mod fbresult_flatbuffers;
