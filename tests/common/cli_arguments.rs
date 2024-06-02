use std::fmt::Display;

use clap::Parser;

#[derive(clap::ValueEnum, Clone, Default, Debug)]
pub enum LoadMethod {
    Mmap,
    #[default]
    Datastore,
    Directly,
}
impl Display for LoadMethod {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let result = match self {
            LoadMethod::Mmap => "mmap",
            LoadMethod::Datastore => "datastore",
            LoadMethod::Directly => "directly",
        };
        write!(f, "{result}")
    }
}

#[derive(clap::ValueEnum, Clone, Default, Debug)]
pub enum RoutingAlgorithm {
    #[default]
    Ch,
    Mld,
}

impl Display for RoutingAlgorithm {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let result = match self {
            RoutingAlgorithm::Ch => "ch",
            RoutingAlgorithm::Mld => "mld",
        };
        write!(f, "{result}")
    }
}


// TODO: move to external file
#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
pub struct Args {
    // underlying memory storage
    #[arg(short, default_value_t = LoadMethod::Datastore)]
    memory: LoadMethod,

    // Number of times to greet
    #[arg(short, default_value_t = RoutingAlgorithm::Ch)]
    p: RoutingAlgorithm,
}
