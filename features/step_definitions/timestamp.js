var assert = require('assert');

module.exports = function () {
    this.Then(/^I should get a valid timestamp/, (callback) => {
        this.ShouldGetAResponse();
        this.ShouldBeValidJSON((err) => {
            this.ShouldBeWellFormed();
            assert.equal(typeof this.json.timestamp, 'string');
            assert.equal(this.json.timestamp, '2000-01-01T00:00:00Z');
            callback(err);
        });
    });
};
