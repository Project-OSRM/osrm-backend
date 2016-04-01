'use strict';

var util = require('util');
var path = require('path');
var fs = require('fs');
var chalk = require('chalk');

var OSRMError = class extends Error {
    constructor (process, code, msg, log, lines) {
        super(msg);
        this.process = process;
        this.code = code;
        this.msg = msg;
        this.lines = lines;
        this.log = log;
    }

    extract (callback) {
        this.logTail(this.log, this.lines, callback);
    }

    // toString (callback) {
    //     this.extract((tail) => {
    //         callback(util.format('*** %s\nLast %s from %s:\n%s\n', this.msg, this.lines, this.log, tail));
    //     });
    // }

    logTail (logPath, n, callback) {
        var expanded = path.resolve(this.TEST_FOLDER, logPath);
        fs.exists(expanded, (exists) => {
            if (exists) {
                fs.readFile(expanded, (err, data) => {
                    var lines = data.toString().trim().split('\n');
                    callback(lines
                        .slice(lines.length - n)
                        .map(line => util.format('    %s', line))
                        .join('\n'));
                });
            } else {
                callback(util.format('File %s does not exist!', expanded));
            }
        });
    }
};

var unescapeStr = (str) => str.replace(/\\\|/g, '\|').replace(/\\\\/g, '\\');

module.exports = {
    OSRMError: OSRMError,

    FileError: class extends OSRMError {
        constructor (logFile, code, msg) {
            super ('fileutil', code, msg, logFile, 5);
        }
    },

    LaunchError: class extends OSRMError {
        constructor (logFile, launchProcess, code, msg) {
            super (launchProcess, code, msg, logFile, 5);
        }
    },

    ExtractError: class extends OSRMError {
        constructor (logFile, code, msg) {
            super('osrm-extract', code, msg, logFile, 3);
        }
    },

    ContractError:  class extends OSRMError {
        constructor (logFile, code, msg) {
            super('osrm-contract', code, msg, logFile, 3);
        }
    },

    RoutedError: class extends OSRMError {
        constructor (logFile, msg) {
            super('osrm-routed', null, msg, logFile, 3);
        }
    },

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
