// Manages test data caching system with hashing for performance optimization
import d3 from 'd3-queue';
import fs from 'fs';
import util from 'util';
import path from 'path';
import * as hash from '../lib/hash.js';
import { rm } from 'fs/promises';
import { createDir } from '../lib/utils.js';
import { formatterHelpers } from '@cucumber/cucumber';

export default class Cache {
  constructor() {}

  // Initializes caching system with OSRM binary hash
  initializeCache(callback) {
    this.getOSRMHash((err, osrmHash) => {
      if (err) return callback(err);
      this.osrmHash = osrmHash;
      callback();
    });
  }

  // computes all paths for every feature
  // Sets up cache directories and hashes for all test features
  setupFeatures(features, callback) {
    this.featureIDs = {};
    this.featureCacheDirectories = {};
    this.featureProcessedCacheDirectories = {};
    let queue = d3.queue();

    function initializeFeature(feature, callback) {
      let uri = feature.getUri();

      // setup cache for feature data
      // if OSRM_PROFILE is set to force a specific profile, then
      // include the profile name in the hash of the profile file
      hash.hashOfFile(uri, this.OSRM_PROFILE, (err, hash) => {
        if (err) return callback(err);

        // shorten uri to be realtive to 'features/'
        let featurePath = path.relative(path.resolve('./features'), uri);
        // bicycle/bollards/{HASH}/
        let featureID = path.join(featurePath, hash);

        let featureCacheDirectory = this.getFeatureCacheDirectory(featureID);
        let featureProcessedCacheDirectory = this.getFeatureProcessedCacheDirectory(featureCacheDirectory, this.osrmHash);
        this.featureIDs[uri] = featureID;
        this.featureCacheDirectories[uri] = featureCacheDirectory;
        this.featureProcessedCacheDirectories[uri] = featureProcessedCacheDirectory;

        d3.queue(1)
          .defer(createDir, featureProcessedCacheDirectory)
          .defer(this.cleanupFeatureCache.bind(this), featureCacheDirectory, hash)
          .defer(this.cleanupProcessedFeatureCache.bind(this), featureProcessedCacheDirectory, this.osrmHash)
          .awaitAll(callback);
      });
    }

    for (let i = 0; i < features.length; ++i) {
      queue.defer(initializeFeature.bind(this), features[i]);
    }
    queue.awaitAll(callback);
  }

  cleanupProcessedFeatureCache(directory, osrmHash, callback) {
    let parentPath = path.resolve(path.join(directory, '..'));
    fs.readdir(parentPath, (err, files) => {
      if (err) return callback(err);
      let q = d3.queue();
      files.forEach((f) => {
        let filePath = path.join(parentPath, f);
        fs.stat(filePath, (err, stat) => {
          if (err) return callback(err);
          if (stat.isDirectory() && filePath.search(osrmHash) < 0) {
            rm(filePath, { recursive: true, force: true });
          }
        });
      });
      q.awaitAll(callback);
    });
  }

  cleanupFeatureCache(directory, featureHash, callback) {
    let parentPath = path.resolve(path.join(directory, '..'));
    fs.readdir(parentPath, (err, files) => {
      if (err) return callback(err);
      let q = d3.queue();
      files.filter((name) => name !== featureHash).forEach((f) => {
        rm(path.join(parentPath, f), { recursive: true, force: true });
      });
      q.awaitAll(callback);
    });
  }

  setupFeatureCache(feature) {
    let uri = feature.getUri();
    this.featureID = this.featureIDs[uri];
    this.featureCacheDirectory = this.featureCacheDirectories[uri];
    this.featureProcessedCacheDirectory = this.featureProcessedCacheDirectories[uri];
  }

  setupScenarioCache(scenarioID) {
    this.scenarioCacheFile = this.getScenarioCacheFile(this.featureCacheDirectory, scenarioID);
    this.processedCacheFile = this.getProcessedCacheFile(this.featureProcessedCacheDirectory, scenarioID);
    this.inputCacheFile = this.getInputCacheFile(this.featureProcessedCacheDirectory, scenarioID);
    this.rasterCacheFile = this.getRasterCacheFile(this.featureProcessedCacheDirectory, scenarioID);
    this.speedsCacheFile = this.getSpeedsCacheFile(this.featureProcessedCacheDirectory, scenarioID);
    this.penaltiesCacheFile = this.getPenaltiesCacheFile(this.featureProcessedCacheDirectory, scenarioID);
    this.profileCacheFile = this.getProfileCacheFile(this.featureProcessedCacheDirectory, scenarioID);
  }

