var fs = require('fs');
var path = require('path');
var util = require('util');
var exec = require('child_process').exec;
var d3 = require('d3-queue');

var OSM = require('./build_osm');
var classes = require('./data_classes');

module.exports = function () {
    this.setGridSize = (meters) => {
        // the constant is calculated (with BigDecimal as: 1.0/(DEG_TO_RAD*EARTH_RADIUS_IN_METERS
        // see ApproximateDistance() in ExtractorStructs.h
        // it's only accurate when measuring along the equator, or going exactly north-south
        this.zoom = parseFloat(meters) * 0.8990679362704610899694577444566908445396483347536032203503E-5;
    };

    this.setOrigin = (origin) => {
        this.origin = origin;
    };

    this.buildWaysFromTable = (table, callback) => {
        // add one unconnected way for each row
        var buildRow = (row, ri, cb) => {
            // comments ported directly from ruby suite:
            // NOTE: currently osrm crashes when processing an isolated oneway with just 2 nodes, so we use 4 edges
            // this is related to the fact that a oneway dead-end street doesn't make a lot of sense

            // if we stack ways on different x coordinates, routability tests get messed up, because osrm might pick a neighboring way if the one test can't be used.
            // instead we place all lines as a string on the same y coordinate. this prevents using neighboring ways.

            // add some nodes

            var makeFakeNode = (namePrefix, offset) => {
                return new OSM.Node(this.makeOSMId(), this.OSM_USER, this.OSM_TIMESTAMP,
                    this.OSM_UID, this.origin[0]+(offset + this.WAY_SPACING * ri) * this.zoom,
                    this.origin[1], {name: util.format('%s%d', namePrefix, ri)});
            };

            var nodes = ['a','b','c','d','e'].map((l, i) => makeFakeNode(l, i));

            nodes.forEach(node => { this.OSMDB.addNode(node); });

            // ...with a way between them
            var way = new OSM.Way(this.makeOSMId(), this.OSM_USER, this.OSM_TIMESTAMP, this.OSM_UID);

            nodes.forEach(node => { way.addNode(node); });

            // remove tags that describe expected test result, reject empty tags
            var tags = {};
            for (var rkey in row) {
                if (!rkey.match(/^forw\b/) &&
                    !rkey.match(/^backw\b/) &&
                    !rkey.match(/^bothw\b/) &&
                    row[rkey].length)
                    tags[rkey] = row[rkey];
            }

            var wayTags = { highway: 'primary' },
                nodeTags = {};

            for (var key in tags) {
                var nodeMatch = key.match(/node\/(.*)/);
                if (nodeMatch) {
                    if (tags[key] === '(nil)') {
                        delete nodeTags[key];
                    } else {
                        nodeTags[nodeMatch[1]] = tags[key];
                    }
                } else {
                    if (tags[key] === '(nil)') {
                        delete wayTags[key];
                    } else {
                        wayTags[key] = tags[key];
                    }
                }
            }

            wayTags.name = util.format('w%d', ri);
            way.setTags(wayTags);
            this.OSMDB.addWay(way);

            for (var k in nodeTags) {
                nodes[2].addTag(k, nodeTags[k]);
            }
            cb();
        };

        var q = d3.queue();
        table.hashes().forEach((row, ri) => {
            q.defer(buildRow, row, ri);
        });

        q.awaitAll(callback);
    };

    function ensureDecimal(i) {
        if (parseInt(i) === i) return i.toFixed(1);
        else return i;
    };

    this.tableCoordToLonLat = (ci, ri) => {
        return [this.origin[0] + ci * this.zoom, this.origin[1] - ri * this.zoom].map(ensureDecimal);
    };

    this.addOSMNode = (name, lon, lat, id) => {
        id = id || this.makeOSMId();
        var node = new OSM.Node(id, this.OSM_USER, this.OSM_TIMESTAMP, this.OSM_UID, lon, lat, {name: name});
        this.OSMDB.addNode(node);
        this.nameNodeHash[name] = node;
    };

    this.addLocation = (name, lon, lat) => {
        this.locationHash[name] = new classes.Location(lon, lat);
    };

    this.findNodeByName = (s) => {
        if (s.length !== 1) throw new Error(util.format('*** invalid node name "%s", must be single characters', s));
        if (!s.match(/[a-z0-9]/)) throw new Error(util.format('*** invalid node name "%s", must be alphanumeric', s));

        var fromNode;
        if (s.match(/[a-z]/)) {
            fromNode = this.nameNodeHash[s.toString()];
        } else {
            fromNode = this.locationHash[s.toString()];
        }

        return fromNode;
    };

    this.findWayByName = (s) => {
        return this.nameWayHash[s.toString()] || this.nameWayHash[s.toString().split('').reverse().join('')];
    };

    this.makeOSMId = () => {
        this.osmID = this.osmID + 1;
        return this.osmID;
    };

    this.resetOSM = () => {
        this.OSMDB.clear();
        this.nameNodeHash = {};
        this.locationHash = {};
        this.nameWayHash = {};
        this.osmID = 0;
    };

    this.writeOSM = (callback) => {
      fs.exists(this.scenarioCacheFile, (exists) => {
        if (exists) callback();
        else {
          this.osmData.toXML((xml) => {
            fs.writeFile(this.scenarioCacheFile, xml, callback);
          });
        }
      });
    };

    this.isProcessed = (callback) => {
        fs.exists(this.processedCacheFile, (exists) => {
        });
    };

    this.extractData = (callback) => {
        this.runBin(this.OSRM_EXTRACT_PATH, util.format("%s --profile %s", this.extractArgs, this.profileFile), (err) => {
            if (err) {
                return callback(new Error(err.code, util.format('osrm-extract exited with code %d', err.code)));
            }
            callback();
        });
    };

    this.contractData = (callback) => {
        this.runBin(this.OSRM_CONTRACT_PATH, util.format("%s %s", this.contractArgs, this.processedCacheFile), (err) => {
            if (err) {
                return callback(new Error(err.code, util.format('osrm-contract exited with code %d', err.code)));
            }
            callback();
        });
    };

    this.extractAndContract = (callback) => {
        fs.exists(this.processedCacheFile, (exists) => {
            if (exists) {
                callback();
            } else {
                let queue = d3.queue(1);
                queue.defer(this.extractData.bind(this));
                queue.defer(this.contractData.bind(this));
                queue.awaitAll(callback);
            }
        });
    };

    this.reprocess = (callback) => {
        let queue = d3.queue(1);
        queue.defer(this.writeOSM.bind(this));
        queue.defer(this.extractAndContract.bind(this));
        queue.awaitAll(callback);
    };

    this.reprocessAndLoadData = (callback) => {
        let queue = d3.queue(1);
        queue.defer(this.writeOSM.bind(this));
        queue.defer(this.extractAndContract.bind(this));
        queue.defer(this.osrmLoader.load.bind(this.osrmLoader), this.processedCacheFile)
        queue.awaitAll(callback);
    };

    this.processRowsAndDiff = (table, fn, callback) => {
        var q = d3.queue(1);

        table.hashes().forEach((row, i) => { q.defer(fn, row, i); });

        q.awaitAll((err, actual) => {
            if (err) return callback(err);
            this.diffTables(table, actual, {}, callback);
        });
    };
};
