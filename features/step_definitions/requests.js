// Step definitions for generic HTTP request testing and response validation
import assert from 'assert';
import { When, Then } from '@cucumber/cucumber';

When(/^I request \/(.*)$/, function (path, callback) {
  this.reprocessAndLoadData((e) => {
    if (e) return callback(e);
    this.requestUrl(path, (err, res, body) => {
      this.response = res;
      this.body = body;
      callback(err, res, body);
    });
  });
});

Then(/^I should get a response/, function () {
  this.ShouldGetAResponse();
});

Then(/^response should be valid JSON$/, function (callback) {
  this.ShouldBeValidJSON(callback);
});

Then(/^response should be well-formed$/, function () {
  this.ShouldBeWellFormed();
});

Then(/^status code should be (.+)$/, function (code, callback) {
  try {
    this.json = JSON.parse(this.body);
  } catch(e) {
    return callback(e);
  }
  assert.equal(this.json.code, code);
  callback();
});

Then(/^status message should be "(.*?)"$/, function (message, callback) {
  try {
    this.json = JSON.parse(this.body);
  } catch(e) {
    return callback(e);
  }
  assert(this.json.status_message, message);
  callback();
});

Then(/^response should be a well-formed route$/, function () {
  this.ShouldBeWellFormed();
  assert.equal(this.json.code, 'ok');
  assert.ok(Array.isArray(this.json.routes));
  assert.ok(Array.isArray(this.json.waypoints));
});

Then(/^"([^"]*)" should return code (\d+)$/, function (binary, code) {
  assert.ok(this.processError instanceof Error);
  assert.equal(this.processError.process, binary);
  assert.equal(parseInt(this.processError.code), parseInt(code));
});