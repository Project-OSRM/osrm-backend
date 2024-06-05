use std::io::{self, Write};

use cucumber::{cli, event, parser, Event};
// TODO: add colors
pub struct DotWriter;

impl<W: 'static> cucumber::Writer<W> for DotWriter {
    type Cli = cli::Empty; // we provide no CLI options

    async fn handle_event(&mut self, ev: parser::Result<Event<event::Cucumber<W>>>, _: &Self::Cli) {
        match ev {
            Ok(Event { value, .. }) => match value {
                event::Cucumber::Feature(_feature, ev) => match ev {
                    event::Feature::Started => {
                        print!(".")
                    }
                    event::Feature::Scenario(_scenario, ev) => match ev.event {
                        event::Scenario::Started => {
                            print!(".")
                        }
                        event::Scenario::Step(_step, ev) => match ev {
                            event::Step::Started => {
                                print!(".")
                            }
                            event::Step::Passed(..) => print!("."),
                            event::Step::Skipped => print!("-"),
                            event::Step::Failed(_, _, _, _err) => {
                                print!("x")
                            }
                        },
                        _ => {}
                    },
                    _ => {}
                },
                _ => {}
            },
            Err(e) => println!("Error: {e}"),
        }
        let _ = io::stdout().flush();
    }
}
