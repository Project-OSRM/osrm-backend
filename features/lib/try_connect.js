// Network connectivity testing utility for checking if OSRM server is ready
'use strict';

const net = require('net');

// Attempts TCP connection to test if server is accepting connections
module.exports = function tryConnect(host, port, callback) {
  net.connect({ port: port, host: host })
    .on('connect', () => { callback(); })
    .on('error', () => {
        callback(new Error('Could not connect.'));
    });
}

