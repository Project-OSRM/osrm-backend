use std::{
    io::{BufRead, BufReader},
    process::{Child, Command, Stdio},
};

#[derive(Default)]
pub struct TaskStarter {
    ready: bool,
    command: String,
    arguments: Vec<String>,
    child: Option<Child>,
}

impl TaskStarter {
    pub fn new(command: &str) -> Self {
        Self {
            ready: false,
            command: command.into(),
            arguments: Vec::new(),
            child: None,
        }
    }
    pub fn is_ready(&self) -> bool {
        self.ready
    }
    pub fn arg(&mut self, argument: &str) -> &mut Self {
        self.arguments.push(argument.into());
        self
    }

    pub fn spawn_wait_till_ready(&mut self, ready_token: &str) {
        let mut command = &mut Command::new(&self.command);
        for argument in &self.arguments {
            command = command.arg(argument);
        }

        match command.stdout(Stdio::piped()).spawn() {
            Ok(o) => self.child = Some(o),
            Err(e) => panic!("cannot spawn task: {e}"),
        }

        if let Some(output) = &mut self.child.as_mut().unwrap().stdout {
            // implement with a timeout
            let mut reader = BufReader::new(output);
            let mut line = String::new();
            while let Ok(_count) = reader.read_line(&mut line) {
                // println!("count: {count} ->{line}");
                if line.contains(ready_token) {
                    self.ready = true;
                    break;
                }
            }
        }
    }
}

impl Drop for TaskStarter {
    fn drop(&mut self) {
        if let Err(e) = self.child.as_mut().expect("can't access child").kill() {
            panic!("shutdown failed: {e}");
        }
    }
}
