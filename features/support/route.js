var Timeout = require('node-timeout');
var request = require('request');

module.exports = function () {
    this.requestPath = (service, params, callback) => {
        var uri = [this.HOST, service, 'v1', this.profile].join('/');
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
            return Array.prototype.concat.apply([],
                instructions.legs.map(l => l.steps))
                    .filter(s => s.maneuver.type !== 'arrive')
                    .map(keyFinder)
                .join(',');
        }
    };

    this.wayList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.name);
    };

    this.compassList = (instructions) => {
        return this.extractInstructionList(instructions, 6);
    };

    this.bearingList = (instructions) => {
        return this.extractInstructionList(instructions, 7);
    };

    this.turnList = (instructions) => {
        var types = {
            '0': 'none',
            '1': 'straight',
            '2': 'slight_right',
            '3': 'right',
            '4': 'sharp_right',
            '5': 'u_turn',
            '6': 'sharp_left',
            '7': 'left',
            '8': 'slight_left',
            '9': 'via',
            '10': 'head',
            '11': 'enter_roundabout',
            '12': 'leave_roundabout',
            '13': 'stay_roundabout',
            '14': 'start_end_of_street',
            '15': 'destination',
            '16': 'name_changes',
            '17': 'enter_contraflow',
            '18': 'leave_contraflow'
        };

        // replace instructions codes with strings, e.g. '11-3' gets converted to 'enter_roundabout-3'
        return instructions ? instructions.map(r => r[0].toString().replace(/^(\d*)/, (match, num) => types[num])).join(',') : instructions;
    };

    this.modeList = (instructions) => {
        return this.extractInstructionList(instructions, 8);
    };

    this.timeList = (instructions) => {
        return this.extractInstructionList(instructions, 4, 's');
    };

    this.distanceList = (instructions) => {
        return this.extractInstructionList(instructions, 2, 'm');
    };
};
