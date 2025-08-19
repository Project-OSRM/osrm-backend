// Custom World class for OSRM test environment using modern Cucumber.js v13 patterns
import d3 from 'd3-queue';
import path from 'path';
import fs from 'fs';
import * as OSM from '../lib/osm.js';
import OSRMLoader from '../lib/osrm_loader.js';
import { createDir } from '../lib/utils.js';
import { World, setWorldConstructor } from '@cucumber/cucumber';

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

class OSRMWorld extends World {
  // Private instances of support classes for clean composition
  #env = new Env();
  #cache = new Cache();
  #data = new Data();
  #http = new Http();
  #route = new Route();
  #run = new Run();
  #sharedSteps = new SharedSteps();
  #fuzzy = new Fuzzy();
  #options = new Options();

  constructor(options) {
    // Get built-in Cucumber helpers: this.attach, this.log, this.parameters
    super(options);

    // Initialize core objects
    this.osrmLoader = new OSRMLoader(this);
    this.OSMDB = new OSM.DB();

    // Copy properties that need direct access
    this.FuzzyMatch = this.#fuzzy.FuzzyMatch;

    // Copy all methods and properties from support classes (cleaner than complex delegation)
    this.#copyMethodsFromClass(this.#env);
    this.#copyMethodsFromClass(this.#cache); 
    this.#copyMethodsFromClass(this.#data);
    this.#copyMethodsFromClass(this.#http);
    this.#copyMethodsFromClass(this.#route);
    this.#copyMethodsFromClass(this.#run);
    this.#copyMethodsFromClass(this.#sharedSteps);
    this.#copyMethodsFromClass(this.#options);

    // Initialize Env constants
    this.TIMEOUT = (process.env.CUCUMBER_TIMEOUT && parseInt(process.env.CUCUMBER_TIMEOUT)) || 5000;
    this.ROOT_PATH = process.cwd();
    this.TEST_PATH = path.resolve(this.ROOT_PATH, 'test');
    this.CACHE_PATH = path.resolve(this.TEST_PATH, 'cache');
    this.LOGS_PATH = path.resolve(this.TEST_PATH, 'logs');
    this.PROFILES_PATH = path.resolve(this.ROOT_PATH, 'profiles');
    this.FIXTURES_PATH = path.resolve(this.ROOT_PATH, 'unit_tests/fixtures');
    this.BIN_PATH = (process.env.OSRM_BUILD_DIR && process.env.OSRM_BUILD_DIR) || path.resolve(this.ROOT_PATH, 'build');
    this.DATASET_NAME = 'cucumber';
  }

  // Private helper to copy methods from support classes
  #copyMethodsFromClass(instance) {
    Object.getOwnPropertyNames(Object.getPrototypeOf(instance)).forEach(name => {
      if (name !== 'constructor' && typeof instance[name] === 'function') {
        this[name] = instance[name].bind(this);
      }
    });
  }

  // Initialize the world for a specific test case
  // This method is called from Before hook since constructors can't be async
  init(testCase, callback) {
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
  }

  setupCurrentScenario(testCase, callback) {
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
  }

  // Cleanup method called from After hook
  cleanup(callback) {
    this.resetOptionsOutput();
    if (this.osrmLoader) {
      this.osrmLoader.shutdown(() => {
        callback();
      });
    } else {
      callback();
    }
  }

}

// Register the custom World constructor
setWorldConstructor(OSRMWorld);
