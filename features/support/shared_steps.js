'use strict';

var util = require('util');
var assert = require('assert');
var polyline = require('polyline');
var fs = require('fs');

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
        this.reprocessAndLoadData((e) => {
            if (e) return callback(e);
            var headers = new Set(table.raw()[0]);

            var requestRow = (row, rowIndex, cb) => {
                var got;

                var afterRequest = (err, res, body) => {
                    console.log(waypoints);
                    if (err) return cb(err);
                    if (body && body.length) {
                        let destinations, exits, pronunciations, instructions, refs, bearings, turns, modes, times, classes,
                            distances, summary, intersections, lanes, locations, annotation, weight_name, weights, approaches,
                            driving_sides;

                        let json = JSON.parse(body);

                        var hasRoute = false;
                        if (this.osrmLoader.method === 'valhalla') {
                            got.code = json.trip.status;
                            hasRoute = json.trip.status == 0;
                        } else {
                            got.code = json.code;
                            hasRoute = json.code === 'Ok';
                        }
                        var route;

                        if (hasRoute) {
                            if (this.osrmLoader.method === 'valhalla') {
                                route = json.trip;
                            } else {
                                route = json.routes[0];
                            }
                            instructions = this.wayList(route);
                            pronunciations = this.pronunciationList(route);
                            refs = this.refList(route);
                            destinations = this.destinationsList(route);
                            exits = this.exitsList(route);
                            bearings = this.bearingList(route);
                            turns = this.turnList(route);
                            intersections = this.intersectionList(route);
                            modes = this.modeList(route);
                            driving_sides = this.drivingSideList(route);
                            classes = this.classesList(route);
                            times = this.timeList(route);
                            distances = this.distanceList(route);
                            lanes = this.lanesList(route);
                            summary = this.summary(route);
                            locations = this.locations(route);
                            annotation = this.annotationList(route);
                            weight_name = this.weightName(route);
                            weights = this.weightList(route);
                            approaches = this.approachList(route);


                            var resultdata = {
                                feature: this.feature.getName(),
                                scenario: this.scenario.getName(),
                                row: rowIndex,
                                expected: {
                                    route: table.hashes()[rowIndex]["route"],
                                    turns: table.hashes()[rowIndex]["turns"],
                                },
                                got: {
                                    route: instructions,
                                    turns: turns
                                }
                            }
                            fs.writeFileSync(`${this.scenarioCacheFile}_${rowIndex}_results.json`,JSON.stringify(resultdata));

                            var geojson = {
                                type: 'FeatureCollection',
                                features: [
                                    {
                                        type: 'Feature',
                                        properties: { type: 'startpoint' },
                                        geometry: {
                                            type: 'Point',
                                            coordinates: [parseFloat(waypoints[0].lon),parseFloat(waypoints[0].lat)]
                                        }
                                    }
                                    ,
                                    {
                                        type: 'Feature',
                                        properties: { type: 'endpoint' },
                                        geometry: {
                                            type: 'Point',
                                            coordinates: [parseFloat(waypoints[1].lon), parseFloat(waypoints[1].lat)]
                                        }
                                    },
                                    {
                                        type: 'Feature',
                                        geometry: {
                                            type: 'LineString',
                                            coordinates: []
                                        },
                                        properties: {}
                                    }
                                ]
                            };

                            // Extract Valhalla route geometry
                            if (this.osrmLoader.method === 'valhalla') {
                                geojson.features[2].geometry.coordinates = polyline.decode(route.legs[0].shape,6).map(c => c.reverse());
                            } else {
                                // OSRM route geometry
                                // TODO: Assume polyline5 for now
                                if (typeof route.geometry === 'string') {
                                    geojson.features[2].geometry.coordinates = polyline.decode(route.geometry).map(c => c.reverse());
                                } else {
                                    geojson.features[2].geometry = route.geometry;
                                }
                            }
                            fs.writeFileSync(`${this.scenarioCacheFile}_${rowIndex}_shape.geojson`,JSON.stringify(geojson));
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
                        }

                        if (headers.has('summary')) {
                            got.summary = (summary || '').trim();
                        }

                        if (headers.has('alternative')) {
                            // TODO examine more than first alternative?
                            got.alternative ='';
                            if (json.routes && json.routes.length > 1)
                                got.alternative = this.wayList(json.routes[1]);
                        }

                        var distance = hasRoute && (this.osrmLoader.method === 'valhalla') ? route.summary.length : route.distance,
                            time = hasRoute && (this.osrmLoader.method === 'valhalla') ? route.summary.time : route.duration,
                            weight = hasRoute && (this.osrmLoader.method === 'valhalla') ? route.summary.time : route.weight;

                        if (headers.has('distance')) {
                            if (row.distance.length) {
                                if (!row.distance.match(/\d+m/))
                                    return cb(new Error('*** Distance must be specified in meters. (ex: 250m)'));
                                got.distance = instructions ? util.format('%dm', distance) : '';
                            } else {
                                got.distance = '';
                            }
                        }

                        if (headers.has('weight')) {
                            if (row.weight.length) {
                                if (!row.weight.match(/[\d\.]+/))
                                    return cb(new Error('*** Weight must be specified as a numeric value. (ex: 8)'));
                                got.weight = instructions ? util.format('%d', weight) : '';
                            } else {
                                got.weight = '';
                            }
                        }

                        if (headers.has('time')) {
                            if (!row.time.match(/\d+s/))
                                return cb(new Error('*** Time must be specied in seconds. (ex: 60s)'));
                            got.time = instructions ? util.format('%ds', time) : '';
                        }

                        if (headers.has('lanes')) {
                            got.lanes = (lanes || '').trim();
                        }

                        if (headers.has('speed')) {
                            if (row.speed !== '' && instructions) {
                                if (!row.speed.match(/\d+ km\/h/))
                                    cb(new Error('*** Speed must be specied in km/h. (ex: 50 km/h)'));
                                var speed = time > 0 ? Math.round(3.6*distance/time) : null;
                                got.speed = util.format('%d km/h', speed);
                            } else {
                                got.speed = '';
                            }
                        }

                        if (headers.has('intersections')) {
                            got.intersections = (intersections || '').trim();
                        }

                        if (headers.has('locations')){
                            got.locations = (locations || '').trim();
                        }
/*
                        if (headers.has('approaches')){
                            got.approaches = (approaches || '').trim();
                        }*/
                        // if header matches 'a:*', parse out the values for *
                        // and return in that header
                        headers.forEach((k) => {
                            let whitelist = ['duration', 'distance', 'datasources', 'nodes', 'weight', 'speed'];
                            if (k.match(/^a:/)) {
                                let a_type = k.slice(2);
                                if (whitelist.indexOf(a_type) == -1)
                                    return cb(new Error('Unrecognized annotation field', a_type));
                                if (annotation && !annotation[a_type])
                                    return cb(new Error('Annotation not found in response', a_type));
                                got[k] = annotation && annotation[a_type] || '';
                            }
                        });

                        var putValue = (key, value) => {
                            if (headers.has(key)) got[key] = instructions ? value : '';
                        };

                        putValue('ref', refs);
                        putValue('bearing', bearings);
                        putValue('turns', turns);
                        putValue('modes', modes);
                        putValue('classes', classes);
                        putValue('times', times);
                        putValue('distances', distances);
                        putValue('pronunciations', pronunciations);
                        putValue('destinations', destinations);
                        putValue('exits', exits);
                        putValue('weight_name', weight_name);
                        putValue('weights', weights);
                        putValue('weight', weight);
                        putValue('approach', approaches);

                        if (driving_sides) {
                            putValue('driving_side', driving_sides);
                        }

                        for (var key in row) {
                            if (this.FuzzyMatch.match(got[key], row[key])) {
                                got[key] = row[key];
                            }
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
                        bearings = [],
                        approaches = [];

                    if (row.bearings) {
                        got.bearings = row.bearings;
                        bearings = row.bearings.split(' ').filter(b => !!b);
                    }

                    if (row.approaches) {
                        got.approaches = row.approaches;
                        approaches = row.approaches.split(' ').filter(b => !!b);
                    }

                    if (row.from && row.to) {
                        var fromNode = this.findNodeByName(row.from);
                        if (!fromNode) return cb(new Error(util.format('*** unknown from-node "%s"', row.from)));
                        waypoints.push(fromNode);

                        var toNode = this.findNodeByName(row.to);
                        if (!toNode) return cb(new Error(util.format('*** unknown to-node "%s"', row.to)));
                        waypoints.push(toNode);

                        got.from = row.from;
                        got.to = row.to;
                        this.requestRoute(waypoints, bearings, approaches, params, afterRequest);
                    } else if (row.waypoints) {
                        row.waypoints.split(',').forEach((n) => {
                            var node = this.findNodeByName(n.trim());
                            if (!node) return cb(new Error(util.format('*** unknown waypoint node "%s"', n.trim())));
                            waypoints.push(node);
                        });
                        got.waypoints = row.waypoints;
                        this.requestRoute(waypoints, bearings, approaches, params, afterRequest);
                    } else {
                        return cb(new Error('*** no waypoints'));
                    }
                }
            };

            this.processRowsAndDiff(table, requestRow, callback);
        });
    };
};
