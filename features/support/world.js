// Custom World class for OSRM test environment using modern Cucumber.js v13 patterns
import fs from 'node:fs';
import path from 'node:path';

import { World, setWorldConstructor } from '@cucumber/cucumber';

import * as OSM from '../lib/osm.js';
import { env } from '../support/env.js';
import Cache from './cache.js';
import Data from './data.js';
import Route from './route.js';
import Fuzzy from './fuzzy.js';
import SharedSteps from './shared_steps.js';

class OSRMWorld extends World {
  // Private instances of support classes for clean composition
  #data;
  #route;
  #sharedSteps;
  #fuzzy;

  constructor(options) {
    // Get built-in Cucumber helpers: this.attach, this.log, this.parameters
    super(options);
    this.loadMethod = null;
    this.dataLoaded = false;

    // Initialize service instances with access to world
    this.#data = new Data(this);
    this.#route = new Route(this);
    this.#sharedSteps = new SharedSteps(this);
    this.#fuzzy = new Fuzzy(this);

    // Copy methods from services to world for compatibility
    this.#copyMethodsFromServices();

    // Initialize core objects
    this.OSMDB = new OSM.DB();

    // Copy properties that need direct access
    this.FuzzyMatch = this.#fuzzy.FuzzyMatch;
  }

  // Copy methods from service classes
  #copyMethodsFromServices() {
    [
      this.#data,
      this.#route,
      this.#sharedSteps,
    ].forEach((service) => {
      Object.getOwnPropertyNames(Object.getPrototypeOf(service)).forEach(
        (name) => {
          if (name !== 'constructor' && typeof service[name] === 'function') {
            this[name] = service[name].bind(this);
          }
        },
      );
    });
  }

  // Clean getter access to services
  get data() {
    return this.#data;
  }
  get route() {
    return this.#route;
  }
  get sharedSteps() {
    return this.#sharedSteps;
  }
  get fuzzy() {
    return this.#fuzzy;
  }

  before(scenario) {
    this.cache = new Cache(env, scenario);
    this.setupCurrentScenario(this.cache, scenario);
    this.resetChildOutput();
    return Promise.resolve();
  }

  async after(scenario) {
    if (this.osmCacheFile && fs.existsSync(this.osmCacheFile))
      await this.attach(fs.createReadStream(this.osmCacheFile),
        { mediaType: 'application/osm+xml', fileName: path.basename(this.osmCacheFile) });
    return env.osrmLoader.after(scenario);
  }

  setProfile(profile) {
    this.profile = env.OSRM_PROFILE || profile || env.DEFAULT_PROFILE;
    // Sometimes a profile file needs to be patched. In that case it will be copied into
    // the cache directory and this reference adjusted.
    this.profileFile = path.join(env.wp.profilesPath, `${this.profile}.lua`);
  }

  setupCurrentScenario(cache, scenario) {
    this.setProfile(null);
    this.setGridSize(env.DEFAULT_GRID_SIZE);
    this.setOrigin(env.DEFAULT_ORIGIN);
    this.queryParams = {};
    this.extractArgs   = [];
    this.contractArgs  = [];
    this.partitionArgs = [];
    this.customizeArgs = [];
    this.loaderArgs    = [];
    // environment will be patched eg. for OSRM_RASTER_SOURCE
    this.environment   = Object.assign({}, process.env);
    // this.environment.CUCUMBER_TEST = 'ON';
    // process.report.reportOnSignal = false;
    this.resetOSM();

    const basename = cache.getCacheBaseName(scenario);

    this.osmCacheFile       = `${basename}.osm`;
    this.osrmCacheFile      = `${basename}.osrm`;
    this.rasterCacheFile    = `${basename}_raster.asc`;
    this.speedsCacheFile    = `${basename}_speeds.csv`;
    this.penaltiesCacheFile = `${basename}_penalties.csv`;
    this.profileCacheFile   = `${basename}_profile.lua`;
  }

  /**
   * Saves the output from a completed child process for testing against.
   *
   * @param {child_process} child The child process
   */
  saveChildOutput(child) {
    this.stderr = child.stderr.toString();
    this.stdout = child.stdout.toString();
    this.exitCode = child.status;
    this.termSignal = child.signal;
  }

  resetChildOutput() {
    this.stdout = '';
    this.stderr = '';
    this.exitCode = null;
    this.termSignal = null;
  }

  /**
   * Replaces placeholders in gherkin commands
   *
   * eg. it replaces {osm_file} with the input file path.
   */
  expandOptions(options) {
    const table = {
      'osm_file'          : this.osmCacheFile,
      'processed_file'    : this.osrmCacheFile,
      'profile_file'      : this.profileFile,
      'rastersource_file' : this.rasterCacheFile,
      'speeds_file'       : this.speedsCacheFile,
      'penalties_file'    : this.penaltiesCacheFile,
      'timezone_names'    : process.platform === 'win32' ? 'win' : 'iana'
    };

    function replacer(_match, p1) {
      return table[p1] || p1;
    }

    options = options.replaceAll(/\{(\w+)\}/g, replacer);
    return options.split(/\s+/);
  }
}

// Register the custom World constructor
setWorldConstructor(OSRMWorld);
