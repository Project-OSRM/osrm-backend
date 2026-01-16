// Cucumber before/after hooks for test setup, teardown, and environment initialization
import { BeforeAll, Before, After, AfterAll } from '@cucumber/cucumber';
import { setParallelCanAssign } from '@cucumber/cucumber';

// Import the custom World constructor (registers itself via setWorldConstructor)
import './world.js';
import { env } from './env.js';
import { verifyExistenceOfBinaries } from '../lib/utils.js';
import { testOsrmDown } from '../lib/osrm_loader.js';

import {setDefaultTimeout} from '@cucumber/cucumber';
setDefaultTimeout(parseInt(process.env.CUCUMBER_TIMEOUT || '5000'));

/**
 * A function that assures that an \@isolated scenario will not run while any other
 * scenario is running in parallel.
 */
function isolated (pickleInQuestion, picklesInProgress) {
  for (const tag of pickleInQuestion.tags) {
    if (tag.name === '@isolated')
      return picklesInProgress.length == 0;
  }
  // No other restrictions
  return true;
};

setParallelCanAssign(isolated);

BeforeAll(function () {
  env.beforeAll(this.parameters);
  return Promise.all([
    verifyExistenceOfBinaries(env),
    testOsrmDown()
  ]);
});

Before(function (scenario) {
  for (const t of scenario.pickle.tags) {
    if (t.name.startsWith('@no_')) {
      const tag = t.name.substring(4);
      if (env.wp.loadMethod === tag || env.wp.algorithm === tag || process.platform === tag) {
        return 'skipped';
      }
    }
    if (t.name.startsWith('@with_')) {
      const tag = t.name.substring(6);
      if (env.wp.loadMethod !== tag && env.wp.algorithm !== tag && process.platform !== tag) {
        return 'skipped';
      }
    }
  }
  return this.before(scenario);
});

After(function (scenario) {
  return this.after(scenario);
});

AfterAll(() => {
  return env.afterAll();
});
