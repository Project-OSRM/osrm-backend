use std::{env, fs, path::PathBuf};

use log::debug;

use crate::common::{
    file_util::get_file_as_byte_vec, lexicographic_file_walker::LexicographicFileWalker,
};

pub fn md5_of_osrm_executables() -> chksum_md5::MD5 {
    // create OSRM digest before any tests are executed since cucumber-rs doesn't have @beforeAll
    let exe_path = env::current_exe().expect("failed to get current exe path");
    let path = exe_path
    .ancestors()
    .find(|p| p.ends_with("target"))
    .expect("compiled cucumber test executable resides in a directory tree with the root name 'target'")
    .parent().unwrap();
    // TODO: Remove after migration to Rust build dir
    let build_path = path.join("build");
    // TODO: Remove after migration to Rust build dir
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
    debug!("{profiles_path:?}");

    dependencies.extend(
        LexicographicFileWalker::new(&profiles_path)
            .filter(|pb| !pb.to_str().unwrap().contains("examples"))
            .filter(|pathbuf| match pathbuf.extension() {
                Some(ext) => ext.to_str().unwrap() == "lua",
                None => false,
            }),
    );
    let mut md5 = chksum_md5::new();
    debug!("md5: {}", md5.digest().to_hex_lowercase());

    for path_buf in dependencies {
        let data = get_file_as_byte_vec(&path_buf);
        if data.is_empty() {
            continue;
        }
        md5.update(data);
        // debug!("md5: {}", md5.digest().to_hex_lowercase());
    }
    md5
}
