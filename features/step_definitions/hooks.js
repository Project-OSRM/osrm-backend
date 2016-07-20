var util = require('util');

module.exports = function () {
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
};
