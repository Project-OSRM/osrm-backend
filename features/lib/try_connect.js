'use strict';

const net = require('net');

module.exports = function tryConnect(host, port, callback) {
  net.connect({ port: port, host: host })
    .on('connect', () => { callback(); })
    .on('error', () => {
        callback(new Error('Could not connect.'));
    });
}

