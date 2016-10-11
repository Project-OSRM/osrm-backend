var util = require('util');
var d3 = require('d3-queue');
var classes = require('../support/data_classes');

module.exports = function () {
    this.Then(/^routability should be$/, (table, callback) => {
        this.buildWaysFromTable(table, () => {
            var directions = ['forw','backw','bothw'],
                headers = new Set(Object.keys(table.hashes()[0]));

            if (!directions.some(k => !!headers.has(k))) {
                throw new Error('*** routability table must contain either "forw", "backw" or "bothw" column');
            }

            this.reprocessAndLoadData((e) => {
                if (e) return callback(e);
                var testRow = (row, i, cb) => {
                    var outputRow = Object.assign({}, row);

                    testRoutabilityRow(i, (err, result) => {
                        if (err) return cb(err);
                        directions.filter(d => headers.has(d)).forEach((direction) => {
                            var usingShortcut = false,
                                want = row[direction];
                            // shortcuts are when a test has mapped a value like `foot` to
                            // a value like `5 km/h`, to represent the speed that one
                            // can travel by foot. we check for these and use the mapped to
                            // value for later comparison.
                            if (this.shortcutsHash[row[direction]]) {
                                want = this.shortcutsHash[row[direction]];
                                usingShortcut = row[direction];
                            }

                            switch (true) {
                            case '' === want:
                            case 'x' === want:
                                outputRow[direction] = result[direction].status ?
                                    result[direction].status.toString() : '';
                                break;
                            case /^\d+ s/.test(want):
                                // the result here can come back as a non-number value like
                                // `diff`, but we only want to apply the unit when it comes
                                // back as a number, for tableDiff's literal comparison
                                if (result[direction].time) {
                                    outputRow[direction] = !isNaN(result[direction].time) ?
                                        result[direction].time.toString()+' s' :
                                        result[direction].time.toString() || '';
                                } else {
                                    outputRow[direction] = '';
                                }
                                break;
                            case /^\d+ km\/h/.test(want):
                                if (result[direction].speed) {
                                    outputRow[direction] = !isNaN(result[direction].speed) ?
                                        result[direction].speed.toString()+' km/h' :
                                        result[direction].speed.toString() || '';
                                } else {
                                    outputRow[direction] = '';
                                }
                                break;
                            default:
                                throw new Error(util.format('*** Unknown expectation format: %s', want));
                            }

                            if (this.FuzzyMatch.match(outputRow[direction], want)) {
                                outputRow[direction] = [usingShortcut ? usingShortcut : row[direction]];
                            }
                        });

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
                r.status = res.statusCode === 200 ? 'x' : null;
                if (r.status) {
                    r.route = this.wayList(r.json.routes[0]);
                    r.summary = r.json.routes[0].legs.map(l => l.summary).join(',');

                    if (r.route.split(',')[0] === util.format('w%d', i)) {
                        r.time = r.json.routes[0].duration;
                        r.distance = r.json.routes[0].distance;
                        r.speed = r.time > 0 ? parseInt(3.6 * r.distance / r.time) : null;
                    } else {
                        r.status = null;
                    }
                }

                callback(null, r);
            });
        };

        d3.queue(1)
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

                var sq = d3.queue();

                var parseRes = (key, scb) => {
                    if (result.forw[key] === result.backw[key]) {
                        result.bothw[key] = result.forw[key];
                    // FIXME these time and speed checks are stopgaps for precision errors in how
                    // OSRM returns inconsistent durations for rev/for requests along the same way
                    } else if (key === 'time') {
                        var range = [result.forw[key] - 1, result.forw[key] + 1];
                        if (result.backw[key] >= range[0] && result.backw[key] <= range[1])
                            // usually when we see minor differences here there's an integer
                            // duration value and one that comes back with a .9 or .1 rounding.
                            // This returns the integer one
                            result.bothw[key] = parseInt(result.forw[key]) === result.forw[key] ? result.forw[key] : result.backw[key];
                        else
                            result.bothw[key] = 'diff';
                    } else if (key === 'speed') {
                        if (Math.abs(result.backw.time - result.forw.time) < 0.2) {
                            result.bothw[key] = parseInt(result.forw[key]) === result.forw[key] ? result.forw[key] : result.backw[key];
                        } else {
                            result.bothw[key] = 'diff';
                        }
                    } else {
                        result.bothw[key] = 'diff';
                    }
                    scb();
                };

                ['status', 'time', 'distance', 'speed'].forEach((key) => {
                    sq.defer(parseRes, key);
                });

                sq.awaitAll((err) => { cb(err, result); });
            });
    };
};
