'use strict';

var util = require('util');
var assert = require('assert');

module.exports = function () {
    this.ShouldGetAResponse = () => {
        assert.equal(this.response.statusCode, 200);
        assert.ok(this.response.body);
        assert.ok(this.response.body.length);
    };

    this.ShouldBeValidJSON = (callback) => {
        try {
            this.json = JSON.parse(this.response.body);
            callback();
        } catch (e) {
            callback(e);
        }
    };

    this.ShouldBeWellFormed = () => {
        assert.equal(typeof this.json.status, 'number');
    };

    this.WhenIRouteIShouldGet = (table, callback) => {
        this.reprocessAndLoadData(() => {
            var headers = new Set(table.raw()[0]);

            var requestRow = (row, ri, cb) => {
                var got;

                var afterRequest = (err, res, body) => {
                    if (err) return cb(err);
                    if (body && body.length) {
                        let pronunciations, instructions, bearings, turns, modes, times, distances, summary, intersections;

                        let json = JSON.parse(body);

                        let hasRoute = json.code === 'Ok';

                        if (hasRoute) {
                            instructions = this.wayList(json.routes[0]);
                            pronunciations = this.pronunciationList(json.routes[0]);
                            bearings = this.bearingList(json.routes[0]);
                            turns = this.turnList(json.routes[0]);
                            intersections = this.intersectionList(json.routes[0]);
                            modes = this.modeList(json.routes[0]);
                            times = this.timeList(json.routes[0]);
                            distances = this.distanceList(json.routes[0]);
                            summary = this.summary(json.routes[0]);
                        }

                        if (headers.has('status')) {
                            got.status = res.statusCode.toString();
                        }

                        if (headers.has('message')) {
                            got.message = json.message || '';
                        }

                        if (headers.has('#')) {
                            // comment column
                            got['#'] = row['#'];
                        }

                        if (headers.has('geometry')) {
                            got.geometry = json.routes[0].geometry;
                        }

                        if (headers.has('route')) {
                            got.route = (instructions || '').trim();

                            if (headers.has('summary')) {
                                got.summary = (summary || '').trim();
                            }

                            if (headers.has('alternative')) {
                                // TODO examine more than first alternative?
                                got.alternative ='';
                                if (json.routes && json.routes.length > 1)
                                    got.alternative = this.wayList(json.routes[1]);
                            }

                            var distance = hasRoute && json.routes[0].distance,
                                time = hasRoute && json.routes[0].duration;

                            if (headers.has('distance')) {
                                if (row.distance.length) {
                                    if (!row.distance.match(/\d+m/))
                                        throw new Error('*** Distance must be specified in meters. (ex: 250m)');
                                    got.distance = instructions ? util.format('%dm', distance) : '';
                                } else {
                                    got.distance = '';
                                }
                            }

                            if (headers.has('time')) {
                                if (!row.time.match(/\d+s/))
                                    throw new Error('*** Time must be specied in seconds. (ex: 60s)');
                                got.time = instructions ? util.format('%ds', time) : '';
                            }

                            if (headers.has('speed')) {
                                if (row.speed !== '' && instructions) {
                                    if (!row.speed.match(/\d+ km\/h/))
                                        throw new Error('*** Speed must be specied in km/h. (ex: 50 km/h)');
                                    var speed = time > 0 ? Math.round(3.6*distance/time) : null;
                                    got.speed = util.format('%d km/h', speed);
                                } else {
                                    got.speed = '';
                                }
                            }

                            if (headers.has('intersections')) {
                                got.intersections = (intersections || '').trim();
                            }

                            var putValue = (key, value) => {
                                if (headers.has(key)) got[key] = instructions ? value : '';
                            };

                            putValue('bearing', bearings);
                            putValue('turns', turns);
                            putValue('modes', modes);
                            putValue('times', times);
                            putValue('distances', distances);
                            putValue('pronunciations', pronunciations);
                        }

                        var ok = true;

                        for (var key in row) {
                            if (this.FuzzyMatch.match(got[key], row[key])) {
                                got[key] = row[key];
                            } else {
                                ok = false;
                            }
                        }

                        if (!ok) {
                            this.logFail(row, got, { route: { query: this.query, response: res }});
                        }

                        cb(null, got);
                    } else {
                        cb(new Error('request failed to return valid body'));
                    }
                };

                if (headers.has('request')) {
                    got = { request: row.request };
                    this.requestUrl(row.request, afterRequest);
                } else {
                    var defaultParams = this.queryParams;
                    var userParams = [];
                    got = {};
                    for (var k in row) {
                        var match = k.match(/param:(.*)/);
                        if (match) {
                            if (row[k] === '(nil)') {
                                userParams.push([match[1], null]);
                            } else if (row[k]) {
                                userParams.push([match[1], row[k]]);
                            }
                            got[k] = row[k];
                        }
                    }

                    var params = this.overwriteParams(defaultParams, userParams),
                        waypoints = [],
                        bearings = [];

                    if (row.bearings) {
                        got.bearings = row.bearings;
                        bearings = row.bearings.split(' ').filter(b => !!b);
                    }

                    if (row.from && row.to) {
                        var fromNode = this.findNodeByName(row.from);
                        if (!fromNode) throw new Error(util.format('*** unknown from-node "%s"'), row.from);
                        waypoints.push(fromNode);

                        var toNode = this.findNodeByName(row.to);
                        if (!toNode) throw new Error(util.format('*** unknown to-node "%s"'), row.to);
                        waypoints.push(toNode);

                        got.from = row.from;
                        got.to = row.to;
                        this.requestRoute(waypoints, bearings, params, afterRequest);
                    } else if (row.waypoints) {
                        row.waypoints.split(',').forEach((n) => {
                            var node = this.findNodeByName(n.trim());
                            if (!node) throw new Error('*** unknown waypoint node "%s"', n.trim());
                            waypoints.push(node);
                        });
                        got.waypoints = row.waypoints;
                        this.requestRoute(waypoints, bearings, params, afterRequest);
                    } else {
                        throw new Error('*** no waypoints');
                    }
                }
            };

            this.processRowsAndDiff(table, requestRow, callback);
        });
    };
};
