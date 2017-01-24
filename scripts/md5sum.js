#!/usr/bin/env node

'use strict'

var crypto = require('crypto');
var fs = require('fs');

var idx = process.argv.indexOf('-c');
if (idx > -1) {
  var validate_file = process.argv[idx+1];
  if (!process.argv[idx+1]) {
      console.error('Please pass arg to -c with a path to the data.md5sum file used to validate');
      process.exit(1);
  }
  validate(validate_file);
} else {
  // we are generating checksums for all filenames passed
  var args = process.argv.slice(2);
  if (args.length > 0) {
      generate(args);
  } else {
      console.error('Please pass either a list of files to generate an md5sum for or validate with "-c data.md5sum"');
      process.exit(1);
  }
}


function md5FileSync (filename) {
  var BUFFER_SIZE = 8192;
  var fd = fs.openSync(filename, 'r');
  var hash = crypto.createHash('md5');
  var buffer = new Buffer(BUFFER_SIZE);

  try {
    var bytesRead;

    do {
      bytesRead = fs.readSync(fd, buffer, 0, BUFFER_SIZE);
      hash.update(buffer.slice(0, bytesRead));
    } while (bytesRead === BUFFER_SIZE)
  } finally {
    fs.closeSync(fd);
  }

  return hash.digest('hex');
}

function generate(files) {
  files.forEach(function(filename) {
    var md5_actual = md5FileSync(filename);
    console.log(md5_actual,'',filename);
  })
}

function validate(validate_file) {

  var sums = {};
  var lines = fs.readFileSync(validate_file).
    toString().
    split('\n').
    filter(function(line) {
      return line !== "";
  });

  var error = 0;

  lines.forEach(function(line) {
      var parts = line.split('  ');
      var filename = parts[1];
      var md5 = parts[0];
      sums[filename] = md5;
      var md5_actual = md5FileSync(filename);
      if (md5_actual !== md5) {
          error++;
          console.error(filename + ': FAILED')
      } else {
          console.log(filename + ': OK');
      }
  })

  if (error > 0) {
      console.error('ms5sum.js WARNING: 1 computed checksum did NOT match');
      console.error('\nExpected:')
      lines.forEach(function(line) {
          var parts = line.split('  ');
          var filename = parts[1];
          var md5 = parts[0];
          console.log(md5 + '  ' + filename);
      })
      process.exit(1);
  } else {
      process.exit(0);
  }
}
