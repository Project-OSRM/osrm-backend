var util = require('util');
var d3 = require('d3-queue');
var polyline = require('polyline');

module.exports = function () {
    this.When(/^I match I should get$/, (table, callback) => {
        var got;

        this.reprocessAndLoadData((e) => {
            if (e) return callback(e);
            var testRow = (row, ri, cb) => {
                var afterRequest = (err, res) => {
                    if (err) return cb(err);
                    var json;

                    var headers = new Set(table.raw()[0]);

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

                    var subMatchings = [],
                        turns = '',
                        route = '',
                        duration = '',
                        annotation = '',
                        geometry = '',
                        OSMIDs = '';


                    if (res.statusCode === 200) {
                        if (headers.has('matchings')) {
                            subMatchings = json.matchings.filter(m => !!m).map(sub => sub.matched_points);
                        }

                        if (headers.has('turns')) {
                            if (json.matchings.length != 1) throw new Error('*** Checking turns only supported for matchings with one subtrace');
                            turns = this.turnList(json.matchings[0].instructions);
                        }

                        if (headers.has('route')) {
                            if (json.matchings.length != 1) throw new Error('*** Checking route only supported for matchings with one subtrace');
                            route = this.wayList(json.matchings[0]);
                        }

                        if (headers.has('duration')) {
                            if (json.matchings.length != 1) throw new Error('*** Checking duration only supported for matchings with one subtrace');
                            duration = json.matchings[0].duration;
                        }

                        if (headers.has('annotation')) {
                            if (json.matchings.length != 1) throw new Error('*** Checking annotation only supported for matchings with one subtrace');
                            annotation = this.annotationList(json.matchings[0]);
                        }

                        if (headers.has('geometry')) {
                            if (json.matchings.length != 1) throw new Error('*** Checking geometry only supported for matchings with one subtrace');
                            geometry = json.matchings[0].geometry;
                        }

                        if (headers.has('OSM IDs')) {
                            if (json.matchings.length != 1) throw new Error('*** CHecking annotation only supported for matchings with one subtrace');
                            OSMIDs = this.OSMIDList(json.matchings[0]);
                        }
                    }

                    if (headers.has('turns')) {
                        got.turns = turns;
                    }

                    if (headers.has('route')) {
                        got.route = route;
                    }

                    if (headers.has('duration')) {
                        got.duration = duration.toString();
                    }

                    if (headers.has('annotation')) {
                        got.annotation = annotation.toString();
                    }

                    if (headers.has('geometry')) {
                        if (this.queryParams['geometries'] === 'polyline')
                            got.geometry = polyline.decode(geometry).toString();
                        else
                            got.geometry = geometry;
                    }

                    if (headers.has('OSM IDs')) {
                        got['OSM IDs'] = OSMIDs;
                    }

                    var ok = true;
                    var encodedResult = '',
                        extendedTarget = '';

                    var q = d3.queue();

                    var testSubMatching = (sub, si, scb) => {
                        if (si >= subMatchings.length) {
                            ok = false;
                            q.abort();
                            scb();
                        } else {
                            var sq = d3.queue();

                            var testSubNode = (ni, ncb) => {
                                var node = this.findNodeByName(sub[ni]),
                                    outNode = subMatchings[si][ni];

                                if (this.FuzzyMatch.matchLocation(outNode, node)) {
                                    encodedResult += sub[ni];
                                    extendedTarget += sub[ni];
                                } else {
                                    encodedResult += util.format('? [%s,%s]', outNode[0], outNode[1]);
                                    extendedTarget += util.format('%s [%d,%d]', node.lat, node.lon);
                                    ok = false;
                                }
                                ncb();
                            };

                            for (var i=0; i<sub.length; i++) {
                                sq.defer(testSubNode, i);
                            }

                            sq.awaitAll(scb);
                        }
                    };

                    row.matchings.split(',').forEach((sub, si) => {
                        q.defer(testSubMatching, sub, si);
                    });

                    q.awaitAll(() => {
                        if (ok) {
                            if (headers.has('matchings')) {
                                got.matchings = row.matchings;
                            }

                            if (headers.has('timestamps')) {
                                got.timestamps = row.timestamps;
                            }
                        } else {
                            got.matchings = encodedResult;
                            row.matchings = extendedTarget;
                            this.logFail(row, got, { matching: { query: this.query, response: res } });
                        }

                        cb(null, got);
                    });
                };

                if (row.request) {
                    got = {};
                    got.request = row.request;
                    this.requestUrl(row.request, afterRequest);
                } else {
                    var params = this.queryParams;
                    got = {};
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

                    var trace = [],
                        timestamps = [];

                    if (row.trace) {
                        for (var i=0; i<row.trace.length; i++) {
                            var n = row.trace[i],
                                node = this.findNodeByName(n);
                            if (!node) throw new Error(util.format('*** unknown waypoint node "%s"'), n);
                            trace.push(node);
                        }
                        if (row.timestamps) {
                            timestamps = row.timestamps.split(' ').filter(s => !!s).map(t => parseInt(t, 10));
                        }
                        got.trace = row.trace;
                        this.requestMatching(trace, timestamps, params, afterRequest);
                    } else {
                        throw new Error('*** no trace');
                    }
                }
            };

            this.processRowsAndDiff(table, testRow, callback);
        });
    });
};
