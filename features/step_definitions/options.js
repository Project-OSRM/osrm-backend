var assert = require('assert');
var fs = require('fs');

module.exports = function () {
    this.When(/^I run "osrm\-routed\s?(.*?)"$/, { timeout: this.TIMEOUT }, (options, callback) => {
        this.runBin('osrm-routed', options, () => {
            callback();
        });
    });

    this.When(/^I run "osrm\-extract\s?(.*?)"$/, (options, callback) => {
        this.runBin('osrm-extract', options, () => {
            callback();
        });
    });

    this.When(/^I run "osrm\-contract\s?(.*?)"$/, (options, callback) => {
        this.runBin('osrm-contract', options, () => {
            callback();
        });
    });

    this.When(/^I run "osrm\-datastore\s?(.*?)"$/, (options, callback) => {
        this.runBin('osrm-datastore', options, () => {
            callback();
        });
    });

    this.Then(/^it should exit with code (\d+)$/, (code) => {
        assert.equal(this.exitCode, parseInt(code));
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
        var actualData = fs.readFileSync(this.osmData.extractedFile+'.osrm.datasource_names',{encoding:'UTF-8'}).trim().split("\n").join(",");
        assert.equal(actualData, expectedData);
    });

    this.Given(/^the query options$/, (table, callback) => {
        table.raw().forEach(tuple => {
            this.queryParams[tuple[0]] = tuple[1];
        });

        callback();
    });
};
