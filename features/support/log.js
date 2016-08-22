'use strict';

const fs = require('fs');
const path = require('path');
const mkdirp = require('mkdirp');

module.exports = function () {
    this.setupScenarioLogFile = (featureID, scenarioID) => {
      let logPath = path.join(this.LOGS_PATH, featureID);
      this.scenarioLogFile = path.join(logPath, scenarioID) + ".log";
      mkdirp.sync(logPath);
      fs.writeFileSync(this.scenarioLogFile, '');
    };

    this.log = (message) => {
        fs.appendFile(this.scenarioLogFile, message + '\n', err => {
            if (err) throw err;
        });
    };
};
