'use strict';

const net = require('net');
const Timeout = require('node-timeout');

module.exports = function tryConnect(port, callback) {
  net.connect({ port: port, host: '127.0.0.1' })
    .on('connect', () => { callback(); })
    .on('error', () => {
        callback(new Error('Could not connect.'));
    });
}

