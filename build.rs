extern crate flatc_rust; // or just `use flatc_rust;` with Rust 2018 edition.

use std::path::Path;

fn main() {
    println!("cargo:rerun-if-changed=include/engine/api/flatbuffers/");
    flatc_rust::run(flatc_rust::Args {
        extra: &["--gen-all"],
        inputs: &[
            Path::new("include/engine/api/flatbuffers/position.fbs"),
            Path::new("include/engine/api/flatbuffers/waypoint.fbs"),
            Path::new("include/engine/api/flatbuffers/route.fbs"),
            Path::new("include/engine/api/flatbuffers/table.fbs"),
            Path::new("include/engine/api/flatbuffers/fbresult.fbs"),
        ],
        out_dir: Path::new("target/flatbuffers/"),
        ..Default::default()
    })
    .expect("flatc");
}
