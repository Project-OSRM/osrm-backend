use serde::Deserialize;

#[derive(Debug, Default, Deserialize)]
pub struct OSRMError {
    pub code: String,
    pub message: String,
}

impl OSRMError {
    pub fn from_json_reader(reader: impl std::io::Read) -> Self {
        let response = match serde_json::from_reader::<_, Self>(reader) {
            Ok(response) => response,
            Err(e) => panic!("parsing error {e}"),
        };
        response
    }
}