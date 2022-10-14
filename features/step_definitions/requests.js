var assert = require('assert');

module.exports = function () {
    this.When(/^I request \/(.*)$/, (path, callback) => {
        this.reprocessAndLoadData((e) => {
            if (e) return callback(e);
            this.requestPath(path, {}, (err, res, body) => {
                this.response = res;
                callback(err, res, body);
            });
        });
    });

    this.Then(/^I should get a response/, () => {
        this.ShouldGetAResponse();
    });

    this.Then(/^response should be valid JSON$/, (callback) => {
        this.ShouldBeValidJSON(callback);
    });

    this.Then(/^response should be well-formed$/, () => {
        this.ShouldBeWellFormed();
    });

    this.Then(/^status code should be (\d+)$/, (code, callback) => {
        try {
            this.json = JSON.parse(this.response.body);
        } catch(e) {
            return callback(e);
        }
        assert.equal(this.json.status, parseInt(code));
        callback();
    });

    this.Then(/^status message should be "(.*?)"$/, (message, callback) => {
        try {
            this.json = JSON.parse(this.response.body);
        } catch(e) {
            return callback(e);
        }
        assert(this.json.status_message, message);
        callback();
    });

    this.Then(/^response should be a well-formed route$/, () => {
        this.ShouldBeWellFormed();
        assert.equal(this.json.code, 'ok');
        assert.ok(Array.isArray(this.json.routes));
        assert.ok(Array.isArray(this.json.waypoints));
    });

    this.Then(/^"([^"]*)" should return code (\d+)$/, (binary, code) => {
        assert.ok(this.processError instanceof Error);
        assert.equal(this.processError.process, binary);
        assert.equal(parseInt(this.processError.code), parseInt(code));
    });
};
