// Support functions for OSRM binary options and output handling
'use strict';

module.exports = function () {
  this.resetOptionsOutput = function () {
    this.stdout = null;
    this.stderr = null;
    this.exitCode = null;
    this.termSignal = null;
  };

  this.runAndSafeOutput = function (binary, options, callback) {
    return this.runBin(
      binary,
      this.expandOptions(options),
      this.environment,
      (err, stdout, stderr) => {
        this.stdout = stdout;
        this.stderr = stderr;
        this.exitCode = (err && err.code) || 0;
        this.termSignal = (err && err.signal) || '';
        callback(err);
      }
    );
  };
};