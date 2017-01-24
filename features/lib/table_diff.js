'use strict';

var util = require('util');
var path = require('path');
var fs = require('fs');
var chalk = require('chalk');

var unescapeStr = (str) => str.replace(/\\\|/g, '\|').replace(/\\\\/g, '\\');

module.exports = function (expected, actual) {
    let headers = expected.raw()[0];
    let expected_keys = expected.hashes();
    let diff = [];
    let hasErrors = false;

    var good = 0, bad = 0;

    expected_keys.forEach((row, i) => {
        var rowError = false;

        for (var j in row) {
            if (unescapeStr(row[j]) != actual[i][j]) {
                rowError = true;
                hasErrors = true;
                break;
            }
        }

        if (rowError) {
            bad++;
            diff.push(Object.assign({}, row, {c_status: 'undefined'}));
            diff.push(Object.assign({}, actual[i], {c_status: 'comment'}));
        } else {
            good++;
            diff.push(row);
        }
    });

    if (!hasErrors) return null;

    var s = ['Tables were not identical:'];
    s.push(headers.map(key => '    ' + key).join(' | '));
    diff.forEach((row) => {
        var rowString = '| ';
        headers.forEach((header) => {
            if (!row.c_status) rowString += chalk.green('    ' + row[header] + ' | ');
            else if (row.c_status === 'undefined') rowString += chalk.yellow('(-) ' + row[header] + ' | ');
            else rowString += chalk.red('(+) ' + row[header] + ' | ');
        });
        s.push(rowString);
    });

    return s.join('\n') + '\nTODO this is a temp workaround waiting for https://github.com/cucumber/cucumber-js/issues/534';
};
