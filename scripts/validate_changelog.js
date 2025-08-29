// CHANGELOG.md validator - ensures changelog entries follow the required format standards

import { createInterface } from 'readline';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

// Get current directory path in ES modules
const __dirname = path.dirname(fileURLToPath(import.meta.url));

// Read CHANGELOG.md line by line
const linereader = createInterface( {
  input: fs.createReadStream(path.join(__dirname, '..', 'CHANGELOG.md'))
});

// Track validation state
let done = false;
let linenum = 0;
let has_errors = false;
// Validate each line of the changelog
linereader.on('line', function(line) {
  linenum += 1;
  if (line.match(/^# [^U]/)) done = true;
  if (done) return;

  const line_errors = [];

  if (line.match(/^ {6}/)) {
    if (!line.match(/^ {6}- (ADDED|FIXED|CHANGED|REMOVED): /)) {
      line_errors.push('ERROR: changelog entries must start with \'- (ADDED|FIXED|CHANGED|REMOVED): \'');
    }
    if (!line.match(/\[#[0-9]+\]\(http.*\)$/)) {
      line_errors.push('ERROR: changelog entries must end with an issue or PR link in Markdown format');
    }
  }

  if (line_errors.length > 0) {
    has_errors = true;

    if (process.stdout.isTTY) {
      console.log('\x1b[31mERROR ON LINE %d\x1b[0m: %s', linenum, line);
      for (let i = 0; i<line_errors.length; i++) {
        console.log('    \x1b[33m%s\x1b[0m', line_errors[i]);
      }
    } else {
      console.log('ERROR ON LINE %d: %s', linenum, line);
      for (let i = 0; i<line_errors.length; i++) {
        console.log('    %s', line_errors[i]);
      }
    }
  }

});

linereader.on('close', function() {
  process.exit(has_errors ? 1 : 0);
});