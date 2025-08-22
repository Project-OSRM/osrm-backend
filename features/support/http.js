const { Timeout } = require('../lib/utils');
const http = require('http');
const https = require('https');

function httpRequest(url, callback) {
  const client = url.startsWith('https') ? https : http;
  const req = client.get(url, (res) => {
    let data = '';

    // Collect data chunks
    res.on('data', (chunk) => {
      data += chunk;
    });

    // Handle end of response
    res.on('end', () => {
      callback(null, res, data);
    });
  });

  // Handle errors
  req.on('error', (err) => {
    callback(err);
  });

  req.end();
}
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
  this.sendRequest = (baseUri, parameters, callback) => {

    var limit = Timeout(this.TIMEOUT, { err: { statusCode: 408 } });
    var runRequest = (cb) => {
      var params = this.paramsToString(parameters);
      this.query = baseUri + (params.length ? '/' + params : '');

      httpRequest(this.query, (err, res, body) => {
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
