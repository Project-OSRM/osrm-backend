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

module.exports = function (expectedWithHeaders, actual) {
    let headers = expectedWithHeaders.raw()[0];
    let expected = expectedWithHeaders.rows();
    let tableError = false;
    let status = [];
    let columnStatus = []

    expected.forEach((expectedRow, i) => {
        var rowError = false;
        status[i] = [];
        expectedRow.forEach((value, j) => {
            if (unescapeStr(value) != actual[i][j]) {
                status[i][j] = false;
                columnStatus[j] = false;
                tableError = true;
            }
        });
    });
    
    if (!tableError) return null;

    // determine column widths
    var widths = [];
    var wantStr = '(-) ';
    var gotStr  = '(+) ';
    var okStr   = '    ';

    headers.forEach( (value,i) => {
        widths[i] = value.length;
    });

    expected.forEach((row,i) => {
        var cells = []
        row.forEach( (value,j) => {
            var length
            
            if (status[i][j]==false)
                length = Math.max( value.length, actual[i][j].length );
            else
                length = value.length;
                
            if(widths[j]==null || length > widths[j])
              widths[j] = length;
       });
    });

    // format
    var lines = [chalk.red('Tables were not identical:')];
    var cells;


    // header row
    cells = []
    headers.forEach( (value,i) => {
        var content = value.padRight(' ', widths[i] );
        if (columnStatus[i] == false )
            content = okStr + content;
        cells.push( chalk.white( content ) );
    });
    lines.push( '| ' + cells.join(' | ') + ' |');
      
    // content rows
    expected.forEach((row,i) => {
        var cells;
        var rowError = Object.keys(status[i]).length > 0;
        
        
        // expected row
        cells = []
        row.forEach( (value,j) => {
            var content = value.padRight(' ', widths[j] );
            
            if (status[i][j] == false)
                cells.push( chalk.yellow( wantStr + content) );
            else {
                if (rowError) {
                    if (columnStatus[j]==false)
                        content = okStr + content
                    cells.push( chalk.yellow( content) );
                }
                else {
                    if (columnStatus[j]==false)
                        content = okStr + content
                    cells.push( chalk.green( content) );
                }
            }
        });
        lines.push('| ' + cells.join(' | ') + ' |');

        // if error in row, insert extra row showing actual result
        if (rowError) {
            cells = []
            row.forEach( (value,j) => {
                var content
                if (actual[i][j])
                    content = actual[i][j].toString().padRight(' ', widths[j] );
                else
                    content = ''.padRight(' ', widths[j] );
                if (status[i][j] == false)
                    cells.push( chalk.red( gotStr + content) );
                else {
                    if (columnStatus[j]==false)
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
