'use strict';

var d3 = require('d3-queue');
var path = require('path');
var mkdirp = require('mkdirp');
var rimraf = require('rimraf');
var OSM = require('../lib/osm');
var OSRMLoader = require('../lib/osrm_loader');

module.exports = function () {
    this.registerHandler('BeforeFeatures', {timeout: 30000},  (features, callback) => {
        this.osrmLoader = new OSRMLoader(this);
        this.OSMDB = new OSM.DB();

        let queue = d3.queue(1);
        queue.defer(this.initializeEnv.bind(this));
        queue.defer(this.verifyOSRMIsNotRunning.bind(this));
        queue.defer(this.verifyExistenceOfBinaries.bind(this));
        queue.defer(this.initializeCache.bind(this));
        queue.defer(this.setupFeatures.bind(this, features));
        queue.awaitAll(callback);
    });

    this.BeforeFeature((feature, callback) => {
        this.profile = this.OSRM_PROFILE || this.DEFAULT_PROFILE;
        this.profileFile = path.join(this.PROFILES_PATH, this.profile + '.lua');
        this.setupFeatureCache(feature);
        callback();
    });

    this.Before((scenario, callback) => {
        this.osrmLoader.setLoadMethod(this.DEFAULT_LOAD_METHOD);
        this.setGridSize(this.DEFAULT_GRID_SIZE);
        this.setOrigin(this.DEFAULT_ORIGIN);
        this.queryParams = {};
        this.extractArgs = '';
        this.contractArgs = '';
        this.partitionArgs = '';
        this.customizeArgs = '';
        this.loaderArgs = '';
        this.environment = Object.assign(this.DEFAULT_ENVIRONMENT);
        this.resetOSM();

        this.scenarioID = this.getScenarioID(scenario);
        this.setupScenarioCache(this.scenarioID);

        // setup output logging
        let logDir = path.join(this.LOGS_PATH, this.featureID);
        this.scenarioLogFile = path.join(logDir, this.scenarioID) + '.log';
        d3.queue(1)
            .defer(mkdirp, logDir)
            .defer(rimraf, this.scenarioLogFile)
            .awaitAll(callback);
        // uncomment to get path to logfile
        // console.log('  Writing logging output to ' + this.scenarioLogFile);
    });

    this.After((scenario, callback) => {
        this.resetOptionsOutput();
        callback();
    });

    this.AfterFeatures((features, callback) => {
        callback();
    });
};
