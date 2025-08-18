// Step definitions for testing OSRM binary options and command-line functionality
'use strict';

const assert = require('assert');
const fs = require('fs');
const { When, Then, Given } = require('@cucumber/cucumber');

// TODO: Use global timeout configuration instead of hardcoded value when setDefaultTimeout is implemented
When(
  /^I run "osrm-routed\s?(.*?)"$/,
  { timeout: 5000 },
  function (options, callback) {
    this.runAndSafeOutput('osrm-routed', options, callback);
  }
);

When(
  /^I run "osrm-(extract|contract|partition|customize)\s?(.*?)"$/,
  function (binary, options, callback) {
    const stamp = this.processedCacheFile + '.stamp_' + binary;
    this.runAndSafeOutput('osrm-' + binary, options, (err) => {
      if (err) return callback(err);
      fs.writeFile(stamp, 'ok', callback);
    });
  }
);

When(
  /^I try to run "(osrm-[a-z]+)\s?(.*?)"$/,
  function (binary, options, callback) {
    this.runAndSafeOutput(binary, options, () => {
      callback();
    });
  }
);

When(
  /^I run "osrm-datastore\s?(.*?)"(?: with input "([^"]*)")?$/,
  function (options, input, callback) {
    let child = this.runAndSafeOutput('osrm-datastore', options, callback);
    if (input !== undefined) child.stdin.write(input);
  }
);

Then(/^it should exit successfully$/, function () {
  assert.equal(this.exitCode, 0);
  assert.equal(this.termSignal, '');
});

Then(/^it should exit with an error$/, function () {
  assert.ok(this.exitCode !== 0 || this.termSignal);
});

Then(/^stdout should( not)? contain "(.*?)"$/, function (not, str) {
  const contains = this.stdout.indexOf(str) > -1;
  assert.ok(
    typeof not === 'undefined' ? contains : !contains,
    'stdout ' +
      (typeof not === 'undefined' ? 'does not contain' : 'contains') +
      ' "' +
      str +
      '"'
  );
});

Then(/^stderr should( not)? contain "(.*?)"$/, function (not, str) {
  const contains = this.stderr.indexOf(str) > -1;
  assert.ok(
    typeof not === 'undefined' ? contains : !contains,
    'stderr ' +
      (typeof not === 'undefined' ? 'does not contain' : 'contains') +
      ' "' +
      str +
      '"'
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
