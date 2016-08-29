var assert = require('assert');
var fs = require('fs');

module.exports = function () {
    this.runAndSafeOutput = (binary, options, callback) => {
        this.runBin(binary, this.expandOptions(options), (err, stdout, stderr) => {
            console.log(JSON.stringify(err));
            this.stdout = stdout;
            this.stderr = stderr;
            this.exitCode = err && err.code || 0;
            callback(err);
        });
    }

    this.When(/^I run "osrm\-routed\s?(.*?)"$/, { timeout: this.TIMEOUT }, (options, callback) => {
        this.runAndSafeOutput('osrm-routed', options, callback);
    });

    this.When(/^I run "osrm\-extract\s?(.*?)"$/, (options, callback) => {
        this.runAndSafeOutput('osrm-extract', options, callback);
    });

    this.When(/^I run "osrm\-contract\s?(.*?)"$/, (options, callback) => {
        this.runAndSafeOutput('osrm-contract', options, callback);
    });

    this.When(/^I try to run "osrm\-contract\s?(.*?)"$/, (options, callback) => {
        this.runAndSafeOutput('osrm-contract', options, (err) => { callback(); });
    });

    this.When(/^I run "osrm\-datastore\s?(.*?)"$/, (options, callback) => {
        this.runAndSafeOutput('osrm-datastore', options, callback);
    });

    this.Then(/^it should exit with code (\d+)$/, (code) => {
        assert.equal(this.exitCode, parseInt(code));
    });

    this.Then(/^it should exit with code not (\d+)$/, (code) => {
        assert.notEqual(this.exitCode, parseInt(code));
    });

    this.Then(/^stdout should contain "(.*?)"$/, (str) => {
        assert.ok(this.stdout.indexOf(str) > -1);
    });

    this.Then(/^stderr should contain "(.*?)"$/, (str) => {
        assert.ok(this.stderr.indexOf(str) > -1);
    });

    this.Then(/^stdout should contain \/(.*)\/$/, (regexStr) => {
        var re = new RegExp(regexStr);
        assert.ok(this.stdout.match(re));
    });

    this.Then(/^stderr should contain \/(.*)\/$/, (regexStr) => {
        var re = new RegExp(regexStr);
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

    this.Then(/^datasource names should contain "(.+)"$/, (expectedData) => {
        var actualData = fs.readFileSync(this.osmData.extractedFile + '.osrm.datasource_names', {encoding:'UTF-8'}).trim().split('\n').join(',');
        assert.equal(actualData, expectedData);
    });

    this.Given(/^the query options$/, (table, callback) => {
        table.raw().forEach(tuple => {
            this.queryParams[tuple[0]] = tuple[1];
        });

        callback();
    });
};
