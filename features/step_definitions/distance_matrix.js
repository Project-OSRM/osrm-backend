var util = require('util');

var flatbuffers = require('../support/flatbuffers').flatbuffers;
var FBResult = require('../support/fbresult_generated').osrm.engine.api.fbresult.FBResult;

module.exports = function () {
    const durationsRegex = new RegExp(/^I request a travel time matrix I should get$/);
    const durationsCodeOnlyRegex = new RegExp(/^I request a travel time matrix with these waypoints I should get the response code$/);
    const distancesRegex = new RegExp(/^I request a travel distance matrix I should get$/);
    const estimatesRegex = new RegExp(/^I request a travel time matrix I should get estimates for$/);
    const durationsRegexFb = new RegExp(/^I request a travel time matrix with flatbuffers I should get$/);
    const distancesRegexFb = new RegExp(/^I request a travel distance matrix with flatbuffers I should get$/);

    const DURATIONS_NO_ROUTE = 2147483647;     // MAX_INT
    const DISTANCES_NO_ROUTE = 3.40282e+38;    // MAX_FLOAT

    const FORMAT_JSON = 'json';
    const FORMAT_FB = 'flatbuffers';

    this.When(durationsRegex, function(table, callback) {tableParse.call(this, table, DURATIONS_NO_ROUTE, 'durations', FORMAT_JSON, callback);}.bind(this));
    this.When(durationsCodeOnlyRegex, function(table, callback) {tableCodeOnlyParse.call(this, table, 'durations', FORMAT_JSON, callback);}.bind(this));
    this.When(distancesRegex, function(table, callback) {tableParse.call(this, table, DISTANCES_NO_ROUTE, 'distances', FORMAT_JSON, callback);}.bind(this));
    this.When(estimatesRegex, function(table, callback) {tableParse.call(this, table, DISTANCES_NO_ROUTE, 'fallback_speed_cells', FORMAT_JSON, callback);}.bind(this));
    this.When(durationsRegexFb, function(table, callback) {tableParse.call(this, table, DURATIONS_NO_ROUTE, 'durations', FORMAT_FB, callback);}.bind(this));
    this.When(distancesRegexFb, function(table, callback) {tableParse.call(this, table, DISTANCES_NO_ROUTE, 'distances', FORMAT_FB, callback);}.bind(this));
};

const durationsParse = function(v) { return isNaN(parseInt(v)); };
const distancesParse = function(v) { return isNaN(parseFloat(v)); };
const estimatesParse = function(v) { return isNaN(parseFloat(v)); };

function tableCodeOnlyParse(table, annotation, format, callback) {

    const params = this.queryParams;
    params.annotations = ['durations','fallback_speed_cells'].indexOf(annotation) !== -1 ? 'duration' : 'distance';
    params.output = format;

    var got;

    this.reprocessAndLoadData((e) => {
        if (e) return callback(e);
        var testRow = (row, ri, cb) => {
            var afterRequest = (err, res) => {
                if (err) return cb(err);

                for (var k in row) {
                    var match = k.match(/param:(.*)/);
                    if (match) {
                        if (row[k] === '(nil)') {
                            params[match[1]] = null;
                        } else if (row[k]) {
                            params[match[1]] = [row[k]];
                        }
                        got[k] = row[k];
                    }
                }

                var json;
                got.code = 'unknown';
                if (res.body.length) {
                    json = JSON.parse(res.body);
                    got.code = json.code;
                }

                cb(null, got);
            };

            var params = this.queryParams,
                waypoints = [];
            if (row.waypoints) {
                row.waypoints.split(',').forEach((n) => {
                    var node = this.findNodeByName(n);
                    if (!node) throw new Error(util.format('*** unknown waypoint node "%s"', n.trim()));
                    waypoints.push({ coord: node, type: 'loc' });

                });
                got = { waypoints: row.waypoints };

                this.requestTable(waypoints, params, afterRequest);
            } else {
                throw new Error('*** no waypoints');
            }
        };

        this.processRowsAndDiff(table, testRow, callback);
    });

}

function tableParse(table, noRoute, annotation, format, callback) {

    const parse = annotation == 'distances' ? distancesParse : (annotation == 'durations' ? durationsParse : estimatesParse);
    const params = this.queryParams;
    params.annotations = ['durations','fallback_speed_cells'].indexOf(annotation) !== -1 ? 'duration' : 'distance';
    params.output = format;

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

    this.reprocessAndLoadData((e) => {
        if (e) return callback(e);
        // compute matrix

        this.requestTable(waypoints, params, (err, response) => {
            if (err) return callback(err);
            if (!response.body.length) return callback(new Error('Invalid response body'));

            var result = [];
            if (format === 'json') {
                var json = JSON.parse(response.body);

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
            } else { //flatbuffers
                var body = response.body;
                var bytes = new Uint8Array(body.length);
                for (var indx = 0; indx < body.length; ++indx) {
                    bytes[indx] = body.charCodeAt(indx);
                }
                var buf = new flatbuffers.ByteBuffer(bytes);
                var fb = FBResult.getRootAsFBResult(buf);

                var matrix;
                if (annotation === 'durations') {
                    matrix = fb.table().durationsArray();
                }
                if (annotation === 'distances') {
                    matrix = fb.table().distancesArray();
                }
                var cols = fb.table().cols();
                var rows = fb.table().rows();
                for (let r = 0; r < rows; ++r) {
                    result[r]={};
                    for(let c=0; c < cols; ++c) {
                        result[r][tableRows[0][c+1]] = matrix[r*cols + c];
                    }
                }
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
