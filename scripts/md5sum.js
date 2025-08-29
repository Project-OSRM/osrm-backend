#!/usr/bin/env node
// MD5 checksum generator and validator for data file integrity verification

import crypto from 'crypto';
import fs from 'fs';

const idx = process.argv.indexOf('-c');
if (idx > -1) {
  const validate_file = process.argv[idx+1];
  if (!process.argv[idx+1]) {
    console.error('Please pass arg to -c with a path to the data.md5sum file used to validate');
    process.exit(1);
  }
  validate(validate_file);
} else {
  const args = process.argv.slice(2);
  if (args.length > 0) {
    generate(args);
  } else {
    console.error('Please pass either a list of files to generate an md5sum for or validate with "-c data.md5sum"');
    process.exit(1);
  }
}


// Synchronously calculate MD5 hash of a file using streaming to handle large files efficiently
function md5FileSync (filename) {
  const BUFFER_SIZE = 8192;
  const fd = fs.openSync(filename, 'r');
  const hash = crypto.createHash('md5');
  const buffer = Buffer.alloc(BUFFER_SIZE);

  try {
    let bytesRead;

    do {
      bytesRead = fs.readSync(fd, buffer, 0, BUFFER_SIZE);
      hash.update(buffer.slice(0, bytesRead));
    } while (bytesRead === BUFFER_SIZE);
  } finally {
    fs.closeSync(fd);
  }

  return hash.digest('hex');
}

// Generate MD5 checksums for multiple files and output in standard format
function generate(files) {
  files.forEach(function(filename) {
    const md5_actual = md5FileSync(filename);
    console.log(md5_actual,'',filename);
  });
}

// Validate files against checksums stored in a .md5sum file
function validate(validate_file) {

  const sums = {};
  const lines = fs.readFileSync(validate_file).
    toString().
    split('\n').
    filter(function(line) {
      return line !== '';
    });

  let error = 0;

  lines.forEach(function(line) {
    const parts = line.split('  ');
    const filename = parts[1];
    const md5 = parts[0];
    sums[filename] = md5;
    const md5_actual = md5FileSync(filename);
    if (md5_actual !== md5) {
      error++;
      console.error(filename + ': FAILED');
    } else {
      console.log(filename + ': OK');
    }
  });

  if (error > 0) {
    console.error('ms5sum.js WARNING: 1 computed checksum did NOT match');
    console.error('\nExpected:');
    lines.forEach(function(line) {
      const parts = line.split('  ');
      const filename = parts[1];
      const md5 = parts[0];
      console.log(md5 + '  ' + filename);
    });
    process.exit(1);
  } else {
    process.exit(0);
  }
}