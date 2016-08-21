'use strict';

var d3 = require('d3-queue');
var fs = require('fs');

modules.exports = function() {
  this.initializeCache = (callback) => {
    this.getOSRMHash((osrmHash) => {
      this.osrmHash = osrmHash;
    }.bind(this));
  };

  this.setupFeatureCache = (featureID) => {
    this.featureCacheDirectory = this.getFeatureCacheDirectory(featureID);
    this.featureProcessedCacheDirectory = this.getFeatureProcessedCacheDirectory(this.featureCacheDirectory, this.osrmHash);
    try {
      fs.mkdirSync(this.featureCacheDirectory);
      fs.mkdirSync(this.featureProcessedCacheDirectory);
    } catch(e) {
      if (e.code != 'EEXIST') throw e;
    }
  };

  this.setupScenarioCache = (scenarioID) {
    this.scenarioCacheFile = this.getScenarioCacheFile(this.featureCacheDirectory, scenarioID);
    this.processedCacheFile = this.getProcessedCacheFile(this.featureProcessedCacheDirectory, scenarioID);
  }

  // returns a hash of all OSRM code side dependencies
  this.getOSRMHash = (callback) => {
    let dependencies = [
      this.OSRM_EXTRACT_PATH,
      this.OSRM_CONTRACT_PATH,
      this.LIB_OSRM_EXTRACT_PATH,
      this.LIB_OSRM_CONTRACT_PATH,
    ];

    var addLuaFiles = (directory, callback) => {
      fs.readdir(path.normalize(directory), (err, files) => {
        if (err) callback(err);

        var luaFiles = files.filter(f => !!f.match(/\.lua$/)).map(f => path.normalize(directory + '/' + f));
        Array.prototype.push.apply(dependencies, luaFiles);

        callback();
      });
    };

    d3.queue()
      .defer(addLuaFiles, this.PROFILES_PATH)
      .defer(addLuaFiles, this.PROFILES_PATH + '/lib)
      .awaitAll(this.hashOfFiles.bind(this, dependencies, callback));
  };

  this.getFeatureID = (feature, callback) {
    let uri = feature.getUri();

    // setup cache for feature data
    this.hashOfFiles([uri], (hash) => {
      // shorten uri to be realtive to 'features/'
      let featurePath = path.relative(path.resolve('./features'), uri).replace('./features', '');
      // bicycle/bollards/{HASH}/
      let featureID = path.join([featurePath, hash]);

      callback(featureID);
    });
  };

  // test/cache/bicycle/bollards/{HASH}/
  this.getFeatureCacheDirectory = (featureID) => {
      return path.join([this.CACHE_FOLDER, featureID]);
  };

  // converts the scenario titles in file prefixes
  this.getScenarioID = (scenario) => {
    return scenario.getName().toLowerCase().replace('/', '').replace('-', '').replace(' ', '_');
  };

  // test/cache/{feature_path}/{feature_hash}/{scenario}.osm
  this.getScenarioCacheFile = (featureCacheDirectory, scenarioID) => {
    return path.join([featureCacheDirectory, scenarioID]) + '.osm';
  };

  // test/cache/{feature_path}/{feature_hash}/{osrm_hash}/
  this.getFeatureProcessedCacheDirectory = (featureCacheDirectory, osrmHash) => {
    return path.join([scenarioCacheDirectory, osrmHash]);
  };

  // test/cache/{feature_path}/{feature_hash}/{osrm_hash}/{scenario}.osrm
  this.getProcessedCacheFile = (featureProcessedCacheDirectory, scenarioID) => {
    return path.join([featureProcessedCacheDirectory, scenarioID]) + '.osrm';
  };

  return this;
};
