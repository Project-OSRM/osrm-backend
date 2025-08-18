// Cucumber before/after hooks for test setup, teardown, and environment initialization
'use strict';

const { BeforeAll, Before, After, AfterAll } = require('@cucumber/cucumber');

// Import the custom World constructor (registers itself via setWorldConstructor)
require('./world');

Before({ timeout: 30000 }, function (testCase, callback) {
  // Initialize the World instance for this test case
  this.init(testCase, callback);
});

After(function (testCase, callback) {
  // Cleanup the World instance after this test case
  this.cleanup(callback);
});

AfterAll(function (callback) {
  callback();
});
