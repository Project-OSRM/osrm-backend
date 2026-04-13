// Step definitions for testing basic routing API functionality
import { When, Given } from '@cucumber/cucumber';


When(/^I route I should get$/, async function (table) {
  await this.WhenIRouteIShouldGet(table);
});

// Runs routing test multiple times for performance testing
When(/^I route (\d+) times I should get$/, async function (n, table) {
  for (let i = 0; i < n; i++) {
    await this.WhenIRouteIShouldGet(table);
  }
});

// Sets skip_waypoints parameter for routing requests
Given(/^skip waypoints$/, function (callback) {
  this.queryParams['skip_waypoints'] = true;
  callback();
});
