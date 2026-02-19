#!/usr/bin/env node
// Command execution timer - runs shell commands and logs execution time to file

import { exec } from 'child_process';
import fs from 'fs';

const name = process.argv[2];
const cmd = process.argv.slice(3).join(' ');
const start = Date.now();
exec(cmd, (err, stdout, stderr) => {
  if (err) {
    console.log(stdout);
    console.log(stderr);
    return process.exit(err.code);
  }
  const stop = +new Date();
  const time = (stop - start) / 1000.;
  fs.appendFileSync('/tmp/osrm.timings', `${name}\t${time}`, 'utf-8');
});