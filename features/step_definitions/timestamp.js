// Step definitions for testing timestamp validation in API responses
import assert from 'assert';
import { Then } from '@cucumber/cucumber';

Then(/^I should get a valid timestamp/, function (callback) {
  this.ShouldGetAResponse();
  this.ShouldBeValidJSON((err) => {
    this.ShouldBeWellFormed();
    assert.equal(typeof this.json.timestamp, 'string');
    assert.equal(this.json.timestamp, '2000-01-01T00:00:00Z');
    callback(err);
  });
});
