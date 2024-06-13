// struct to keep state agent, profile, host, etc
// functions to make nearest, route, etc calls
// fn nearest(arg1, ... argn) -> NearestResponse

use std::{path::Path, time::Duration};

use ureq::{Agent, AgentBuilder};

pub struct HttpRequest {
    agent: Agent,
}

impl HttpRequest {
    // pub fn fetch_to_file(url: &str, output: &Path) -> Result<()> {}

    pub fn new() -> Self {
        let agent = AgentBuilder::new()
        .timeout_read(Duration::from_secs(5))
        .timeout_write(Duration::from_secs(5))
        .build();

        Self { agent }
    }
}
