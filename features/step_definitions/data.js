var util = require('util');
var path = require('path');
var fs = require('fs');
var d3 = require('d3-queue');
var OSM = require('../support/build_osm');

module.exports = function () {
    this.Given(/^the profile "([^"]*)"$/, (profile, callback) => {
        this.profile = profile;
        this.profileFile = path.join([this.PROFILES_PATH, this.profile + '.lua']);
        callback();
    });

    this.Given(/^the extract extra arguments "(.*?)"$/, (args, callback) => {
        this.extractArgs = args;
        callback();
    });

    this.Given(/^the contract extra arguments "(.*?)"$/, (args, callback) => {
        this.contractArgs = args;
        callback();
    });

    this.Given(/^a grid size of ([0-9.]+) meters$/, (meters, callback) => {
        this.setGridSize(meters);
        callback();
    });

    this.Given(/^the origin ([-+]?[0-9]*\.?[0-9]+),([-+]?[0-9]*\.?[0-9]+)$/, (lat, lon, callback) => {
        this.setOrigin([parseFloat(lon), parseFloat(lat)]);
        callback();
    });

    this.Given(/^the shortcuts$/, (table, callback) => {
        var q = d3.queue();

        var addShortcut = (row, cb) => {
            this.shortcutsHash[row.key] = row.value;
            cb();
        };

        table.hashes().forEach((row) => {
            q.defer(addShortcut, row);
        });

        q.awaitAll(callback);
    });

    this.Given(/^the node map$/, (table, callback) => {
        var q = d3.queue();

        var addNode = (name, ri, ci, cb) => {
            if (name) {
                var nodeWithID = name.match(/([a-z])\:([0-9]*)/);
                if (nodeWithID) {
                    var nodeName = nodeWithID[1],
                        nodeID = nodeWithID[2];
                    if (this.nameNodeHash[nodeName]) throw new Error(util.format('*** duplicate node %s', name));
                    lonLat = this.tableCoordToLonLat(ci, ri);
                    this.addOSMNode(nodeName, lonLat[0], lonLat[1], nodeID);
                } else {
                    if (name.length !== 1) throw new Error(util.format('*** node invalid name %s, must be single characters', name));
                    if (!name.match(/[a-z0-9]/)) throw new Error(util.format('*** invalid node name %s, must me alphanumeric', name));

                    var lonLat;
                    if (name.match(/[a-z]/)) {
                        if (this.nameNodeHash[name]) throw new Error(util.format('*** duplicate node %s', name));
                        lonLat = this.tableCoordToLonLat(ci, ri);
                        this.addOSMNode(name, lonLat[0], lonLat[1], null);
                    } else {
                        if (this.locationHash[name]) throw new Error(util.format('*** duplicate node %s'), name);
                        lonLat = this.tableCoordToLonLat(ci, ri);
                        this.addLocation(name, lonLat[0], lonLat[1], null);
                    }
                }

                cb();
            }
            else cb();
        };

        table.raw().forEach((row, ri) => {
            row.forEach((name, ci) => {
                q.defer(addNode, name, ri, ci);
            });
        });

        q.awaitAll(callback);
    });

    this.Given(/^the node locations$/, (table, callback) => {
        var q = d3.queue();

        var addNodeLocations = (row, cb) => {
            var name = row.node;
            if (this.findNodeByName(name)) throw new Error(util.format('*** duplicate node %s'), name);

            if (name.match(/[a-z]/)) {
                var id = row.id && parseInt(row.id);
                this.addOSMNode(name, row.lon, row.lat, id);
            } else {
                this.addLocation(name, row.lon, row.lat);
            }

            cb();
        };

        table.hashes().forEach((row) => q.defer(addNodeLocations, row));

        q.awaitAll(callback);
    });

    this.Given(/^the nodes$/, (table, callback) => {
        var q = d3.queue();

        var addNode = (row, cb) => {
            var name = row.node,
                node = this.findNodeByName(name);
            delete row.node;
            if (!node) throw new Error(util.format('*** unknown node %s'), name);
            for (var key in row) {
                node.addTag(key, row[key]);
            }
            cb();
        };

        table.hashes().forEach((row) => q.defer(addNode, row));

        q.awaitAll(callback);
    });

    this.Given(/^the ways$/, (table, callback) => {
        if (this.osm_str) throw new Error('*** Map data already defined - did you pass an input file in this scenario?');

        var q = d3.queue();

        var addWay = (row, cb) => {
            var way = new OSM.Way(this.makeOSMId(), this.OSM_USER, this.OSM_TIMESTAMP, this.OSM_UID);

            var nodes = row.nodes;
            if (this.nameWayHash.nodes) throw new Error(util.format('*** duplicate way %s', nodes));

            for (var i=0; i<nodes.length; i++) {
                var c = nodes[i];
                if (!c.match(/[a-z]/)) throw new Error(util.format('*** ways can only use names a-z (%s)', c));
                var node = this.findNodeByName(c);
                if (!node) throw new Error(util.format('*** unknown node %s', c));
                way.addNode(node);
            }

            var tags = {
                highway: 'primary'
            };

            for (var key in row) {
                tags[key] = row[key];
            }

            delete tags.nodes;

            if (row.highway === '(nil)') delete tags.highway;

            if (row.name === undefined)
                tags.name = nodes;
            else if (row.name === '""' || row.name === "''") // eslint-disable-line quotes
                tags.name = '';
            else if (row.name === '' || row.name === '(nil)')
                delete tags.name;
            else
                tags.name = row.name;

            way.setTags(tags);
            this.OSMDB.addWay(way);
            this.nameWayHash[nodes] = way;
            cb();
        };

        table.hashes().forEach((row) => q.defer(addWay, row));

        q.awaitAll(callback);
    });

    this.Given(/^the relations$/, (table, callback) => {
        if (this.osm_str) throw new Error('*** Map data already defined - did you pass an input file in this scenario?');

        var q = d3.queue();

        var addRelation = (row, cb) => {
            var relation = new OSM.Relation(this.makeOSMId(), this.OSM_USER, this.OSM_TIMESTAMP, this.OSM_UID);

            for (var key in row) {
                var isNode = key.match(/^node:(.*)/),
                    isWay = key.match(/^way:(.*)/),
                    isColonSeparated = key.match(/^(.*):(.*)/);
                if (isNode) {
                    row[key].split(',').map(function(v) { return v.trim(); }).forEach((nodeName) => {
                        if (nodeName.length !== 1) throw new Error(util.format('*** invalid relation node member "%s"'), nodeName);
                        var node = this.findNodeByName(nodeName);
                        if (!node) throw new Error(util.format('*** unknown relation node member "%s"'), nodeName);
                        relation.addMember('node', node.id, isNode[1]);
                    });
                } else if (isWay) {
                    row[key].split(',').map(function(v) { return v.trim(); }).forEach((wayName) => {
                        var way = this.findWayByName(wayName);
                        if (!way) throw new Error(util.format('*** unknown relation way member "%s"'), wayName);
                        relation.addMember('way', way.id, isWay[1]);
                    });
                } else if (isColonSeparated && isColonSeparated[1] !== 'restriction') {
                    throw new Error(util.format('*** unknown relation member type "%s:%s", must be either "node" or "way"'), isColonSeparated[1], isColonSeparated[2]);
                } else {
                    relation.addTag(key, row[key]);
                }
            }
            relation.uid = this.OSM_UID;

            this.OSMDB.addRelation(relation);

            cb();
        };

        table.hashes().forEach((row) => q.defer(addRelation, row));

        q.awaitAll(callback);
    });

    this.Given(/^the input file ([^"]*)$/, (file, callback) => {
        if (path.extname(file) !== '.osm') throw new Error('*** Input file must be in .osm format');
        fs.readFile(file, 'utf8', (err, data) => {
            if (!err) this.osm_str = data.toString();
            callback(err);
        });
    });

    this.Given(/^the raster source$/, (data, callback) => {
        fs.writeFile(path.resolve(this.TEST_PATH, 'rastersource.asc'), data, callback);
    });

    this.Given(/^the speed file$/, (data, callback) => {
        fs.writeFile(path.resolve(this.TEST_PATH, 'speeds.csv'), data, callback);
    });

    this.Given(/^the turn penalty file$/, (data, callback) => {
        fs.writeFile(path.resolve(this.TEST_PATH, 'penalties.csv'), data, callback);
    });

    this.Given(/^the data has been saved to disk$/, (callback) => {
        this.reprocess(callback);
    });

    this.Given(/^the data has been extracted$/, (callback) => {
        this.reprocess(callback);
    });

    this.Given(/^the data has been contracted$/, (callback) => {
        this.reprocess(callback);
    });

    this.Given(/^osrm\-routed is stopped$/, (callback) => {
        this.OSRMLoader.shutdown(callback);
    });

    this.Given(/^data is loaded directly/, (callback) => {
        this.loadMethod = 'directly';
        callback();
    });

    this.Given(/^data is loaded with datastore$/, (callback) => {
        this.loadMethod = 'datastore';
        callback();
    });

    this.Given(/^the HTTP method "([^"]*)"$/, (method, callback) => {
        this.httpMethod = method;
        callback();
    });
};
