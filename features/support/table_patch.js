var DifferentError = require('./exception_classes').TableDiffError;

module.exports = function () {
    this.diffTables = (expected, actual, options, callback) => {
        // this is a temp workaround while waiting for https://github.com/cucumber/cucumber-js/issues/534

        var error = new DifferentError(expected, actual);

        return callback(error.string);
    };
};
