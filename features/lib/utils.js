'use strict';


module.exports = {
ensureDecimal: (i) => {
    if (parseInt(i) === i) return i.toFixed(1);
    else return i;
}
};

