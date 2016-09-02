'use strict';

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

    this.runBin = (bin, options, env, callback) => {
        let cmd = util.format('%s%s/%s%s%s', this.QQ, this.BIN_PATH, bin, this.EXE, this.QQ);
        let opts = options.split(' ').filter((x) => { return x && x.length > 0; });
        this.log(util.format('*** running %s\n', cmd));
        // we need to set a large maxbuffer here because we have long running processes like osrm-routed
        // with lots of log output
        let child = child_process.execFile(cmd, opts, {maxBuffer: 1024 * 1024 * 1000, env: env}, callback);
        child.stdout.on('data', this.log.bind(this));
        child.stderr.on('data', this.log.bind(this));
        child.on('exit', function(code) {
            this.log(util.format('*** %s exited with code %d\n', bin, code));
        }.bind(this));
        return child;
    };
};
