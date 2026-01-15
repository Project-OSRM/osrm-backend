// Cucumber before/after hooks for test setup, teardown, and environment initialization
import { BeforeAll, Before, After, AfterAll } from '@cucumber/cucumber';

// Import the custom World constructor (registers itself via setWorldConstructor)
import './world.js';

BeforeAll((callback) => {
  callback();
});

Before({ timeout: 30000 }, function (testCase, callback) {
  // Initialize the World instance for this test case
  this.init(testCase, callback);
});

After(function (testCase, callback) {
  // Cleanup the World instance after this test case
  this.cleanup(callback);
});

AfterAll((callback) => {
  callback();
});
