'use strict';

const fs = require('fs');
const path = require('path');
const mkdirp = require('mkdirp');
const d3 = require('d3-queue');

module.exports = function () {
    this.setupScenarioLogFile = (featureID, scenarioID, callback) => {
        this.logQueue = d3.queue(1);

        let logPath = path.join(this.LOGS_PATH, featureID);
        let scenarioLogFile = path.join(logPath, scenarioID) + '.log';

        function openLogFile(file, callback) {
          this.logStream = fs.createWriteStream(file);
          this.logStream.on('open', () => { callback(); })
          this.logStream.on('error', (err) => { throw err; })
        }


        d3.queue(1)
          .defer(mkdirp, logPath)
          .defer(openLogFile.bind(this), scenarioLogFile)
          .awaitAll(callback);
    };

    this.finishScenarioLogs = (callback) => {
        function finishStream(err) {
          if (err) return callback(err);

          this.logStream.on('finish', callback);
          this.logStream.end();
          this.logStream = null;
        }
        this.logQueue.awaitAll(finishStream.bind(this));
    };

    this.log = (message) => {
        function writeToStream(message, callback) {
          let wrote = this.logStream.write(message);
          if (wrote) return callback();

          this.logStream.once('drain', writeToStream.bind(this, message, callback));
        }
        this.logQueue.defer(writeToStream.bind(this), message);
    };
};
