use std::{
    collections::VecDeque,
    fs,
    path::{Path, PathBuf},
};

// TODO: port into toolbox-rs
pub struct LexicographicFileWalker {
    dirs: VecDeque<PathBuf>,
    files: VecDeque<PathBuf>,
}

impl LexicographicFileWalker {
    pub fn new(path: &Path) -> Self {
        let mut dirs = VecDeque::new();

        if path.is_dir() {
            dirs.push_back(path.to_path_buf());
        }

        Self {
            dirs,
            files: VecDeque::new(),
        }
    }
}

impl Iterator for LexicographicFileWalker {
    type Item = PathBuf;

    fn next(&mut self) -> Option<Self::Item> {
        if self.dirs.is_empty() && self.files.is_empty() {
            return None;
        }
        while self.files.is_empty() && !self.dirs.is_empty() {
            assert!(!self.dirs.is_empty());
            let current_dir = self.dirs.pop_front().unwrap();
            let mut temp_dirs = Vec::new();

            for entry in fs::read_dir(current_dir).unwrap() {
                let entry = entry.unwrap();
                let path = entry.path();
                if path.is_dir() {
                    temp_dirs.push(path.clone());
                } else {
                    self.files.push_back(path.clone());
                }
            }
            self.files.make_contiguous().sort();
            temp_dirs.sort();
            self.dirs.extend(temp_dirs.into_iter());
        }
        self.files.pop_front()
    }
}
