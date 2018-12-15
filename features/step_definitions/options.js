'use strict';

const assert = require('assert');
const fs = require('fs');

module.exports = function () {
    this.resetOptionsOutput = () => {
        this.stdout = null;
        this.stderr = null;
        this.exitCode = null;
        this.termSignal = null;
    };

    this.runAndSafeOutput = (binary, options, callback) => {
        return this.runBin(binary, this.expandOptions(options), this.environment, (err, stdout, stderr) => {
            this.stdout = stdout;
            this.stderr = stderr;
            this.exitCode = err && err.code || 0;
            this.termSignal = err && err.signal || '';
            callback(err);
        });
    };

    this.When(/^I run "osrm-routed\s?(.*?)"$/, { timeout: this.TIMEOUT }, (options, callback) => {
        this.runAndSafeOutput('osrm-routed', options, callback);
    });

    this.When(/^I run "osrm-(extract|contract|partition|customize)\s?(.*?)"$/, (binary, options, callback) => {
        const stamp = this.processedCacheFile + '.stamp_' + binary;
        this.runAndSafeOutput('osrm-' + binary, options, (err) => {
            if (err) return callback(err);
            fs.writeFile(stamp, 'ok', callback);
        });
    });

    this.When(/^I try to run "(osrm-[a-z]+)\s?(.*?)"$/, (binary, options, callback) => {
        this.runAndSafeOutput(binary, options, () => { callback(); });
    });

    this.When(/^I run "osrm-datastore\s?(.*?)"(?: with input "([^"]*)")?$/, (options, input, callback) => {
        let child = this.runAndSafeOutput('osrm-datastore', options, callback);
        if (input !== undefined)
            child.stdin.write(input);
    });

    this.Then(/^it should exit successfully$/, () => {
        assert.equal(this.exitCode, 0);
        assert.equal(this.termSignal, '');
    });

    this.Then(/^it should exit with an error$/, () => {
        assert.ok(this.exitCode !== 0 || this.termSignal);
    });

    this.Then(/^stdout should( not)? contain "(.*?)"$/, (not, str) => {
        const contains = this.stdout.indexOf(str) > -1;
        assert.ok(typeof not === 'undefined' ? contains : !contains,
            'stdout ' + (typeof not === 'undefined' ? 'does not contain' : 'contains') + ' "' + str + '"');
    });

    this.Then(/^stderr should( not)? contain "(.*?)"$/, (not, str) => {
        const contains = this.stderr.indexOf(str) > -1;
        assert.ok(typeof not === 'undefined' ? contains : !contains,
            'stderr ' + (typeof not === 'undefined' ? 'does not contain' : 'contains') + ' "' + str + '"');
    });

    this.Then(/^stdout should contain \/(.*)\/$/, (regexStr) => {
        const re = new RegExp(regexStr);
        assert.ok(this.stdout.match(re));
    });

    this.Then(/^stderr should contain \/(.*)\/$/, (regexStr) => {
        const re = new RegExp(regexStr);
        assert.ok(this.stdout.match(re));
    });

    this.Then(/^stdout should be empty$/, () => {
        assert.equal(this.stdout.trim(), '');
    });

    this.Then(/^stderr should be empty$/, () => {
        assert.equal(this.stderr.trim(), '');
    });

    this.Then(/^stdout should contain (\d+) lines?$/, (lines) => {
        assert.equal(this.stdout.split('\n').length - 1, parseInt(lines));
    });

    this.Then(/^stderr should contain (\d+) lines?$/, (lines) => {
        assert.equal(this.stderr.split('\n').length - 1, parseInt(lines));
    });

    this.Given(/^the query options$/, (table, callback) => {
        table.raw().forEach(tuple => {
            this.queryParams[tuple[0]] = tuple[1];
        });

        callback();
    });
};
