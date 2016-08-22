'use strict';

var util = require('util');
var d3 = require('d3-queue');
var path = require('path');
var OSM = require('../lib/osm');
var OSRMLoader = require('../lib/osrm_loader');

module.exports = function () {
    this.BeforeFeatures((features, callback) => {
        this.osrmLoader = new OSRMLoader(this);
        this.OSMDB = new OSM.DB();

        let queue = d3.queue(1);
        queue.defer(this.initializeEnv.bind(this));
        queue.defer(this.verifyOSRMIsNotRunning.bind(this));
        queue.defer(this.verifyExistenceOfBinaries.bind(this));
        queue.defer(this.initializeCache.bind(this));
        queue.awaitAll(callback);
    });

    this.BeforeFeature((feature, callback) => {
        this.profile = this.DEFAULT_PROFILE;
        this.profileFile = path.join(this.PROFILES_PATH, this.profile + '.lua');
        this.getFeatureID(feature, (featureID) => {
          this.featureID = featureID;
          this.setupFeatureCache(this.featureID);
          callback();
        });
    });

    this.Before((scenario, callback) => {
        this.osrmLoader.setLoadMethod(this.DEFAULT_LOAD_METHOD);
        this.setGridSize(this.DEFAULT_GRID_SIZE);
        this.setOrigin(this.DEFAULT_ORIGIN);
        this.queryParams = {};
        this.extractArgs = '';
        this.contractArgs = '';
        this.resetOSM();

        this.scenarioID = this.getScenarioID(scenario);
        this.setupScenarioCache(this.scenarioID);
        this.setupScenarioLogFile(this.featureID, this.scenarioID);

        callback();
    });
};
