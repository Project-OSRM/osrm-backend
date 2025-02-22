'use strict';

var util = require('util');
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

                    got.code = 'unknown';
                    if (res.body.length) {
                        json = JSON.parse(res.body);
                        got.code = json.code;
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

                    var subMatchings = [''],
                        turns = '',
                        route = '',
                        duration = '',
                        annotation = '',
                        geometry = '',
                        OSMIDs = '',
                        alternatives = '';


                    if (res.statusCode === 200) {
                        if (headers.has('matchings')) {
                            subMatchings = [];

                            // find the first matched
                            let start_index = 0;
                            while (start_index < json.tracepoints.length && json.tracepoints[start_index] === null) start_index++;

                            var sub = [];
                            let prev_index = null;
                            for(var i = start_index; i < json.tracepoints.length; i++){
                                if (json.tracepoints[i] === null) continue;

                                let current_index = json.tracepoints[i].matchings_index;

                                if(prev_index !== current_index) {
                                    if (sub.length > 0) subMatchings.push(sub);
                                    sub = [];
                                    prev_index = current_index;
                                }

                                sub.push(json.tracepoints[i].location);
                            }
                            subMatchings.push(sub);
                        }

                        if (headers.has('turns')) {
                            if (json.matchings.length != 1) throw new Error('*** Checking turns only supported for matchings with one subtrace');
                            turns = this.turnList(json.matchings[0]);
                        }

                        if (headers.has('route')) {
                            if (json.matchings.length != 1) throw new Error('*** Checking route only supported for matchings with one subtrace');
                            route = this.wayList(json.matchings[0]);
                        }

                        if (headers.has('duration')) {
                            if (json.matchings.length != 1) throw new Error('*** Checking duration only supported for matchings with one subtrace');
                            duration = json.matchings[0].duration;
                        }

                        // annotation response values are requested by 'a:{type_name}'
                        var found = false;
                        headers.forEach((h) => { if (h.match(/^a:/)) found = true; });
                        if (found) {
                            if (json.matchings.length != 1) throw new Error('*** Checking annotation only supported for matchings with one subtrace');
                            annotation = this.annotationList(json.matchings[0]);
                        }

                        if (headers.has('geometry')) {
                            if (json.matchings.length != 1) throw new Error('*** Checking geometry only supported for matchings with one subtrace');
                            geometry = json.matchings[0].geometry;
                        }

                        if (headers.has('alternatives')) {
                            alternatives = this.alternativesList(json);
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

                    if (headers.has('data_version')) {
                        got.data_version = json.data_version || '';
                    }

                    // if header matches 'a:*', parse out the values for *
                    // and return in that header
                    headers.forEach((k) => {
                        let whitelist = ['duration', 'distance', 'datasources', 'nodes', 'weight'];
                        if (k.match(/^a:/)) {
                            let a_type = k.slice(2);
                            if (whitelist.indexOf(a_type) == -1)
                                return cb(new Error('Unrecognized annotation field:' + a_type));
                            if (!annotation[a_type])
                                return cb(new Error('Annotation not found in response: ' + a_type));
                            got[k] = annotation[a_type];
                        }
                    });

                    if (headers.has('geometry')) {
                        if (this.queryParams['geometries'] === 'polyline') {
                            got.geometry = polyline.decode(geometry).toString();
                        } else if (this.queryParams['geometries'] === 'polyline6') {
                            got.geometry = polyline.decode(geometry,6).toString();
                        } else {
                            got.geometry = geometry.coordinates;
                        }
                    }

                    if (headers.has('OSM IDs')) {
                        got['OSM IDs'] = OSMIDs;
                    }

                    if (headers.has('alternatives')) {
                        got['alternatives'] = alternatives;
                    }
                    var ok = true;
                    var encodedResult = '',
                        extendedTarget = '',
                        resultWaypoints = [];

                    var testSubMatching = (sub, si) => {
                        var testSubNode = (ni) => {
                            var node = this.findNodeByName(sub[ni]),
                                outNode = subMatchings[si][ni];

                            if (this.FuzzyMatch.matchLocation(outNode, node)) {
                                encodedResult += sub[ni];
                                extendedTarget += sub[ni];
                            } else {
                                if (outNode != null) {
                                    encodedResult += util.format('? [%s,%s]', outNode[0], outNode[1]);
                                } else {
                                    encodedResult += '?';
                                }
                                extendedTarget += util.format('%s [%d,%d]', node.lat, node.lon);
                                ok = false;
                            }
                        };

                        for (var i=0; i<sub.length; i++) {
                            testSubNode(i);
                        }
                    };

                    if (headers.has('matchings')) {
                        if (subMatchings.length != row.matchings.split(',').length) {
                            return cb(new Error('*** table matchings and api response are not the same'));
                        }

                        row.matchings.split(',').forEach((sub, si) => {
                            testSubMatching(sub, si);
                        });
                    }

                    if (headers.has('waypoints')) {
                        var got_loc = [];
                        for (let i = 0; i < json.tracepoints.length; i++) {
                            if (!json.tracepoints[i]) continue;
                            if (json.tracepoints[i].waypoint_index != null)
                                got_loc.push(json.tracepoints[i].location);
                        }

                        if (row.waypoints.length != got_loc.length)
                            return cb(new Error(`Expected ${row.waypoints.length} waypoints, got ${got_loc.length}`));

                        for (i = 0; i < row.waypoints.length; i++)
                        {
                            var want_node = this.findNodeByName(row.waypoints[i]);
                            if (!this.FuzzyMatch.matchLocation(got_loc[i], want_node)) {
                                resultWaypoints.push(util.format('? [%s,%s]', got_loc[i][0], got_loc[i][1]));
                                ok = false;
                            } else {
                                resultWaypoints.push(row.waypoints[i]);
                            }
                        }
                    }

                    if (ok) {
                        if (headers.has('matchings')) {
                            got.matchings = row.matchings;
                        }

                        if (headers.has('timestamps')) {
                            got.timestamps = row.timestamps;
                        }

                        if (headers.has('waypoints')) {
                            got.waypoints = row.waypoints;
                        }
                    } else {
                        got.waypoints = resultWaypoints.join(';');
                        got.matchings = encodedResult;
                        row.matchings = extendedTarget;
                    }

                    cb(null, got);
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
                            if (!node) throw new Error(util.format('*** unknown waypoint node "%s"', n));
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
