var util = require('util');

module.exports = function () {
    this.When(/^I plan a trip I should get$/, (table, callback) => {
        var got;

        this.reprocessAndLoadData(() => {
            var testRow = (row, ri, cb) => {
                var afterRequest = (err, res) => {
                    if (err) return cb(err);
                    var headers = new Set(table.raw()[0]);

                    for (var k in row) {
                        var match = k.match(/param:(.*)/);
                        if (match) {
                            if (row[k] === '(nil)') {
                                params[match[1]] = null;
                            } else if (row[k]) {
                                params[match[1]] = [row[k]];
                            }

                            got[k] = row[k];
                        }
                    }

                    var json;
                    if (res.body.length) {
                        json = JSON.parse(res.body);
                    }

                    if (headers.has('status')) {
                        got.status = json.status.toString();
                    }

                    if (headers.has('message')) {
                        got.message = json.status_message;
                    }

                    if (headers.has('#')) {
                        // comment column
                        got['#'] = row['#'];
                    }

                    var subTrips;
                    if (res.statusCode === 200) {
                        if (headers.has('trips')) {
                            subTrips = json.trips.filter(t => !!t).map(t => t.legs).map(tl => Array.prototype.concat.apply([], tl.map((sl, i) => {
                                var toAdd = [];
                                if (i === 0) toAdd.push(sl.steps[0].intersections[0].location);
                                toAdd.push(sl.steps[sl.steps.length-1].intersections[0].location);
                                return toAdd;
                            })));
                        }
                    }

                    var ok = true,
                        encodedResult = '',
                        extendedTarget = '';

                    row.trips.split(',').forEach((sub, si) => {
                        if (si >= subTrips.length) {
                            ok = false;
                        } else {
                            ok = false;
                            // TODO: Check all rotations of the round trip
                            for (var ni=0; ni<sub.length; ni++) {
                                var node = this.findNodeByName(sub[ni]),
                                    outNode = subTrips[si][ni];
                                if (this.FuzzyMatch.matchLocation(outNode, node)) {
                                    encodedResult += sub[ni];
                                    extendedTarget += sub[ni];
                                    ok = true;
                                } else {
                                    encodedResult += util.format('? [%s,%s]', outNode[0], outNode[1]);
                                    extendedTarget += util.format('%s [%d,%d]', sub[ni], node.lat, node.lon);
                                }
                            }
                        }
                    });

                    if (ok) {
                        got.trips = row.trips;
                        got.via_points = row.via_points;
                    } else {
                        got.trips = encodedResult;
                        got.trips = extendedTarget;
                        this.logFail(row, got, { trip: { query: this.query, response: res }});
                    }

                    ok = true;

                    for (var key in row) {
                        if (this.FuzzyMatch.match(got[key], row[key])) {
                            got[key] = row[key];
                        } else {
                            ok = false;
                        }
                    }

                    if (!ok) {
                        this.logFail(row, got, { trip: { query: this.query, response: res }});
                    }

                    cb(null, got);
                };

                if (row.request) {
                    got.request = row.request;
                    this.requestUrl(row.request, afterRequest);
                } else {
                    var params = this.queryParams,
                        waypoints = [];
                    params['steps'] = 'true';
                    if (row.from && row.to) {
                        var fromNode = this.findNodeByName(row.from);
                        if (!fromNode) throw new Error(util.format('*** unknown from-node "%s"', row.from));
                        waypoints.push(fromNode);

                        var toNode = this.findNodeByName(row.to);
                        if (!toNode) throw new Error(util.format('*** unknown to-node "%s"', row.to));
                        waypoints.push(toNode);

                        got = { from: row.from, to: row.to };
                        this.requestTrip(waypoints, params, afterRequest);
                    } else if (row.waypoints) {
                        row.waypoints.split(',').forEach((n) => {
                            var node = this.findNodeByName(n);
                            if (!node) throw new Error(util.format('*** unknown waypoint node "%s"', n.trim()));
                            waypoints.push(node);
                        });
                        got = { waypoints: row.waypoints };
                        this.requestTrip(waypoints, params, afterRequest);
                    } else {
                        throw new Error('*** no waypoints');
                    }
                }
            };

            this.processRowsAndDiff(table, testRow, callback);
        });
    });
};
