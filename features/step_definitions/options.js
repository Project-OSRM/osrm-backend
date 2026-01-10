// Step definitions for testing OSRM binary options and command-line functionality

import assert from 'assert';
import fs from 'fs';
import { When, Then, Given } from '@cucumber/cucumber';

import { runBinSync } from '../support/run.js';

When(/^I run "osrm-routed\s?(.*?)"$/, function (options, callback) {
  const child = runBinSync(
    'osrm-routed',
    this.expandOptions(options),
    { env : this.environment },
    this.log
  );
  this.saveChildOutput(child);
  callback();
});

When(
  /^I run "osrm-(extract|contract|partition|customize)\s?(.*?)"$/,
  function (binary, options, callback) {
    const child = runBinSync(
      `osrm-${binary}`,
      this.expandOptions(options),
      { env : this.environment },
      this.log
    );
    this.saveChildOutput(child);
    if (child.error != null)
      return callback(child.error);
    const stamp = `${this.osrmCacheFile}.stamp_${binary}`;
    fs.writeFile(stamp, 'ok', callback);
  }
);

When(
  /^I try to run "(osrm-[a-z]+)\s?(.*?)"$/,
  function (binary, options, callback) {
    const child = runBinSync(
      binary,
      this.expandOptions(options),
      { env : this.environment },
      this.log
    );
    this.saveChildOutput(child);
    callback();
  },
);

When(
  /^I run "osrm-datastore\s?(.*?)"(?: with input "([^"]*)")?$/,
  function (args, input, callback) {
    const options = { env : this.environment };
    if (input != null) // Check for both null and undefined
      options.input = input;
    const child = runBinSync(
      'osrm-datastore',
      this.expandOptions(args),
      options,
      this.log
    );
    this.saveChildOutput(child);
    callback();
  },
);

Then(/^it should exit successfully$/, function () {
  assert.equal(this.exitCode, 0);
  assert.equal(this.termSignal, null);
});

Then(/^it should exit with an error$/, function () {
  assert.ok(this.exitCode !== 0 || this.termSignal);
});

Then(/^stdout should( not)? contain "(.*?)"$/, function (not, str) {
  const contains = this.stdout.indexOf(str) > -1;
  const isNegative = not != null; // Check for both null and undefined

  assert.ok(
    isNegative ? !contains : contains,
    `stdout ${isNegative ? 'contains' : 'does not contain'} "${str}"`,
  );
});

Then(/^stderr should( not)? contain "(.*?)"$/, function (not, str) {
  const contains = this.stderr.indexOf(str) > -1;
  const isNegative = not != null; // Check for both null and undefined

  assert.ok(
    isNegative ? !contains : contains,
    `stderr ${isNegative ? 'contains' : 'does not contain'} "${str}"`,
  );
});

Then(/^stdout should contain \/(.*)\/$/, function (regexStr) {
  const re = new RegExp(regexStr);
  assert.ok(this.stdout.match(re));
});

Then(/^stderr should contain \/(.*)\/$/, function (regexStr) {
  const re = new RegExp(regexStr);
  assert.ok(this.stdout.match(re));
});

Then(/^stdout should be empty$/, function () {
  assert.equal(this.stdout.trim(), '');
});

Then(/^stderr should be empty$/, function () {
  assert.equal(this.stderr.trim(), '');
});

Then(/^stdout should contain (\d+) lines?$/, function (lines) {
  assert.equal(this.stdout.split('\n').length - 1, parseInt(lines));
});

Then(/^stderr should contain (\d+) lines?$/, function (lines) {
  assert.equal(this.stderr.split('\n').length - 1, parseInt(lines));
});

Given(/^the query options$/, function (table, callback) {
  table.raw().forEach((tuple) => {
    this.queryParams[tuple[0]] = tuple[1];
  });

  callback();
});
