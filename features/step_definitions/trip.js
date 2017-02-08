var util = require('util');
var polyline = require('polyline');

module.exports = function () {
    function add(a, b) {
        return a + b;
    }

    this.When(/^I plan a trip I should get$/, (table, callback) => {
        var got;

        this.reprocessAndLoadData((e) => {
            if (e) return callback(e);
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
                        got.status = json.code;
                    }

                    if (headers.has('message')) {
                        got.message = json.message;
                    }

                    if (headers.has('geometry')) {
                        if (this.queryParams['geometries'] === 'polyline') {
                            got.geometry = polyline.decode(json.trips[0].geometry).toString();
                        } else if (this.queryParams['geometries'] === 'polyline6') {
                            got.geometry = polyline.decode(json.trips[0].geometry, 6).toString();
                        } else {
                            got.geometry = json.trips[0].geometry.coordinates;
                        }
                    }

                    if (headers.has('#')) {
                        // comment column
                        got['#'] = row['#'];
                    }

                    var subTrips;
                    var trip_durations;
                    var trip_distance;
                    if (res.statusCode === 200) {
                        if (headers.has('trips')) {
                            subTrips = json.trips.filter(t => !!t).map(t => t.legs).map(tl => Array.prototype.concat.apply([], tl.map((sl, i) => {
                                var toAdd = [];
                                if (i === 0) toAdd.push(sl.steps[0].intersections[0].location);
                                toAdd.push(sl.steps[sl.steps.length-1].intersections[0].location);
                                return toAdd;
                            })));
                        }
                        if(headers.has('durations')) {
                            var all_durations = json.trips.filter(t => !!t).map(t => t.legs).map(tl => Array.prototype.concat.apply([], tl.map(sl => {
                                return sl.duration;
                            })));
                            trip_durations = all_durations.map( a => a.reduce(add, 0));
                        }
                        if(headers.has('distance')) {
                            var all_distance = json.trips.filter(t => !!t).map(t => t.legs).map(tl => Array.prototype.concat.apply([], tl.map(sl => {
                                return sl.distance;
                            })));
                            trip_distance = all_distance.map( a => a.reduce(add, 0));
                        }
                    }

                    var ok = true,
                        encodedResult = '',
                        extendedTarget = '';

                    if (json.trips) row.trips.split(',').forEach((sub, si) => {
                        if (si >= subTrips.length) {
                            ok = false;
                        } else {
                            // TODO: Check all rotations of the round trip
                            for (var ni=0; ni<sub.length; ni++) {
                                var node = this.findNodeByName(sub[ni]),
                                    outNode = subTrips[si][ni];
                                if (this.FuzzyMatch.matchLocation(outNode, node)) {
                                    encodedResult += sub[ni];
                                    extendedTarget += sub[ni];
                                } else {
                                    ok = false;
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
                    }

                    got.durations = trip_durations;
                    got.distance = trip_distance;

                    for (var key in row) {
                        if (this.FuzzyMatch.match(got[key], row[key])) {
                            got[key] = row[key];
                        }
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

                        if (row.source) {
                            params.source = got.source = row.source;
                        } 
                        
                        if (row.destination) {
                            params.destination = got.destination = row.destination;
                        } 

                        if (row.hasOwnProperty('roundtrip')) { //roundtrip is a boolean so row.roundtrip alone doesn't work as a check here
                            params.roundtrip = got.roundtrip = row.roundtrip;
                        }

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
