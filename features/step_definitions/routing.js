var d3 = require('d3-queue');

module.exports = function () {
    this.When(/^I route I should get$/, this.WhenIRouteIShouldGet);

    // This is used to route 100 times; timeout for entire step is therefore set to 100 * STRESS_TIMEOUT
    this.When(/^I route (\d+) times I should get$/, { timeout: 30000 }, (n, table, callback) => {
        var q = d3.queue(1);

        for (var i=0; i<n; i++) {
            q.defer(this.WhenIRouteIShouldGet, table);
        }

        q.awaitAll(callback);
    });
};
