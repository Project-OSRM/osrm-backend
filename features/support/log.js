'use strict';

const fs = require('fs');
const path = require('path');
const mkdirp = require('mkdirp');
const d3 = require('d3-queue');

module.exports = function () {
    this.setupLogs = () => {
        this.logQueue = d3.queue(1);
    };

    this.finishLogs = (callback) => {
        this.logQueue.awaitAll(callback);
    };

    this.setupScenarioLogFile = (featureID, scenarioID, callback) => {
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

    this.finishScenarioLogs = () => {
        function finishStream(logStream, callback) {
          if (err) return callback(err);

          logStream.on('finish', callback);
          logStream.end();
        }
        this.logQueue.defer(finishStream.bind(this, this.logStream));
        this.logStream = null;
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
