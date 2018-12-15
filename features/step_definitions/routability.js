var util = require('util');
var d3 = require('d3-queue');
var classes = require('../support/data_classes');

module.exports = function () {
    this.Then(/^routability should be$/, (table, callback) => {
        this.buildWaysFromTable(table, () => {
            var directions = ['forw','backw','bothw'],
                testedHeaders = ['forw','backw','bothw','forw_rate','backw_rate','bothw_rate'],
                headers = new Set(Object.keys(table.hashes()[0]));

            if (!testedHeaders.some(k => !!headers.has(k))) {
                throw new Error('*** routability table must contain either "forw", "backw", "bothw", "forw_rate" or "backw_mode" column');
            }

            this.reprocessAndLoadData((e) => {
                if (e) return callback(e);
                var testRow = (row, i, cb) => {
                    var outputRow = Object.assign({}, row);
                    // clear the fields that are tested for in the copied response object
                    for (var field in outputRow) {
                        if (testedHeaders.indexOf(field) != -1)
                            outputRow[field] = '';
                    }

                    testRoutabilityRow(i, (err, result) => {
                        if (err) return cb(err);
                        directions.filter(d => headers.has(d + '_rate')).forEach((direction) => {
                            var rate = direction + '_rate';
                            var want = row[rate];

                            switch (true) {
                            case '' === want:
                                outputRow[rate] = result[direction].status ?
                                    result[direction].status.toString() : '';
                                break;
                            case /^\d+(\.\d+){0,1}$/.test(want):
                                if (result[direction].rate !== undefined && !isNaN(result[direction].rate)) {
                                    outputRow[rate] = result[direction].rate.toString();
                                } else {
                                    outputRow[rate] = '';
                                }
                                break;
                            default:
                                throw new Error(util.format('*** Unknown expectation format: %s for header %s', want, rate));
                            }
                        });

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

                            // TODO split out accessible/not accessible value from forw/backw headers
                            // rename forw/backw to forw/backw_speed
                            switch (true) {
                            case '' === want:
                                outputRow[direction] = result[direction].status ?
                                    result[direction].mode : '';
                                break;
                            case 'x' === want:
                                outputRow[direction] = result[direction].status ?
                                    'x' : '';
                                break;
                            case /^[\d.]+ s/.test(want):
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
                                outputRow[direction] = result[direction].mode || '';
                            }

                            if (this.FuzzyMatch.match(outputRow[direction], want)) {
                                outputRow[direction] = usingShortcut ? usingShortcut : row[direction];
                            }
                        });

                        cb(null, outputRow);
                    });
                };
                this.processRowsAndDiff(table, testRow, callback);
            });
        });
    });

    // makes simple a-b request using the given cucumber test routability conditions
    // result is an object containing the calculated values for 'rate', 'status',
    // 'time', 'distance', 'speed' and 'mode', for forwards and backwards routing, as well as
    // a bothw object that diffs forwards/backwards
    var testRoutabilityRow = (i, cb) => {
        var result = {};

        var testDirection = (dir, callback) => {
            var a = new classes.Location(this.origin[0] + (1+this.WAY_SPACING*i) * this.zoom, this.origin[1]),
                b = new classes.Location(this.origin[0] + (3+this.WAY_SPACING*i) * this.zoom, this.origin[1]),
                r = {};

            r.which = dir;

            this.requestRoute((dir === 'forw' ? [a, b] : [b, a]), [], [], this.queryParams, (err, res, body) => {
                if (err) return callback(err);

                r.query = this.query;
                r.json = JSON.parse(body);
                r.code = r.json.code;
                r.status = res.statusCode === 200 ? 'x' : null;
                if (r.status) {
                    r.route = this.wayList(r.json.routes[0]);
                    r.summary = r.json.routes[0].legs.map(l => l.summary).join(',');

                    if (r.route.split(',')[0] === util.format('w%d', i)) {
                        r.time = r.json.routes[0].duration;
                        r.distance = r.json.routes[0].distance;
                        r.rate = Math.round(r.distance / r.json.routes[0].weight * 10) / 10.;
                        r.speed = r.time > 0 ? parseInt(3.6 * r.distance / r.time) : null;

                        // use the mode of the first step of the route
                        // for routability table test, we can assume the mode is the same throughout the route,
                        // since the route is just a single way
                        if( r.json.routes[0].legs[0] && r.json.routes[0].legs[0].steps[0] ) {
                            r.mode = r.json.routes[0].legs[0].steps[0].mode;
                        }
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
                    } else {
                        result.bothw[key] = 'diff';
                    }
                    scb();
                };

                ['rate', 'status', 'time', 'distance', 'speed' ,'mode'].forEach((key) => {
                    sq.defer(parseRes, key);
                });

                sq.awaitAll((err) => { cb(err, result); });
            });
    };
};
