// Cucumber before/after hooks for test setup, teardown, and environment initialization
'use strict';

var d3 = require('d3-queue');
var path = require('path');
var fs = require('fs');
var OSM = require('../lib/osm');
var OSRMLoader = require('../lib/osrm_loader');
const { createDir } = require('../lib/utils');

const { BeforeAll, Before, After, AfterAll } = require('@cucumber/cucumber');

console.log('=== hooks.js file loaded ===');

// Global flags for initialization
let supportFunctionsLoaded = false;
let globalInitialized = false;
let collectedFeatures = new Set(); // Collect unique features from testCases

// Initialization function to set up scenario-specific state
function setupCurrentScenario(testCase, callback) {
  this.profile = this.OSRM_PROFILE || this.DEFAULT_PROFILE;
  this.profileFile = path.join(this.PROFILES_PATH, this.profile + '.lua');
  this.osrmLoader.setLoadMethod(this.DEFAULT_LOAD_METHOD);
  this.setGridSize(this.DEFAULT_GRID_SIZE);
  this.setOrigin(this.DEFAULT_ORIGIN);
  this.queryParams = {};
  this.extractArgs = '';
  this.contractArgs = '';
  this.partitionArgs = '';
  this.customizeArgs = '';
  this.loaderArgs = '';
  this.environment = Object.assign({}, this.DEFAULT_ENVIRONMENT);
  this.resetOSM();

  // Set up feature cache for Cucumber v12 API - recreate original BeforeFeature behavior
  const mockFeature = { getUri: () => testCase.pickle.uri };
  this.setupFeatureCache(mockFeature);

  this.scenarioID = this.getScenarioID(testCase);
  this.setupScenarioCache(this.scenarioID);

  // setup output logging
  let logDir = path.join(this.LOGS_PATH, this.featureID || 'default');
  this.scenarioLogFile = path.join(logDir, this.scenarioID) + '.log';
  d3.queue(1)
    .defer(createDir, logDir)
    .defer((callback) => fs.rm(this.scenarioLogFile, { force: true }, callback))
    .awaitAll(callback);
}

Before(function () {
  console.log('=== Before hook called for loading support functions ===');
  if (!supportFunctionsLoaded) {
    console.log('=== Loading support functions onto World ===');
    require('./env').call(this);
    require('./cache').call(this);
    require('./data').call(this);
    require('./http').call(this);
    require('./run').call(this);
    require('./route').call(this);
    require('./shared_steps').call(this);
    require('./fuzzy').call(this);
    require('../step_definitions/options').call(this);
    supportFunctionsLoaded = true;
  }
});

Before({ timeout: 30000 }, function (testCase, callback) {
  // Collect features from testCases to recreate original BeforeFeatures behavior
  collectedFeatures.add(testCase.pickle.uri);

  if (!globalInitialized) {
    this.osrmLoader = new OSRMLoader(this);
    this.OSMDB = new OSM.DB();

    let queue = d3.queue(1);
    queue.defer(this.initializeEnv.bind(this));
    queue.defer(this.verifyOSRMIsNotRunning.bind(this));
    queue.defer(this.verifyExistenceOfBinaries.bind(this));
    queue.defer(this.initializeCache.bind(this));

    // Recreate original BeforeFeatures behavior - create mock features array from collected URIs
    const mockFeatures = Array.from(collectedFeatures).map((uri) => ({
      getUri: () => uri,
    }));
    queue.defer(this.setupFeatures.bind(this, mockFeatures));

    queue.awaitAll((err) => {
      if (err) return callback(err);
      globalInitialized = true;
      setupCurrentScenario.call(this, testCase, callback);
    });
  } else {
    setupCurrentScenario.call(this, testCase, callback);
  }
});

After(function (testCase, callback) {
  this.resetOptionsOutput();
  callback();
});

AfterAll(function (callback) {
  callback();
});
