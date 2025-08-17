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

  // Set up feature cache for Cucumber v12 API - manually initialize since setupFeatures was called with empty array
  // testCase.pickle.uri replaces scenario.uri from v1
  const uri = testCase.pickle.uri;
  
  // Manually populate feature cache directories if not already done
  if (!this.featureCacheDirectories[uri]) {
    const hash = require('../lib/hash');
    const path = require('path');
    
    // Simplified synchronous setup - just create a basic cache directory structure
    let featurePath = path.relative(path.resolve('./features'), uri);
    let featureID = featurePath.replace(/\//g, '_').replace(/\.feature$/, '');
    let featureCacheDirectory = this.getFeatureCacheDirectory(featureID);
    let featureProcessedCacheDirectory = this.getFeatureProcessedCacheDirectory(featureCacheDirectory, this.osrmHash);
    
    this.featureIDs[uri] = featureID;
    this.featureCacheDirectories[uri] = featureCacheDirectory;
    this.featureProcessedCacheDirectories[uri] = featureProcessedCacheDirectory;
  }
  
  const mockFeature = { getUri: () => uri };
  this.setupFeatureCache(mockFeature);

  this.scenarioID = this.getScenarioID(testCase);
  this.setupScenarioCache(this.scenarioID);

  // setup output logging
  let logDir = path.join(this.LOGS_PATH, this.featureID || 'default');
  this.scenarioLogFile = path.join(logDir, this.scenarioID) + '.log';
  d3.queue(1)
    .defer(createDir, logDir)
    .defer((callback) =>
      fs.rm(this.scenarioLogFile, { force: true }, callback)
    )
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
    supportFunctionsLoaded = true;
  }
});

Before({ timeout: 30000 }, function (testCase, callback) {
  if (!globalInitialized) {
    this.osrmLoader = new OSRMLoader(this);
    this.OSMDB = new OSM.DB();

    let queue = d3.queue(1);
    queue.defer(this.initializeEnv.bind(this));
    queue.defer(this.verifyOSRMIsNotRunning.bind(this));
    queue.defer(this.verifyExistenceOfBinaries.bind(this));
    queue.defer(this.initializeCache.bind(this));
    queue.defer(this.setupFeatures.bind(this, [])); // features not available here
    queue.awaitAll((err) => {
      if (err) return callback(err);
      globalInitialized = true;
      setupCurrentScenario.call(this, testCase, callback);
    });
  } else {
    setupCurrentScenario.call(this, testCase, callback);
  }
});

After(function (scenario, callback) {
  this.resetOptionsOutput();
  callback();
});

AfterAll(function (callback) {
  callback();
});
