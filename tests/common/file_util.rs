use std::{fs::{self, File}, io::Read, path::PathBuf};

use log::debug;

pub fn get_file_as_byte_vec(path: &PathBuf) -> Vec<u8> {
    debug!("opening {path:?}");
    let mut f = File::open(path).expect("no file found");
    let metadata = fs::metadata(path).expect("unable to read metadata");
    let mut buffer = vec![0; metadata.len() as usize];
    match f.read(&mut buffer) {
        Ok(l) => assert_eq!(metadata.len() as usize, l, "data was not completely read"),
        Err(e) => panic!("Error: {e}"),
    }

    buffer
}