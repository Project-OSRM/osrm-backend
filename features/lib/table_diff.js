'use strict';

var util = require('util');
var path = require('path');
var fs = require('fs');
var chalk = require('chalk');

var unescapeStr = (str) => str.replace(/\\\|/g, '\|').replace(/\\\\/g, '\\');

String.prototype.padLeft = function(char, length) { 
    return char.repeat(Math.max(0, length - this.length)) + this;
}

String.prototype.padRight = function(char, length) { 
    return this + char.repeat(Math.max(0, length - this.length));
}

module.exports = function (expected, actual) {
    let headers = expected.raw()[0];
    let expectedRows = expected.hashes();
    let tableError = false;
    let statusRows = [];
    let columnStatus = {}

    expectedRows.forEach((expectedRow, i) => {
        var rowError = false;
        statusRows[i] = {};
        var statusRow = statusRows[i];
        for (var key in expectedRow) {
            var actualRow = actual[i]
            var row
            if (unescapeStr(expectedRow[key]) != actualRow[key]) {
                statusRow[key] = false;
                tableError = true;
                columnStatus[key] = false;
            }
        }
    });

    if (!tableError) return null;


    // determine column widths
    var widths = {};
    var wantStr = '(-) ';
    var gotStr  = '(+) ';
    var okStr   = '    ';

    headers.forEach( (key) => {
        widths[key] = key.length;
    });

    expectedRows.forEach((row,i) => {
        var cells = []
        headers.forEach( (key) => {
            var content = row[key]
            var length = content.length;
            if(widths[key]==null || length > widths[key])
              widths[key] = length;
       });
    });


    // format
    var lines = [chalk.red('Tables were not identical:')];
    var cells;

    // header row
    cells = []
    headers.forEach( (key) => {
        var content = String(key).padRight(' ', widths[key] );
        if (columnStatus[key] == false )
            content = okStr + content;
        cells.push( chalk.white( content ) );
    });
    lines.push( '| ' + cells.join(' | ') + ' |');

    // content rows
    expectedRows.forEach((row,i) => {
        var cells;
        var rowError = Object.keys(statusRows[i]).length > 0;

        // expected row
        cells = []
        headers.forEach( (key) => {
            var content = String(row[key]).padRight(' ', widths[key] );
            if (statusRows[i][key] == false)
                cells.push( chalk.yellow( wantStr + content) );
            else {
                if (rowError) {
                    if (columnStatus[key]==false)
                        content = okStr + content
                    cells.push( chalk.yellow( content) );
                }
                else {
                    if (columnStatus[key]==false)
                        content = okStr + content
                    cells.push( chalk.green( content) );
                }
            }
        });
        lines.push('| ' + cells.join(' | ') + ' |');

        // if error in row, insert extra row showing actual result
        if (rowError) {
            cells = []
            headers.forEach( (key) => {
                var content = String(actual[i][key]).padRight(' ', widths[key] );
                if (statusRows[i][key] == false)
                    cells.push( chalk.red( gotStr + content) );
                else {
                    if (columnStatus[key]==false)
                        cells.push( chalk.red( okStr + content) );
                    else
                        cells.push( chalk.red( content) );
                }
            });

            lines.push('| ' + cells.join(' | ') + ' |');
        }
    });
    return lines.join('\n');
};
