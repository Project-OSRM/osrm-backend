'use strict';

const util = require('util');

module.exports = {

    ensureDecimal: (i) => {
        if (parseInt(i) === i) return i.toFixed(1);
        else return i;
    },

    errorReason: (err) => {
        return err.signal ?
            'killed by signal ' + err.signal :
            'exited with code ' + err.code;
    }
};
