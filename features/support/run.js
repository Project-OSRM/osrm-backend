'use strict';

const fs = require('fs');
const path = require('path');
const util = require('util');
const child_process = require('child_process');

module.exports = function () {
    // replaces placeholders for in user supplied commands
    this.expandOptions = (options) => {
        var opts = options.slice();

        if (opts.match('{osm_file}')) {
            opts = opts.replace('{osm_file}', this.inputCacheFile);
        }

        if (opts.match('{processed_file}')) {
            opts = opts.replace('{processed_file}', this.processedCacheFile);
        }

        if (opts.match('{profile_file}')) {
            opts = opts.replace('{profile_file}', this.profileFile);
        }

        return opts;
    };

    this.runBin = (bin, options, callback) => {
        let cmd = util.format('%s%s/%s%s%s %s', this.QQ, this.BIN_PATH, bin, this.EXE, this.QQ, options);
        this.log(util.format('*** running %s\n', cmd));
        let child = child_process.exec(cmd, callback);
        child.stdout.on('data', this.log.bind(this));
        child.stderr.on('data', this.log.bind(this));
        child.on('exit', function(code) {
          this.log(util.format('*** %s exited with code %d\n', bin, code));
        }.bind(this));
        return child;
    };
};
