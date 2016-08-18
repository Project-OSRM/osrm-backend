var util = require('util');
var async = require('async');
var OSM = require('./build_osm');
var classes = require('./data_classes');

module.exports = function () {
    this.BeforeFeatures((features, callback) => {
        this.OSRMLoader = this._OSRMLoader();
        this.OSMDB = new OSM.DB();
        this.osmData = new classes.OSMData(this);
        this.pid = null;

        async.series([
          this.initializeEnv.bind(this),
          this.clearLogFiles.bind(this),
          this.verifyOSRMIsNotRunning.bind(this),
          this.verifyExistenceOfBinaries.bind(this),
          this.initializeCache.bind(this)
        ], callback);
    });

    this.BeforeFeature((feature, callback) => {
        this.profile = this.DEFAULT_PROFILE;
    });

    this.Before((scenario, callback) => {
        this.loadMethod = this.DEFAULT_LOAD_METHOD;
        this.setGridSize(this.DEFAULT_GRID_SIZE);
        this.setOrigin(this.DEFAULT_ORIGIN);
        this.queryParams = {};
        this.resetOSM();

        let scenarioPrefix = this.getScenarioPrefix(scenario);
        let scenarioCacheFile = this.getScenarioCacheFile();

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
};
