var util = require('util');
var d3 = require('d3-queue');
var classes = require('../support/data_classes');

module.exports = function () {
    this.Then(/^routability should be$/, (table, callback) => {
        this.buildWaysFromTable(table, () => {
            var directions = ['forw','backw','bothw'];

            if (!directions.some(k => !!table.hashes()[0].hasOwnProperty(k))) {
                throw new Error('*** routability table must contain either "forw", "backw" or "bothw" column');
            }
            this.reprocessAndLoadData(() => {
                var testRow = (row, i, cb) => {
                    var outputRow = row;

                    testRoutabilityRow(i, (err, result) => {
                        if (err) return cb(err);
                        directions.filter(d => !!table.hashes()[0][d]).forEach((direction) => {
                            var want = this.shortcutsHash[row[direction]] || row[direction];

                            switch (true) {
                            case '' === want:
                            case 'x' === want:
                                outputRow[direction] = result[direction].status ?
                                    result[direction].status.toString() : '';
                                break;
                            case /^\d+s/.test(want):
                                break;
                            case /^\d+ km\/h/.test(want):
                                break;
                            default:
                                throw new Error(util.format('*** Unknown expectation format: %s', want));
                            }

                            if (this.FuzzyMatch.match(outputRow[direction], want)) {
                                outputRow[direction] = row[direction];
                            }
                        });

                        if (outputRow != row) {
                            this.logFail(row, outputRow, result);
                        }

                        cb(null, outputRow);
                    });
                };

                this.processRowsAndDiff(table, testRow, callback);
            });
        });
    });

    var testRoutabilityRow = (i, cb) => {
        var result = {};

        var testDirection = (dir, callback) => {
            var a = new classes.Location(this.origin[0] + (1+this.WAY_SPACING*i) * this.zoom, this.origin[1]),
                b = new classes.Location(this.origin[0] + (3+this.WAY_SPACING*i) * this.zoom, this.origin[1]),
                r = {};

            r.which = dir;

            this.requestRoute((dir === 'forw' ? [a, b] : [b, a]), [], this.queryParams, (err, res, body) => {
                if (err) return callback(err);

                r.query = this.query;
                r.json = JSON.parse(body);
                r.status = r.json.status === 200 ? 'x' : null;
                if (r.status) {
                    r.route = this.wayList(r.json.route_instructions);

                    if (r.route === util.format('w%d', i)) {
                        r.time = r.json.route_summary.total_time;
                        r.distance = r.json.route_summary.total_distance;
                        r.speed = r.time > 0 ? parseInt(3.6 * r.distance / r.time) : null;
                    } else {
                        r.status = null;
                    }
                }

                callback(null, r);
            });
        };

        d3.queue()
            .defer(testDirection, 'forw')
            .defer(testDirection, 'backw')
            .awaitAll((err, res) => {
                if (err) return cb(err);
                // check if forw and backw returned the same values
                res.forEach((dirRes) => {
                    var which = dirRes.which;
                    delete dirRes.which;
                    result[which] = dirRes;
                });

                result.bothw = {};
                ['status', 'time', 'distance', 'speed'].forEach((key) => {
                    if (result.forw[key] === result.backw[key]) {
                        result.bothw[key] = result.forw[key];
                    } else {
                        result.bothw[key] = 'diff';
                    }
                });

                cb(null, result);
            });
    };
};
