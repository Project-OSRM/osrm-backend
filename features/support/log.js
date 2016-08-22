'use strict';

const fs = require('fs');
const path = require('path');
const mkdirp = require('mkdirp');
const d3 = require('d3-queue');

module.exports = function () {
    this.setupScenarioLogFile = (featureID, scenarioID, callback) => {
        let logPath = path.join(this.LOGS_PATH, featureID);
        this.scenarioLogFile = path.join(logPath, scenarioID) + '.log';

        d3.queue(1)
          .defer(mkdirp, logPath)
          .defer(fs.writeFile, this.scenarioLogFile, '')
          .awaitAll(callback);
    };

    this.log = (message) => {
        fs.appendFile(this.scenarioLogFile, message, err => {
            if (err) throw err;
        });
    };
};
