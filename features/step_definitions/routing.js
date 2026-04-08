// Step definitions for testing basic routing API functionality
import { When, Given } from '@cucumber/cucumber';


When(/^I route I should get$/, function (table, callback) {
  this.WhenIRouteIShouldGet(table, callback);
});

// Runs routing test multiple times for performance testing
When(/^I route (\d+) times I should get$/, async function (n, table) {
  for (let i = 0; i < n; i++) {
    await new Promise((resolve, reject) => {
      this.WhenIRouteIShouldGet(table, (err) => {
        if (err) reject(err);
        else resolve();
      });
    });
  }
});

// Sets skip_waypoints parameter for routing requests
Given(/^skip waypoints$/, function (callback) {
  this.queryParams['skip_waypoints'] = true;
  callback();
});
