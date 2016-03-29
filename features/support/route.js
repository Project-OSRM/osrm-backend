var Timeout = require('node-timeout');
var request = require('request');

module.exports = function () {
    this.requestPath = (service, params, callback) => {
        var uri;
        if (service == 'timestamp') {
            uri = [this.HOST, service].join('/');
        } else {
            uri = [this.HOST, service, 'v1', this.profile].join('/');
        }

        return this.sendRequest(uri, params, callback);
    };

    this.requestUrl = (path, callback) => {
        var uri = this.query = [this.HOST, path].join('/'),
            limit = Timeout(this.OSRM_TIMEOUT, { err: { statusCode: 408 } });

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
        return waypoints.map(w => [w.lon, w.lat].map(this.ensureDecimal).join(','));
    };

    this.requestRoute = (waypoints, bearings, userParams, callback) => {
        if (bearings.length && bearings.length !== waypoints.length) throw new Error('*** number of bearings does not equal the number of waypoints');

        var defaults = {
                output: 'json',
                steps: 'true',
                alternatives: 'false'
            },
            params = this.overwriteParams(defaults, userParams),
            encodedWaypoints = encodeWaypoints(waypoints);

        params.coordinates = encodedWaypoints;

        if (bearings.length) {
            // TODOTODO
            var encodedBearings = bearings.map(b => ['b', b.toString()]);
            params = Array.prototype.concat.apply(params, encodedWaypoints.map((o, i) => [o, encodedBearings[i]]));
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
        // TODO what was 'type' here?
        // params = params.concat(waypoints.map(w => [w.type, [w.coord.lat, w.coord.lon].join(',')]));

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
            params.timestamps = timestamps.join(',');
        }

        return this.requestPath('match', params, callback);
    };

    this.extractInstructionList = (instructions, keyFinder, postfix) => {
        postfix = postfix || null;
        if (instructions) {
            return instructions.legs.reduce((m, v) => m.concat(v.steps), [])
                .filter(v => v.maneuver.type !== 'arrive')
                .map(keyFinder)
                .join(',');
        }
    };

    this.wayList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.name);
    };

    this.bearingList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.maneuver.bearing_after);
    };

    this.turnList = (instructions) => {
        return instructions.legs.reduce((m, v) => m.concat(v.steps), [])
            .map(v => {
                switch (v.maneuver.type) {
                    case 'depart':
                    case 'arrive':
                        return v.maneuver.type;
                    case 'roundabout':
                        return 'roundabout-exit-' + v.maneuver.exit;
                    default:
                        return v.maneuver.modifier
                }
            })
            .join(',');
    };

    this.modeList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.mode);
    };

    this.timeList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.duration + 's');
    };

    this.distanceList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.distance + 'm');
    };
};
