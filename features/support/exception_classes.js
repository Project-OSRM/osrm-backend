'use strict';

var util = require('util');
var path = require('path');
var fs = require('fs');
var chalk = require('chalk');

var unescapeStr = (str) => str.replace(/\\\|/g, '\|').replace(/\\\\/g, '\\');

module.exports = {
    TableDiffError: class extends Error {
        constructor (expected, actual) {
            super();
            this.headers = expected.raw()[0];
            this.expected = expected.hashes();
            this.actual = actual;
            this.diff = [];
            this.hasErrors = false;

            var good = 0, bad = 0;

            this.expected.forEach((row, i) => {
                var rowError = false;

                for (var j in row) {
                    if (unescapeStr(row[j]) != actual[i][j]) {
                        rowError = true;
                        this.hasErrors = true;
                        break;
                    }
                }

                if (rowError) {
                    bad++;
                    this.diff.push(Object.assign({}, row, {c_status: 'undefined'}));
                    this.diff.push(Object.assign({}, actual[i], {c_status: 'comment'}));
                } else {
                    good++;
                    this.diff.push(row);
                }
            });
        }

        get string () {
            if (!this.hasErrors) return null;

            var s = ['Tables were not identical:'];
            s.push(this.headers.map(key => '    ' + key).join(' | '));
            this.diff.forEach((row) => {
                var rowString = '| ';
                this.headers.forEach((header) => {
                    if (!row.c_status) rowString += chalk.green('    ' + row[header] + ' | ');
                    else if (row.c_status === 'undefined') rowString += chalk.yellow('(-) ' + row[header] + ' | ');
                    else rowString += chalk.red('(+) ' + row[header] + ' | ');
                });
                s.push(rowString);
            });

            return s.join('\n') + '\nTODO this is a temp workaround waiting for https://github.com/cucumber/cucumber-js/issues/534';
        }
    }
};
