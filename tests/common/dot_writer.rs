use colored::Colorize;
use cucumber::{cli, event, parser, Event};
use std::{io::{self, Write}, time::Instant};

#[derive(Debug, Default)]
pub struct DotWriter {
    scenarios_started: usize,
    scenarios_failed: usize,
    scenarios_finished: usize,
    features_started: usize,
    features_finished: usize,
    step_started: usize,
    step_failed: usize,
    step_passed: usize,
    step_skipped: usize,
    start_time: Option<Instant>,
}

impl<W: 'static> cucumber::Writer<W> for DotWriter {
    type Cli = cli::Empty; // we provide no CLI options

    async fn handle_event(&mut self, ev: parser::Result<Event<event::Cucumber<W>>>, _: &Self::Cli) {
        let green_dot = ".".green();
        let cyan_dash = "-".cyan();
        let red_cross = "X".red();
        match ev {
            Ok(Event { value, .. }) => match value {
                event::Cucumber::Feature(_feature, ev) => match ev {
                    event::Feature::Started => {
                        self.features_started += 1;
                        print!("{green_dot}")
                    }
                    event::Feature::Scenario(_scenario, ev) => match ev.event {
                        event::Scenario::Started => {
                            self.scenarios_started += 1;
                            print!("{green_dot}")
                        }
                        event::Scenario::Step(_step, ev) => match ev {
                            event::Step::Started => {
                                self.step_started += 1;
                                print!("{green_dot}")
                            }
                            event::Step::Passed(..) => {
                                self.step_passed += 1;
                                print!("{green_dot}")
                            }
                            event::Step::Skipped => {
                                self.step_skipped += 1;
                                print!("{cyan_dash}")
                            }
                            event::Step::Failed(_, _, _, _err) => {
                                self.step_failed += 1;
                                print!("{red_cross}")
                            }
                        },
                        event::Scenario::Hook(_, _) => {}
                        event::Scenario::Background(_, _) => {}
                        event::Scenario::Log(_) => {}
                        event::Scenario::Finished => {
                            self.scenarios_finished += 1;
                        }
                    },
                    event::Feature::Rule(_, _) => {}
                    event::Feature::Finished => {
                        self.features_finished += 1;
                    }
                },
                event::Cucumber::Finished => {
                    println!();
                    let f = format!("{} failed", self.scenarios_failed).red();
                    let p = format!("{} passed", self.scenarios_finished).green();
                    println!("{} scenarios ({f}, {p})", self.scenarios_started);
                    let f = format!("{} failed", self.step_failed).red();
                    let s = format!("{} skipped", self.step_skipped).cyan();
                    let p = format!("{} passed", self.step_passed).green();
                    println!("{} steps ({f}, {s}, {p})", self.step_started);

                    let elapsed =  Instant::now() - self.start_time.unwrap();
                    let minutes = elapsed.as_secs()/60;
                    let seconds = (elapsed.as_millis() % 60_000) as f64 / 1000.;
                    println!("{}m{}s", minutes, seconds);
                }
                event::Cucumber::ParsingFinished {
                    features: _,
                    rules: _,
                    scenarios: _,
                    steps: _,
                    parser_errors: _,
                } => {},
                event::Cucumber::Started => {
                    self.start_time = Some(Instant::now());
                },
            },
            Err(e) => println!("Error: {e}"),
        }
        let _ = io::stdout().flush();
    }
}
