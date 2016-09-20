var fs = require('fs');
var util = require('util');
var exec = require('child_process').exec;

module.exports = function () {
    this.runBin = (bin, options, callback) => {
        var opts = options.slice();

        if (opts.match('{osm_base}')) {
            if (!this.osmData.osmFile) throw new Error('*** {osm_base} is missing');
            opts = opts.replace('{osm_base}', this.osmData.osmFile);
        }

        if (opts.match('{extracted_base}')) {
            if (!this.osmData.extractedFile) throw new Error('*** {extracted_base} is missing');
            opts = opts.replace('{extracted_base}', this.osmData.extractedFile);
        }

        if (opts.match('{contracted_base}')) {
            if (!this.osmData.contractedFile) throw new Error('*** {contracted_base} is missing');
            opts = opts.replace('{contracted_base}', this.osmData.contractedFile);
        }

        if (opts.match('{profile}')) {
            opts = opts.replace('{profile}', [this.PROFILES_PATH, this.profile + '.lua'].join('/'));
        }

        var cmd = util.format('%s%s/%s%s%s %s 2>%s', this.QQ, this.BIN_PATH, bin, this.EXE, this.QQ, opts, this.ERROR_LOG_FILE);
        process.chdir(this.TEST_FOLDER);
        exec(cmd, (err, stdout, stderr) => {
            this.stdout = stdout.toString();
            fs.readFile(this.ERROR_LOG_FILE, (e, data) => {
                this.stderr = data ? data.toString() : '';
                this.exitCode = err && err.code || 0;
                process.chdir('../');
                callback(err, stdout, stderr);
            });
        });
    };
};
