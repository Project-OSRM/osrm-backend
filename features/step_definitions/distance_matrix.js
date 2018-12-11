var util = require('util');

module.exports = function () {
    const durationsRegex = new RegExp(/^I request a travel time matrix I should get$/);
    const distancesRegex = new RegExp(/^I request a travel distance matrix I should get$/);
    const estimatesRegex = new RegExp(/^I request a travel time matrix I should get estimates for$/);

    const DURATIONS_NO_ROUTE = 2147483647;     // MAX_INT
    const DISTANCES_NO_ROUTE = 3.40282e+38;    // MAX_FLOAT

    this.When(durationsRegex, function(table, callback) {tableParse.call(this, table, DURATIONS_NO_ROUTE, 'durations', callback);}.bind(this));
    this.When(distancesRegex, function(table, callback) {tableParse.call(this, table, DISTANCES_NO_ROUTE, 'distances', callback);}.bind(this));
    this.When(estimatesRegex, function(table, callback) {tableParse.call(this, table, DISTANCES_NO_ROUTE, 'fallback_speed_cells', callback);}.bind(this));
};

const durationsParse = function(v) { return isNaN(parseInt(v)); };
const distancesParse = function(v) { return isNaN(parseFloat(v)); };
const estimatesParse = function(v) { return isNaN(parseFloat(v)); };

function tableParse(table, noRoute, annotation, callback) {

    const parse = annotation == 'distances' ? distancesParse : (annotation == 'durations' ? durationsParse : estimatesParse);
    const params = this.queryParams;
    params.annotations = ['durations','fallback_speed_cells'].indexOf(annotation) !== -1 ? 'duration' : 'distance';

    var tableRows = table.raw();

    if (tableRows[0][0] !== '') throw new Error('*** Top-left cell of matrix table must be empty');

    var waypoints = [],
        columnHeaders = tableRows[0].slice(1),
        rowHeaders = tableRows.map((h) => h[0]).slice(1),
        symmetric = columnHeaders.length == rowHeaders.length && columnHeaders.every((ele, i) => ele === rowHeaders[i]);

    if (symmetric) {
        columnHeaders.forEach((nodeName) => {
            var node = this.findNodeByName(nodeName);
            if (!node) throw new Error(util.format('*** unknown node "%s"', nodeName));
            waypoints.push({ coord: node, type: 'loc' });
        });
    } else {
        columnHeaders.forEach((nodeName) => {
            var node = this.findNodeByName(nodeName);
            if (!node) throw new Error(util.format('*** unknown node "%s"', nodeName));
            waypoints.push({ coord: node, type: 'dst' });
        });
        rowHeaders.forEach((nodeName) => {
            var node = this.findNodeByName(nodeName);
            if (!node) throw new Error(util.format('*** unknown node "%s"', nodeName));
            waypoints.push({ coord: node, type: 'src' });
        });
    }

    var actual = [];
    actual.push(table.headers);

    this.reprocessAndLoadData((e) => {
        if (e) return callback(e);
        // compute matrix

        this.requestTable(waypoints, params, (err, response) => {
            if (err) return callback(err);
            if (!response.body.length) return callback(new Error('Invalid response body'));

            var json = JSON.parse(response.body);

            var result = {};
            if (annotation === 'fallback_speed_cells') {
                result = table.raw().map(row => row.map(() => ''));
                json[annotation].forEach(pair => {
                    result[pair[0]+1][pair[1]+1] = 'Y';
                });
                result = result.slice(1).map(row => {
                    var hashes = {};
                    row.slice(1).forEach((v,i) => {
                        hashes[tableRows[0][i+1]] = v;
                    });
                    return hashes;
                });
            } else {
                result = json[annotation].map(row => {
                    var hashes = {};
                    row.forEach((v, i) => { hashes[tableRows[0][i+1]] = parse(v) ? '' : v; });
                    return hashes;
                });
            }

            var testRow = (row, ri, cb) => {
                for (var k in result[ri]) {
                    if (this.FuzzyMatch.match(result[ri][k], row[k])) {
                        result[ri][k] = row[k];
                    } else if (row[k] === '' && result[ri][k] === noRoute) {
                        result[ri][k] = '';
                    } else {
                        result[ri][k] = result[ri][k].toString();
                    }
                }

                result[ri][''] = row[''];
                cb(null, result[ri]);
            };

            this.processRowsAndDiff(table, testRow, callback);
        });
    });
}
