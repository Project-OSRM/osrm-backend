var fs = require('fs');

module.exports = function () {
    this.setupScenarioLogFile = (featureID, scenarioID) {
      this.scenarioLogFile = path.join([this.LOGS_PATH, featureID, scenarioID]) + ".log";
      fs.writeFileSync(this.scenarioLogFile, '');
    };

    this.log = (message) => {
        fs.appendFile(this.scenarioLogFile, message + '\n', err => {
            if (err) throw err;
        });
    };
};
