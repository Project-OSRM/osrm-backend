'use strict';

const path = require('path');
const fs = require('fs');
const util = require('util');
const child_process = require('child_process');

module.exports = function () {
    // replaces placeholders for in user supplied commands
    this.expandOptions = (options) => {
        let opts = options.slice();
        let table = {
            '{osm_file}': this.inputCacheFile,
            '{processed_file}': this.processedCacheFile,
            '{profile_file}': this.profileFile,
            '{rastersource_file}': this.rasterCacheFile,
            '{speeds_file}': this.speedsCacheFile,
            '{penalties_file}': this.penaltiesCacheFile
        };

        for (let k in table) {
            opts = opts.replace(k, table[k]);
        }

        return opts;
    };

    this.setupOutputLog = (process, log) => {
        if (process.logFunc) {
            process.stdout.removeListener('data', process.logFunc);
            process.stderr.removeListener('data', process.logFunc);
        }

        process.logFunc = (message) => { log.write(message); };
        process.stdout.on('data', process.logFunc);
        process.stderr.on('data', process.logFunc);
    };

    this.runBin = (bin, options, env, callback) => {
        let cmd = path.resolve(util.format('%s/%s%s', this.BIN_PATH, bin, this.EXE));
        let opts = options.split(' ').filter((x) => { return x && x.length > 0; });
        let log = fs.createWriteStream(this.scenarioLogFile, {'flags': 'a'});
        log.write(util.format('*** running %s %s\n', cmd, options));
        // we need to set a large maxbuffer here because we have long running processes like osrm-routed
        // with lots of log output
        let child = child_process.execFile(cmd, opts, {maxBuffer: 1024 * 1024 * 1000, env: env}, callback);
        child.on('exit', function(code) {
            log.write(util.format('*** %s exited with code %d\n', bin, code));
            // remove listeners and close log file -> some tail messages can be lost
            child.stdout.removeListener('data', child.logFunc);
            child.stderr.removeListener('data', child.logFunc);
            log.end();
        }.bind(this));
        this.setupOutputLog(child, log);
        return child;
    };
};
