use std::env;
use std::fmt::Display;
use std::io::Cursor;
use std::path::PathBuf;
use std::time::Duration;
use std::{collections::HashMap, path::Path};

use serde::{Deserialize, Serialize};
use ureq::AgentBuilder;

macro_rules! build_println {
    ($($tokens: tt)*) => {
        println!("cargo:warning=\r\x1b[32;1m   {}", format!($($tokens)*))
    }
}

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

    let executable_path = match env::consts::OS {
        "windows" => "target/flatc.exe",
        _ => "target/flatc",
    };

    if let Some((platform, compiler)) = match env::consts::OS {
        "linux" if env::consts::ARCH == "x86_64" => Some((OS::Linux, ".clang++-15")),
        "macos" if env::consts::ARCH == "x86_64" => Some((OS::MacIntel, "")),
        "macos" if env::consts::ARCH == "aarch64" => Some((OS::Mac, "")),
        "windows" if env::consts::ARCH == "x86_64" => Some((OS::Windows, "")),
        _ => None,
    } {
        let url = format!("https://github.com/google/flatbuffers/releases/download/v{version}/{platform}.flatc.binary{compiler}.zip");

        if !Path::new(executable_path).exists() {
            build_println!("downloading flatc executable from {url}");
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
                panic!("cannot read from stream: {e}");
            };
            let target_dir = PathBuf::from("target");
            zip_extract::extract(Cursor::new(archive), &target_dir, true)
                .expect("flatc cannot be unpacked")
        } else {
            build_println!("cached flatc executable found, not downloading");
        }
    } else {
        build_println!("unsupported platform: {} {}. 'flatc' binary supporting version {} of the library needs to be in system path", env::consts::OS, env::consts::ARCH, version);
    }

    let (flatc, location) = match Path::new(executable_path).exists() {
        true => (flatc_rust::Flatc::from_path(executable_path), "local"),
        false => (flatc_rust::Flatc::from_env_path(), "downloaded"),
    };
    assert!(flatc.check().is_ok());
    let version = &flatc.version().unwrap();
    build_println!(
        "using {location} flatc v{} to compile schema files",
        version.version()
    );
    flatc
        .run(flatc_rust::Args {
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
        .expect("flatc failed generated files");
}