  // returns a hash of all OSRM code side dependencies
  getOSRMHash(callback) {
    let dependencies = [
      this.OSRM_EXTRACT_PATH,
      this.OSRM_CONTRACT_PATH,
      this.OSRM_CUSTOMIZE_PATH,
      this.OSRM_PARTITION_PATH,
      this.LIB_OSRM_EXTRACT_PATH,
      this.LIB_OSRM_CONTRACT_PATH,
      this.LIB_OSRM_CUSTOMIZE_PATH,
      this.LIB_OSRM_PARTITION_PATH
    ];

    var addLuaFiles = function (directory, callback) {
      fs.readdir(path.normalize(directory), (err, files) => {
        if (err) return callback(err);

        var luaFiles = files.filter(f => !!f.match(/\.lua$/)).map(f => path.normalize(directory + '/' + f));
        Array.prototype.push.apply(dependencies, luaFiles);

        callback();
      });
    };

    // Note: we need a serialized queue here to ensure that the order of the files
    // passed is stable. Otherwise the hash will not be stable
    d3.queue(1)
      .defer(addLuaFiles, this.PROFILES_PATH)
      .defer(addLuaFiles, this.PROFILES_PATH + '/lib')
      .awaitAll(hash.hashOfFiles.bind(hash, dependencies, callback));
  }

  // test/cache/bicycle/bollards/{HASH}/
  getFeatureCacheDirectory(featureID) {
    return path.join(this.CACHE_PATH, featureID);
  }

  // converts the scenario titles in file prefixes
  // Cucumber v12 API: testCase parameter contains { gherkinDocument, pickle } 
  // Use formatterHelpers.PickleParser.getPickleLocation() to get line numbers like scenario.getLine() in v1
  getScenarioID(testCaseParam) {
    const { gherkinDocument, pickle } = testCaseParam;
    let name = pickle.name.toLowerCase().replace(/[/\-'=,():*#]/g, '')
      .replace(/\s/g, '_').replace(/__/g, '_').replace(/\.\./g, '.')
      .substring(0, 64);
    
    // Get line number using Cucumber v12 API
    const { line } = formatterHelpers.PickleParser.getPickleLocation({ gherkinDocument, pickle });
    
    return util.format('%d_%s', line, name);
  }

  // test/cache/{feature_path}/{feature_hash}/{scenario}_raster.asc
  getRasterCacheFile(featureCacheDirectory, scenarioID) {
    return path.join(featureCacheDirectory, scenarioID) + '_raster.asc';
  }

  // test/cache/{feature_path}/{feature_hash}/{scenario}_speeds.csv
  getSpeedsCacheFile(featureCacheDirectory, scenarioID) {
    return path.join(featureCacheDirectory, scenarioID) + '_speeds.csv';
  }

  // test/cache/{feature_path}/{feature_hash}/{scenario}_penalties.csv
  getPenaltiesCacheFile(featureCacheDirectory, scenarioID) {
    return path.join(featureCacheDirectory, scenarioID) + '_penalties.csv';
  }

  // test/cache/{feature_path}/{feature_hash}/{scenario}_profile.lua
  getProfileCacheFile(featureCacheDirectory, scenarioID) {
    return path.join(featureCacheDirectory, scenarioID) + '_profile.lua';
  }

  // test/cache/{feature_path}/{feature_hash}/{scenario}.osm
  getScenarioCacheFile(featureCacheDirectory, scenarioID) {
    return path.join(featureCacheDirectory, scenarioID) + '.osm';
  }

  // test/cache/{feature_path}/{feature_hash}/{osrm_hash}/
  getFeatureProcessedCacheDirectory(featureCacheDirectory, osrmHash) {
    return path.join(featureCacheDirectory, osrmHash);
  }

  // test/cache/{feature_path}/{feature_hash}/{osrm_hash}/{scenario}.osrm
  getProcessedCacheFile(featureProcessedCacheDirectory, scenarioID) {
    return path.join(featureProcessedCacheDirectory, scenarioID) + '.osrm';
  }

  // test/cache/{feature_path}/{feature_hash}/{osrm_hash}/{scenario}.osm
  getInputCacheFile(featureProcessedCacheDirectory, scenarioID) {
    return path.join(featureProcessedCacheDirectory, scenarioID) + '.osm';
  }
}
