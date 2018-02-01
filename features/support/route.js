'use strict';

const Timeout = require('node-timeout');
const request = require('request');
const ensureDecimal = require('../lib/utils').ensureDecimal;

module.exports = function () {
    this.requestPath = (service, params, callback) => {
        var uri;
        if (service == 'timestamp' || this.osrmLoader.method === 'valhalla') {
            uri = [this.HOST, service].join('/');
        } else {
            uri = [this.HOST, service, 'v1', this.profile].join('/');
        }

        return this.sendRequest(uri, params, this.osrmLoader.method === 'valhalla' ? 'POST' : 'GET', callback);
    };

    this.requestUrl = (path, callback) => {
        var uri = this.query = [this.HOST, path].join('/'),
            limit = Timeout(this.TIMEOUT, { err: { statusCode: 408 } });

        function runRequest (cb) {
            request(uri, cb);
        }

        runRequest(limit((err, res, body) => {
            if (err) {
                if (err.statusCode === 408) return callback(this.RoutedError('*** osrm-routed did not respond'));
                else if (err.code === 'ECONNREFUSED')
                    return callback(this.RoutedError('*** osrm-routed is not running'));
            } else
                return callback(err, res, body);
        }));
    };

    // Overwrites the default values in defaults
    // e.g. [[a, 1], [b, 2]], [[a, 5], [d, 10]] => [[a, 5], [b, 2], [d, 10]]
    this.overwriteParams = (defaults, other) => {
        var otherMap = {};
        for (var key in other) otherMap[key] = other[key];
        return Object.assign({}, defaults, otherMap);
    };

    var encodeWaypoints = (waypoints) => {
        return waypoints.map(w => [w.lon, w.lat].map(ensureDecimal).join(','));
    };

    this.requestRoute = (waypoints, bearings, approaches, userParams, callback) => {
        if (bearings.length && bearings.length !== waypoints.length) throw new Error('*** number of bearings does not equal the number of waypoints');
        if (approaches.length && approaches.length !== waypoints.length) throw new Error('*** number of approaches does not equal the number of waypoints');

        var defaults = {
                output: 'json',
                steps: 'true',
                alternatives: 'false'
            },
            params = this.overwriteParams(defaults, userParams),
            encodedWaypoints = encodeWaypoints(waypoints);

        params.coordinates = encodedWaypoints;

        if (bearings.length) {
            params.bearings = bearings.map(b => {
                var bs = b.split(',');
                if (bs.length === 2) return b;
                else return b += ',10';
            }).join(';');
        }

        if (approaches.length) {
            params.approaches = approaches.join(';');
        }

        if (this.osrmLoader.method === 'valhalla') {
        params = {
            locations: waypoints.map(w => {return{ lat: w.lat, lon: w.lon };}),
            costing: 'auto',
            directions_options:{
                units:'miles'}
            };
        }


        return this.requestPath('route', params, callback);
    };

    this.requestNearest = (node, userParams, callback) => {
        var defaults = {
                output: 'json'
            },
            params = this.overwriteParams(defaults, userParams);
        params.coordinates = [[node.lon, node.lat].join(',')];

        return this.requestPath('nearest', params, callback);
    };

    this.requestTable = (waypoints, userParams, callback) => {
        var defaults = {
                output: 'json'
            },
            params = this.overwriteParams(defaults, userParams);

        params.coordinates = waypoints.map(w => [w.coord.lon, w.coord.lat].join(','));
        var srcs = waypoints.map((w, i) => [w.type, i]).filter(w => w[0] === 'src').map(w => w[1]),
            dsts = waypoints.map((w, i) => [w.type, i]).filter(w => w[0] === 'dst').map(w => w[1]);
        if (srcs.length) params.sources = srcs.join(';');
        if (dsts.length) params.destinations = dsts.join(';');

        return this.requestPath('table', params, callback);
    };

    this.requestTrip = (waypoints, userParams, callback) => {
        var defaults = {
                output: 'json'
            },
            params = this.overwriteParams(defaults, userParams);

        params.coordinates = encodeWaypoints(waypoints);

        return this.requestPath('trip', params, callback);
    };

    this.requestMatching = (waypoints, timestamps, userParams, callback) => {
        var defaults = {
                output: 'json'
            },
            params = this.overwriteParams(defaults, userParams);

        params.coordinates = encodeWaypoints(waypoints);

        if (timestamps.length) {
            params.timestamps = timestamps.join(';');
        }

        return this.requestPath('match', params, callback);
    };

    this.extractInstructionList = (instructions, keyFinder) => {
        if (this.osrmLoader.method === 'valhalla') {
            if (instructions) {
                return instructions.legs.reduce((m, v) => m.concat(v.maneuvers), [])
                    .map(keyFinder)
                    .join(',');
            }
        } else {
            if (instructions) {
                return instructions.legs.reduce((m, v) => m.concat(v.steps), [])
                    .map(keyFinder)
                    .join(',');
            }
        }
    };

    this.summary = (instructions) => {
        if (instructions) {
            return instructions.legs.map(l => l.summary).join(';');
        }
    };

    this.wayList = (instructions) => {
        if (this.osrmLoader.method === 'valhalla') {
            var result = this.extractInstructionList(instructions, s => s.street_names && s.street_names.join(',') || '');
            var laststep = result.split(',').slice(-2)[0];
            return result + laststep;
        } else {
            return this.extractInstructionList(instructions, s => s.name);
        }
    };

    this.refList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.ref || '');
    };

    this.pronunciationList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.pronunciation || '');
    };

    this.destinationsList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.destinations || '');
    };

    this.exitsList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.exits || '');
    };

    this.reverseBearing = (bearing) => {
        if (bearing >= 180)
            return bearing - 180.;
        return bearing + 180;
    };

    this.bearingList = (instructions) => {

        if (this.osrmLoader.method === 'valhalla') {
            return 'NOT IMPLEMENTED';
        } else {
            return this.extractInstructionList(instructions, s => ('in' in s.intersections[0] ? this.reverseBearing(s.intersections[0].bearings[s.intersections[0].in]) : 0)
                                                              + '->' +
                                                              ('out' in s.intersections[0] ? s.intersections[0].bearings[s.intersections[0].out] : 0));
        }
    };

    this.lanesList = (instructions) => {
        if (this.osrmLoader.method === 'valhalla') {
            return 'NOT IMPLEMENTED';
        } else {
        return this.extractInstructionList(instructions, s => {
            return s.intersections.map( i => {
                if(i.lanes)
                {
                    return i.lanes.map( l => {
                        let indications = l.indications.join(';');
                        return indications + ':' + (l.valid ? 'true' : 'false');
                    }).join(' ');
                }
                else
                {
                    return '';
                }
            }).join(';');
        });
    }
    };

    this.approachList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.approaches || '');
    };

    this.annotationList = (instructions) => {
        if (!('annotation' in instructions.legs[0]))
            return '';

        var merged = {};
        instructions.legs.map(l => {
            Object.keys(l.annotation).forEach(a => {
                if (!merged[a]) merged[a] = [];
                merged[a].push(l.annotation[a].join(':'));
            });
        });
        Object.keys(merged).map(a => {
            merged[a] = merged[a].join(',');
        });
        return merged;
    };

    this.alternativesList = (instructions) => {
        // alternatives_count come from tracepoints list
        return instructions.tracepoints.map(t => t.alternatives_count.toString()).join(',');
    };

    this.turnList = (instructions) => {
        if (this.osrmLoader.method === 'valhalla') {
                /*
                from https://github.com/valhalla/valhalla-docs/blob/master/turn-by-turn/api-reference.md

                kNone = 0;
kStart = 1; kStartRight = 2; kStartLeft = 3; kDestination = 4; kDestinationRight = 5; kDestinationLeft = 6;
kBecomes = 7; kContinue = 8;
kSlightRight = 9; kRight = 10; kSharpRight = 11; kUturnRight = 12;

kUturnLeft = 13; kSharpLeft = 14; kLeft = 15; kSlightLeft = 16;

kRampStraight = 17; kRampRight = 18; kRampLeft = 19;

kExitRight = 20; kExitLeft = 21;

kStayStraight = 22; kStayRight = 23; kStayLeft = 24;

kMerge = 25;

kRoundaboutEnter = 26;
kRoundaboutExit = 27;
kFerryEnter = 28;
kFerryExit = 29;
kTransit = 30;
kTransitTransfer = 31;
kTransitRemainOn = 32;
kTransitConnectionStart = 33;
kTransitConnectionTransfer = 34;
kTransitConnectionDestination = 35;
kPostTransitConnectionDestination = 36;
*/
var typemap = {
    1: {type: 'depart', modifier: null },
    2: {type: 'depart', modifier: 'right' },
    3: {type: 'depart', modifier: 'left' },

    4: {type: 'arrive', modifier: null },
    5: {type: 'arrive', modifier: 'right' },
    6: {type: 'arrive', modifier: 'left' },

    7: {type: 'becomes', modifier: null },
    8: {type: 'continue', modifier: 'straight' },

    9: {type: 'turn', modifier: 'slight right' },
    10: {type: 'turn', modifier: 'right' },
    11: {type: 'turn', modifier: 'sharp right' },
    12: {type: 'uturn', modifier: 'right' },

    13: {type: 'uturn', modifier: 'left' },
    14: {type: 'turn', modifier: 'sharp left' },
    15: {type: 'turn', modifier: 'left' },
    16: {type: 'turn', modifier: 'slight left' },

    17: {type: 'on ramp', modifier: 'straight' },
    18: {type: 'on ramp', modifier: 'right' },
    19: {type: 'on ramp', modifier: 'left' },

    20: {type: 'off ramp', modifier: 'right' },
    21: {type: 'off ramp', modifier: 'left' },

    22: {type: 'continue', modifier: 'straight' },
    23: {type: 'continue', modifier: 'right' },
    24: {type: 'continue', modifier: 'left' },

    25: {type: 'merge', modifier: null },

    26: {type: 'roundabout', modifier: null },
    27: {type: 'roundabout-exit', modifier: null },

}
        return instructions.legs.reduce((m, v) => m.concat(v.maneuvers), [])
            .map(v => {
                if (v.type in typemap) {
                    if (typemap[v.type].modifier) {
                        return `${typemap[v.type].type} ${typemap[v.type].modifier}`;
                    } else {
                        return `${typemap[v.type].type}`;
                    }
                } else {
                    return "UNRECOGNIZED";
                }
            });
        } else {
        return instructions.legs.reduce((m, v) => m.concat(v.steps), [])
            .map(v => {
                switch (v.maneuver.type) {
                case 'depart':
                case 'arrive':
                    return v.maneuver.type;
                case 'on ramp':
                case 'off ramp':
                    return v.maneuver.type + ' ' + v.maneuver.modifier;
                case 'roundabout':
                    return 'roundabout-exit-' + v.maneuver.exit;
                case 'rotary':
                    if( 'rotary_name' in v )
                        return v.rotary_name + '-exit-' + v.maneuver.exit;
                    else
                        return 'rotary-exit-' + v.maneuver.exit;
                case 'roundabout turn':
                    return v.maneuver.type + ' ' + v.maneuver.modifier + ' exit-' + v.maneuver.exit;
                // FIXME this is a little bit over-simplistic for merge/fork instructions
                default:
                    return v.maneuver.type + ' ' + v.maneuver.modifier;
                }
            })
            .join(',');
        }
    };

    this.locations = (instructions) => {
        if (this.osrmLoader.method === 'valhalla') {
            return 'NOT IMPLEMENTED';
        } else {
        return instructions.legs.reduce((m, v) => m.concat(v.steps), [])
            .map(v => {
                return this.findNodeByLocation(v.maneuver.location);
            })
            .join(',');
        }
    };

    this.intersectionList = (instructions) => {
        if (this.osrmLoader.method === 'valhalla') {
            return 'NOT IMPLEMENTED';
        } else {
        return instructions.legs.reduce((m, v) => m.concat(v.steps), [])
            .map( v => {
                return v.intersections
                    .map( intersection => {
                        var string = intersection.entry[0]+':'+intersection.bearings[0], i;
                        for( i = 1; i < intersection.bearings.length; ++i )
                            string = string + ' ' + intersection.entry[i]+':'+intersection.bearings[i];
                        return string;
                    }).join(',');
            }).join(';');
        }
    };

    this.modeList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.mode);
    };

    this.drivingSideList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.driving_side);
    };

    this.classesList = (instructions) => {
        if (this.osrmLoader.method === 'valhalla') {
            return 'NOT IMPLEMENTED';
        } else {
        return this.extractInstructionList(instructions, s => '[' + s.intersections.map(i => '(' + (i.classes ? i.classes.join(',') : '') + ')').join(',') + ']');
        }
    };

    this.timeList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.duration + 's');
    };

    this.distanceList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.distance + 'm');
    };

    this.weightName = (instructions) => {
        return instructions ? instructions.weight_name : '';
    };

    this.weightList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.weight);
    };
};
