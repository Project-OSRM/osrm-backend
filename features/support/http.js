var Timeout = require('node-timeout');
var request = require('request');

module.exports = function () {
    this.paramsToString = (params) => {
        var paramString = '';
        if (params.coordinates !== undefined) {
            // FIXME this disables passing the output if its a default
            // Remove after #2173 is fixed.
            var outputString = (params.output && params.output !== 'json') ? ('.' + params.output) : '';
            paramString = params.coordinates.join(';') + outputString;
            delete params.coordinates;
            delete params.output;
        }
        if (Object.keys(params).length) {
            paramString += '?' + Object.keys(params).map(k => k + '=' + params[k]).join('&');
        }

        return paramString;
    };

    // FIXME this needs to be simplified!
    // - remove usage of node-timeout
    // - replace with node's native timout mechanism
    this.sendRequest = (baseUri, parameters, callback) => {
        var limit = Timeout(this.TIMEOUT, { err: { statusCode: 408 } });

        var runRequest = (cb) => {
            var params = this.paramsToString(parameters);
            this.query = baseUri + (params.length ? '/' + params : '');

            request(this.query, (err, res, body) => {
                if (err && err.code === 'ECONNREFUSED') {
                    return cb(new Error('*** osrm-routed is not running.'));
                } else if (err && err.statusCode === 408) {
                    return cb(new Error());
                }

                return cb(err, res, body);
            });
        };

        runRequest(limit((err, res, body) => {
            if (err) {
                if (err.statusCode === 408)
                    return callback(new Error('*** osrm-routed did not respond'));
                else if (err.code === 'ECONNREFUSED')
                    return callback(new Error('*** osrm-routed is not running'));
            }
            return callback(err, res, body);
        }));
    };
};
