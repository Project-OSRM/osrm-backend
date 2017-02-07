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
    let diff = [];
    let tableError = false;

    expected.hashes().forEach((expectedRow, i) => {        
        var rowError = false;

        for (var key in expectedRow) {
            var actualRow = actual[i]
            var row
            if (unescapeStr(expectedRow[key]) != actualRow[key]) {
                
                row = Object.assign({}, expectedRow, {diff_status: 'expected'})
                diff.push(row);
                
                row = Object.assign({}, actualRow, {diff_status: 'actual'})
                diff.push(row);
                
                tableError = true;
                rowError = true;
                break;
            }
        }
        if( !rowError ) diff.push(expectedRow);
    });

    if (!tableError) return null;
    
    
    // insert a hash of headers where key=value so we can treat
    // the header row like other rows in the processing below
    var header_hash = {}
    headers.forEach( (key,i) => {
        header_hash[key] = key;
    });
    diff.unshift( header_hash )
    
    // determine column widths
    var widths = [];
    diff.forEach((row) => {
        var cells = []
        headers.forEach( (key,i) => {
            var s = row[key]
            var length = s.length;
            if(widths[i]==null || length > widths[i])
              widths[i] = length;
       });
    });

    // format
    var lines = ['Tables were not identical:'];
    diff.forEach((row) => {
        var cells = []
        headers.forEach( (key,i) => {
            var s
            if( row[key] )
                s = row[key].padRight(' ', widths[i] );
            else
                s = ' '.padRight(' ', widths[i] );
                
            if(row.diff_status == 'expected')
                cells.push( chalk.yellow('(-) ' + s) );
            else if(row.diff_status == 'actual')
                cells.push( chalk.red(   '(+) ' + s) );
            else
                cells.push( chalk.green( '    ' + s) );
        });
        lines.push('| ' + cells.join(' | ') + ' |');
    });
    return lines.join('\n');
};
