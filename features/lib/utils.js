'use strict';

const util = require('util');

module.exports = {

    ensureDecimal: (i) => {
        if (parseInt(i) === i) return i.toFixed(1);
        else return i;
    },

    errorReason: (err) => {
        return err.signal ?
            util.format('killed by signal %s', err.signal) :
            util.format('exited with code %d', err.code);
    }
};
