var util = require('util');

var flatbuffers = require('../support/flatbuffers').flatbuffers;
var FBResult = require('../support/fbresult_generated').osrm.engine.api.fbresult.FBResult;

module.exports = function () {
    this.When(/^I request nearest I should get$/, (table, callback) => {
        this.reprocessAndLoadData((e) => {
            if (e) return callback(e);
            var testRow = (row, ri, cb) => {
                var inNode = this.findNodeByName(row.in);
                if (!inNode) throw new Error(util.format('*** unknown in-node "%s"', row.in));

                var outNode = this.findNodeByName(row.out);
                if (!outNode) throw new Error(util.format('*** unknown out-node "%s"', row.out));

                this.requestNearest(inNode, this.queryParams, (err, response) => {
                    if (err) return cb(err);
                    var coord;

                    if (response.statusCode === 200 && response.body.length) {
                        var json = JSON.parse(response.body);

                        coord = json.waypoints[0].location;

                        var got = { in: row.in, out: row.out };

                        Object.keys(row).forEach((key) => {
                            if (key === 'out') {
                                if (this.FuzzyMatch.matchLocation(coord, outNode)) {
                                    got[key] = row[key];
                                } else {
                                    row[key] = util.format('%s [%d,%d]', row[key], outNode.lat, outNode.lon);
                                }
                            }
                        });

                        cb(null, got);
                    }
                    else {
                        cb();
                    }
                });
            };

            this.processRowsAndDiff(table, testRow, callback);
        });
    });

    this.When(/^I request nearest with flatbuffers I should get$/, (table, callback) => {
        this.reprocessAndLoadData((e) => {
            if (e) return callback(e);
            var testRow = (row, ri, cb) => {
                var inNode = this.findNodeByName(row.in);
                if (!inNode) throw new Error(util.format('*** unknown in-node "%s"', row.in));

                var outNode = this.findNodeByName(row.out);
                if (!outNode) throw new Error(util.format('*** unknown out-node "%s"', row.out));

                this.queryParams.output = 'flatbuffers';
                this.requestNearest(inNode, this.queryParams, (err, response) => {
                    if (err) return cb(err);
                    var coord;

                    if (response.statusCode === 200 && response.body.length) {
                        var body = response.body;
                        var bytes = new Uint8Array(body.length);
                        for (var indx = 0; indx < body.length; ++indx) {
                            bytes[indx] = body.charCodeAt(indx);
                        }
                        var buf = new flatbuffers.ByteBuffer(bytes);
                        var fb = FBResult.getRootAsFBResult(buf);
                        var location = fb.waypoints(0).location();

                        coord = [location.longitude(), location.latitude()];

                        var got = { in: row.in, out: row.out };

                        Object.keys(row).forEach((key) => {
                            if (key === 'out') {
                                if (this.FuzzyMatch.matchLocation(coord, outNode)) {
                                    got[key] = row[key];
                                } else {
                                    row[key] = util.format('%s [%d,%d]', row[key], outNode.lat, outNode.lon);
                                }
                            }
                        });

                        cb(null, got);
                    }
                    else {
                        cb();
                    }
                });
            };

            this.processRowsAndDiff(table, testRow, callback);
        });
    });
};
