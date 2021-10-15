'use strict';

module.exports = function () {
    this.When(/^I monitor I should get$/, (table, callback) => {
        var got = {};

        this.reprocessAndLoadData((e) => {
            if (e) return callback(e);
            var testRow = (row, ri, cb) => {
                var afterRequest = (err, res) => {
                    if (err) return cb(err);

                    var rows = res.body.split('\n');
                    var metrics = {};
                    rows.forEach((r) => {
                        if (r.includes('{')) {
                            var k = r.split('{')[0];
                            metrics[k] = r.split('}')[1].split(' ')[1];
                        } else {
                            var kv = r.split(' ');
                            metrics[kv[0]] = kv[1];
                        }
                    });

                    var headers = new Set(table.raw()[0]);

                    if (headers.has('key'))
                    {
                        got.key=row['key'];
                        got.value=metrics[row['key']];
                    }


                    if (!metrics.hasOwnProperty('osrm_routed_instance_info')) {
                        throw new Error('Have no instance information inside the monitoring!');
                    }
                    cb(null, got);
                };
                this.requestMonitoring(afterRequest);
            };
            this.processRowsAndDiff(table, testRow, callback);
        });
    });
};
