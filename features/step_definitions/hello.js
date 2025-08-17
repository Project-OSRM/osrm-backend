// Simple step definitions to test basic Cucumber v12 functionality
'use strict';

const { Given, When, Then } = require('@cucumber/cucumber');
const assert = require('assert');

console.log('=== hello.js step definitions file loaded ===');

Given('I have a hello message', function () {
  this.message = 'Hello World';
});

When('I display the message', function () {
  this.displayedMessage = this.message;
});

Then('I should see {string}', function (expectedMessage) {
  assert.equal(this.displayedMessage, expectedMessage);
});