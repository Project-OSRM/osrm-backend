'use strict';

const d3 = require('d3-queue');
const fs = require('fs');
const util = require('util');
const path = require('path');
const mkdirp = require('mkdirp');

module.exports = function() {
    this.initializeCache = (callback) => {
        this.getOSRMHash((osrmHash) => {
            this.osrmHash = osrmHash;
            callback();
        });
    };

    this.setupFeatureCache = (featureID, callback) => {
        this.featureCacheDirectory = this.getFeatureCacheDirectory(featureID);
        this.featureProcessedCacheDirectory = this.getFeatureProcessedCacheDirectory(this.featureCacheDirectory, this.osrmHash);

        d3.queue(1)
            .defer(mkdirp, this.featureProcessedCacheDirectory)
            .awaitAll(callback);
    };

    this.setupScenarioCache = (scenarioID) => {
        this.scenarioCacheFile = this.getScenarioCacheFile(this.featureCacheDirectory, scenarioID);
        this.processedCacheFile = this.getProcessedCacheFile(this.featureProcessedCacheDirectory, scenarioID);
        this.inputCacheFile = this.getInputCacheFile(this.featureProcessedCacheDirectory, scenarioID);
        this.rasterCacheFile = this.getRasterCacheFile(this.featureProcessedCacheDirectory, scenarioID);
        this.speedsCacheFile = this.getSpeedsCacheFile(this.featureProcessedCacheDirectory, scenarioID);
        this.penaltiesCacheFile = this.getPenaltiesCacheFile(this.featureProcessedCacheDirectory, scenarioID);
    };

    // returns a hash of all OSRM code side dependencies
    this.getOSRMHash = (callback) => {
        let dependencies = [
            this.OSRM_EXTRACT_PATH,
            this.OSRM_CONTRACT_PATH,
            this.LIB_OSRM_EXTRACT_PATH,
            this.LIB_OSRM_CONTRACT_PATH
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
            .defer(addLuaFiles, this.PROFILES_PATH + '/lib')
            .awaitAll(this.hashOfFiles.bind(this, dependencies, callback));
    };

    this.getFeatureID = (feature, callback) => {
        let uri = feature.getUri();

        // setup cache for feature data
        this.hashOfFiles([uri], (hash) => {
            // shorten uri to be realtive to 'features/'
            let featurePath = path.relative(path.resolve('./features'), uri);
            // bicycle/bollards/{HASH}/
            let featureID = path.join(featurePath, hash);

            callback(featureID);
        });
    };

    // test/cache/bicycle/bollards/{HASH}/
    this.getFeatureCacheDirectory = (featureID) => {
        return path.join(this.CACHE_PATH, featureID);
    };

    // converts the scenario titles in file prefixes
    this.getScenarioID = (scenario) => {
        let name = scenario.getName().toLowerCase().replace(/[\/\-'=,\(\)]/g, '').replace(/\s/g, '_').replace(/__/g, '_').replace(/\.\./g, '.');
        return util.format("%d_%s", scenario.getLine(), name);
    };

    // test/cache/{feature_path}/{feature_hash}/{scenario}_raster.asc
    this.getRasterCacheFile = (featureCacheDirectory, scenarioID) => {
        return path.join(featureCacheDirectory, scenarioID) + '_raster.asc';
    };

    // test/cache/{feature_path}/{feature_hash}/{scenario}_speeds.csv
    this.getSpeedsCacheFile = (featureCacheDirectory, scenarioID) => {
        return path.join(featureCacheDirectory, scenarioID) + '_speeds.csv';
    };

    // test/cache/{feature_path}/{feature_hash}/{scenario}_penalties.csv
    this.getPenaltiesCacheFile = (featureCacheDirectory, scenarioID) => {
        return path.join(featureCacheDirectory, scenarioID) + '_penalties.csv';
    };

    // test/cache/{feature_path}/{feature_hash}/{scenario}.osm
    this.getScenarioCacheFile = (featureCacheDirectory, scenarioID) => {
        return path.join(featureCacheDirectory, scenarioID) + '.osm';
    };

    // test/cache/{feature_path}/{feature_hash}/{osrm_hash}/
    this.getFeatureProcessedCacheDirectory = (featureCacheDirectory, osrmHash) => {
        return path.join(featureCacheDirectory, osrmHash);
    };

    // test/cache/{feature_path}/{feature_hash}/{osrm_hash}/{scenario}.osrm
    this.getProcessedCacheFile = (featureProcessedCacheDirectory, scenarioID) => {
        return path.join(featureProcessedCacheDirectory, scenarioID) + '.osrm';
    };

    // test/cache/{feature_path}/{feature_hash}/{osrm_hash}/{scenario}.osm
    this.getInputCacheFile = (featureProcessedCacheDirectory, scenarioID) => {
        return path.join(featureProcessedCacheDirectory, scenarioID) + '.osm';
    };


    return this;
};
