use std::fmt::Display;
use std::io::{Cursor, Write};
use std::path::PathBuf;
use std::time::Duration;
use std::{collections::HashMap, path::Path};
use std::{env, fs};

use serde::{Deserialize, Serialize};
use ureq::{Agent, AgentBuilder};

#[derive(Debug)]
enum OS {
    Mac,
    MacIntel,
    Linux,
    Windows,
}

impl Display for OS {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{:?}", self)
    }
}

fn main() {
    #[derive(Debug, Serialize, Deserialize)]
    #[serde(untagged)]
    enum DependencyValue {
        String(String),
        Object {
            version: String,
            features: Vec<String>,
        },
    }
    #[derive(Debug, Serialize, Deserialize)]
    struct CargoToml {
        dependencies: HashMap<String, DependencyValue>,
    }

    let cargo_toml_raw = include_str!("Cargo.toml");
    let cargo_toml: CargoToml = toml::from_str(cargo_toml_raw).unwrap();

    let version = match cargo_toml
        .dependencies
        .get("flatbuffers")
        .expect("Must have dependency flatbuffers")
    {
        DependencyValue::String(s) => s,
        DependencyValue::Object {
            version,
            features: _,
        } => version,
    };
    if let Some((platform, compiler)) = match env::consts::OS {
        "linux" if env::consts::ARCH == "x86_64" => Some((OS::Linux, ".clang++-15")),
        "macos" if env::consts::ARCH == "x86_64" => Some((OS::MacIntel, "")), // TODO: check literals
        "macos" if env::consts::ARCH == "arm" => Some((OS::Mac, "")), // TODO: check literals
        "windows" if env::consts::ARCH == "x86_64" => Some((OS::Windows, "")),
        _ => {
            println!("cargo:warning=unsupported platform: {} {}. 'flatc' binary supporting version {} of the library needs to be in system path", env::consts::OS, env::consts::ARCH, version);
            None
        }
    } {
        let url = format!("https://github.com/google/flatbuffers/releases/download/v{version}/{platform}.flatc.binary{compiler}.zip");

        // download flatc compiler if it does not exist
        if !Path::new("target/flatc").exists() {
            println!("cargo:warning=downloading flatc from {url}");
            let agent = AgentBuilder::new()
                .timeout_read(Duration::from_secs(5))
                .timeout_write(Duration::from_secs(5))
                .build();

            let call = agent.get(&url).call();
            let mut reader = match call {
                Ok(response) => response.into_reader(),
                Err(e) => panic!("http error: {e}"),
            };
            let mut archive = Vec::new();
            if let Err(e) = reader.read_to_end(&mut archive) {
                panic!("cannot read from strem: {e}");
            };
            let target_dir = PathBuf::from("target"); // Doesn't need to exist
            zip_extract::extract(Cursor::new(archive), &target_dir, true).expect("flatc cannot be unpacked")
        } else {
            println!("cargo:warning=cached zip file found, not downloading");
        }
        // TODO: unpack binary
    }
    // Linux + x86: https://github.com/google/flatbuffers/releases/download/v24.3.25/Linux.flatc.binary.clang++-15.zip
    // macOS + aarch: https://github.com/google/flatbuffers/releases/download/v24.3.25/Mac.flatc.binary.zip
    // macOS + x86: https://github.com/google/flatbuffers/releases/download/v24.3.25/MacIntel.flatc.binary.zip
    // Windows + x86: https://github.com/google/flatbuffers/releases/download/v24.3.25/Windows.flatc.binary.zip

    // TODO: check if file exists, and then run it or try global installation
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
