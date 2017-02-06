'use strict';

var d3 = require('d3-queue');
const tableDiff = require('../lib/table_diff');


module.exports = function () {

    this.When(/^processing way tags I should get$/, (table, callback) => {
        
        // determine split point
        var headers = table.raw()[0]
        var split
        headers.forEach((key,i) => {
            if (key == '>') {
                split = i
            }
        });
        
        // process rows
        var resultTable = []
        table.rows().forEach((row, i) => {
            // fetch input tags and expected tags
            var input = {}
            var expected = {}
            row.forEach((v,j) => {
                if (j<split && v)
                    input[ headers[j] ] = v
                else if (j>split && v)
                    expected[ headers[j] ] = v
            })
            
            // call profile
            var json = this.processWayTags( input )
            
            // prepare row for later table diff 
            var actual = []
            row.forEach((value,j) => {
                if (j <= split) {
                    actual[ j ] = value                     // input tags are simply copied
                } else if (j > split) {
                    if (json[ headers[j] ] == null)
                        actual[ j ] = ''                    // treat '' as null 
                    else
                        actual[ j ] = json[ headers[j] ]    // output tags from profile output
                }
            })
            
            // store row
            resultTable.push( actual )
        })

        // compare expected and actual tables
        var diff = tableDiff(table, resultTable)
        callback(diff)
    });
}
