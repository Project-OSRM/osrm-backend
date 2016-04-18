var util = require('util');

module.exports = function () {
    this.BeforeFeatures((features, callback) => {
        this.pid = null;
        this.initializeEnv(() => {
            this.initializeOptions(callback);
        });
    });

    this.Before((scenario, callback) => {
        this.scenarioTitle = scenario.getName();

        this.loadMethod = this.DEFAULT_LOAD_METHOD;
        this.queryParams = {};
        var d = new Date();
        this.scenarioTime = util.format('%d-%d-%dT%s:%s:%sZ', d.getFullYear(), d.getMonth()+1, d.getDate(), d.getHours(), d.getMinutes(), d.getSeconds());
        this.resetData();
        this.hasLoggedPreprocessInfo = false;
        this.hasLoggedScenarioInfo = false;
        this.setGridSize(this.DEFAULT_GRID_SIZE);
        this.setOrigin(this.DEFAULT_ORIGIN);
        callback();
    });

    this.After((scenario, callback) => {
        this.setExtractArgs('', () => {
            this.setContractArgs('', () => {
                if (this.loadMethod === 'directly' && !!this.OSRMLoader.loader) this.OSRMLoader.shutdown(callback);
                else callback();
            });
        });
    });

    this.Around('@stress', (scenario, callback) => {
        // TODO implement stress timeout? Around support is being dropped in cucumber-js anyway
        callback();
    });
};
