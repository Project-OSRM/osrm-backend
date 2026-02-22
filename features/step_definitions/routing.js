// Step definitions for testing basic routing API functionality
import d3 from 'd3-queue';
import { When, Given } from '@cucumber/cucumber';


When(/^I route I should get$/, function (table, callback) {
  this.WhenIRouteIShouldGet(table, callback);
});

// Runs routing test multiple times for performance testing
When(/^I route (\d+) times I should get$/, function (n, table, callback) {
  const q = d3.queue(1);

  for (let i=0; i<n; i++) {
    q.defer(this.WhenIRouteIShouldGet, table);
  }

  q.awaitAll(callback);
});

// Sets skip_waypoints parameter for routing requests
Given(/^skip waypoints$/, function (callback) {
  this.queryParams['skip_waypoints'] = true;
  callback();
});
