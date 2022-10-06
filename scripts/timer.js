#!/usr/bin/env node

var exec = require('child_process').exec;
var fs = require('fs');

var name = process.argv[2];
var cmd = process.argv.slice(3).join(' ');
var start = Date.now();
exec(cmd, (err, stdout, stderr) => {
    if (err) {
        console.log(stdout);
        console.log(stderr);
        return process.exit(err.code);
    }
    var stop = +new Date();
    var time = (stop - start) / 1000.;
    fs.appendFileSync('/tmp/osrm.timings', `${name}\t${time}`, 'utf-8');
});


