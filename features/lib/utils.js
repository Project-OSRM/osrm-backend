'use strict';

const util = require('util');
const { mkdir } = require('fs/promises');

function Timeout(ms, options) {
    return function (cb) {
        let called = false;
        const timer = setTimeout(() => {
            if (!called) {
                called = true;
                cb(options.err || new Error(`Operation timed out after ${ms}ms`));
            }
        }, ms);

        return function (...args) {
            if (!called) {
                called = true;
                clearTimeout(timer);
                cb(...args);
            }
        };
    };
}

function createDir(dir, callback) {
    mkdir(dir, { recursive: true })
        .then(() => callback(null))
        .catch(err => callback(err));
}

module.exports = {

    createDir,
    ensureDecimal: (i) => {
        if (parseInt(i) === i) return i.toFixed(1);
        else return i;
    },

    errorReason: (err) => {
        return err.signal ?
            'killed by signal ' + err.signal :
            'exited with code ' + err.code;
    },
    Timeout
};
