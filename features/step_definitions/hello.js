// Simple step definitions to test basic Cucumber v12 functionality
import { Given, When, Then } from '@cucumber/cucumber';
import assert from 'assert';


Given('I have a hello message', function () {
  this.message = 'Hello World';
});

When('I display the message', function () {
  this.displayedMessage = this.message;
});

Then('I should see {string}', function (expectedMessage) {
  assert.equal(this.displayedMessage, expectedMessage);
});