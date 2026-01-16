// Manages test data caching system with hashing for performance optimization
import fs from 'node:fs';
import path from 'node:path';
import util from 'node:util';

import { formatterHelpers } from '@cucumber/cucumber';

export default class Cache {
  constructor(env, scenario) {
    this.env = env;
    // There is one cache directory per feature.
    //
    // The feature cache contains the .osm files cucumber generated from: "Given the
    // node map ... and the ways ..." and all the files osrm-extract and friends
    // generated from it.
    //
    // A hash is generated from all osrm binaries and lua profiles (in
    // env.getOSRMHash()) and the .feature file.  The cache directory is then located at
    // test/cache/car/access.feature/{hash}/

    const uri = scenario.pickle.uri;
    const content = fs.readFileSync(uri);
    const hash = env.osrmHash.copy();
    hash.update(content);
    const hexHash = hash.digest('hex');

    // shorten uri to be relative to 'features/'
    const featurePath = path.relative(path.resolve('./features'), uri);
    /** eg. car/access.feature/{hash}/ */
    this.featureID = path.join(featurePath, hexHash);
    /** eg. test/cache/car/access.feature/{hash}/ */
    this.featureCacheDirectory = path.join(this.env.wp.cachePath, this.featureID);

    // ensure there is a cache directory
    fs.mkdirSync(this.featureCacheDirectory, { recursive: true });
  }

  // converts the scenario titles in file prefixes
  // Cucumber v12 API: testCase parameter contains { gherkinDocument, pickle }
  // Use formatterHelpers.PickleParser.getPickleLocation() to get line numbers like scenario.getLine() in v1
  getScenarioID(testCaseParam) {
    const { gherkinDocument, pickle } = testCaseParam;
    const name = pickle.name
      .toLowerCase()
      .replace(/[/\-'=,():*#]/g, '')
      .replace(/\s/g, '_')
      .replace(/__/g, '_')
      .replace(/\.\./g, '.')
      .substring(0, 64);

    // Get line number using Cucumber v12 API
    const { line } = formatterHelpers.PickleParser.getPickleLocation({
      gherkinDocument,
      pickle,
    });

    return util.format('%d_%s', line, name);
  }

  // test/cache/{feature_path}/{hash}
  getCacheDirectory() {
    return this.featureCacheDirectory;
  }

  // test/cache/{feature_path}/{hash}/42_is_the_answer
  getCacheBaseName(scenario) {
    return path.join(this.featureCacheDirectory, this.getScenarioID(scenario));
  }
}
