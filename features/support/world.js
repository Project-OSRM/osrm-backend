// Custom World constructor for OSRM test environment
import d3 from 'd3-queue';
import path from 'path';
import fs from 'fs';
import * as OSM from '../lib/osm.js';
import OSRMLoader from '../lib/osrm_loader.js';
import { createDir } from '../lib/utils.js';
import { setWorldConstructor } from '@cucumber/cucumber';

import Env from './env.js';
import Cache from './cache.js';
import Data from './data.js';
import Http from './http.js';
import Route from './route.js';
import Run from './run.js';
import Options from './options.js';
import Fuzzy from './fuzzy.js';
import SharedSteps from './shared_steps.js';

// Global flags for initialization
let collectedFeatures = new Set(); // Collect unique features from testCases

function OSRMWorld(options) {
  // Attach all support functions to the World instance using mixin pattern
  const env = new Env();
  const cache = new Cache();
  const data = new Data();
  const http = new Http();
  const route = new Route();
  const run = new Run();
  const sharedSteps = new SharedSteps();
  const fuzzy = new Fuzzy();
  const optionsClass = new Options();

  // Bind methods to this context
  Object.getOwnPropertyNames(Object.getPrototypeOf(env)).forEach(name => {
    if (name !== 'constructor' && typeof env[name] === 'function') {
      this[name] = env[name].bind(this);
    }
  });
  Object.getOwnPropertyNames(Object.getPrototypeOf(cache)).forEach(name => {
    if (name !== 'constructor' && typeof cache[name] === 'function') {
      this[name] = cache[name].bind(this);
    }
  });
  Object.getOwnPropertyNames(Object.getPrototypeOf(data)).forEach(name => {
    if (name !== 'constructor' && typeof data[name] === 'function') {
      this[name] = data[name].bind(this);
    }
  });
  Object.getOwnPropertyNames(Object.getPrototypeOf(http)).forEach(name => {
    if (name !== 'constructor' && typeof http[name] === 'function') {
      this[name] = http[name].bind(this);
    }
  });
  Object.getOwnPropertyNames(Object.getPrototypeOf(route)).forEach(name => {
    if (name !== 'constructor' && typeof route[name] === 'function') {
      this[name] = route[name].bind(this);
    }
  });
  Object.getOwnPropertyNames(Object.getPrototypeOf(run)).forEach(name => {
    if (name !== 'constructor' && typeof run[name] === 'function') {
      this[name] = run[name].bind(this);
    }
  });
  Object.getOwnPropertyNames(Object.getPrototypeOf(sharedSteps)).forEach(name => {
    if (name !== 'constructor' && typeof sharedSteps[name] === 'function') {
      this[name] = sharedSteps[name].bind(this);
    }
  });
  Object.getOwnPropertyNames(Object.getPrototypeOf(fuzzy)).forEach(name => {
    if (name !== 'constructor' && typeof fuzzy[name] === 'function') {
      this[name] = fuzzy[name].bind(this);
    }
  });
  // Copy properties from instances
  Object.assign(this, fuzzy);
  Object.getOwnPropertyNames(Object.getPrototypeOf(optionsClass)).forEach(name => {
    if (name !== 'constructor' && typeof optionsClass[name] === 'function') {
      this[name] = optionsClass[name].bind(this);
    }
  });

  // Initialize core objects
  this.osrmLoader = new OSRMLoader(this);
  this.OSMDB = new OSM.DB();

  // Initialize the world for a specific test case
  // This method is called from Before hook since constructors can't be async
  this.init = function (testCase, callback) {
    // Collect features from testCases
    collectedFeatures.add(testCase.pickle.uri);

    let queue = d3.queue(1);
    queue.defer(this.initializeEnv.bind(this));
    queue.defer(this.verifyOSRMIsNotRunning.bind(this));
    queue.defer(this.verifyExistenceOfBinaries.bind(this));
    queue.defer(this.initializeCache.bind(this));

    // Create mock features array from collected URIs
    const mockFeatures = Array.from(collectedFeatures).map((uri) => ({
      getUri: () => uri,
    }));
    queue.defer(this.setupFeatures.bind(this, mockFeatures));

    queue.awaitAll((err) => {
      if (err) return callback(err);
      this.setupCurrentScenario(testCase, callback);
    });
  };

  this.setupCurrentScenario = function (testCase, callback) {
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

    // Set up feature cache
    const mockFeature = { getUri: () => testCase.pickle.uri };
    this.setupFeatureCache(mockFeature);

    this.scenarioID = this.getScenarioID(testCase);
    this.setupScenarioCache(this.scenarioID);

    // Setup output logging
    let logDir = path.join(this.LOGS_PATH, this.featureID || 'default');
    this.scenarioLogFile = path.join(logDir, this.scenarioID) + '.log';
    d3.queue(1)
      .defer(createDir, logDir)
      .defer((callback) =>
        fs.rm(this.scenarioLogFile, { force: true }, callback)
      )
      .awaitAll(callback);
  };

  // Cleanup method called from After hook
  this.cleanup = function (callback) {
    this.resetOptionsOutput();
    if (this.osrmLoader) {
      this.osrmLoader.shutdown(() => {
        callback();
      });
    } else {
      callback();
    }
  };
}

// Register the custom World constructor
setWorldConstructor(OSRMWorld);
