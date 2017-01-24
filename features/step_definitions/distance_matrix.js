var util = require('util');

module.exports = function () {
    this.When(/^I request a travel time matrix I should get$/, (table, callback) => {
        var NO_ROUTE = 2147483647;    // MAX_INT

        var tableRows = table.raw();

        if (tableRows[0][0] !== '') throw new Error('*** Top-left cell of matrix table must be empty');

        var waypoints = [],
            columnHeaders = tableRows[0].slice(1),
            rowHeaders = tableRows.map((h) => h[0]).slice(1),
            symmetric = columnHeaders.every((ele, i) => ele === rowHeaders[i]);

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
            var params = this.queryParams;

            this.requestTable(waypoints, params, (err, response) => {
                if (err) return callback(err);
                if (!response.body.length) return callback(new Error('Invalid response body'));

                var json = JSON.parse(response.body);

                var result = json['durations'].map(row => {
                    var hashes = {};
                    row.forEach((v, i) => { hashes[tableRows[0][i+1]] = isNaN(parseInt(v)) ? '' : v; });
                    return hashes;
                });

                var testRow = (row, ri, cb) => {
                    for (var k in result[ri]) {
                        if (this.FuzzyMatch.match(result[ri][k], row[k])) {
                            result[ri][k] = row[k];
                        } else if (row[k] === '' && result[ri][k] === NO_ROUTE) {
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
    });
};
