'use strict';

var util = require('util');
var path = require('path');
var fs = require('fs');
var d3 = require('d3-queue');
var OSM = require('../lib/osm');

module.exports = function () {
    this.Given(/^the profile "([^"]*)"$/, (profile, callback) => {
        this.profile = profile;
        this.profileFile = path.join(this.PROFILES_PATH, this.profile + '.lua');
        callback();
    });

    this.Given(/^the extract extra arguments "(.*?)"$/, (args, callback) => {
        this.extractArgs = this.expandOptions(args);
        callback();
    });

    this.Given(/^the contract extra arguments "(.*?)"$/, (args, callback) => {
        this.contractArgs = this.expandOptions(args);
        callback();
    });

    this.Given(/^the partition extra arguments "(.*?)"$/, (args, callback) => {
        this.partitionArgs = this.expandOptions(args);
        callback();
    });

    this.Given(/^the customize extra arguments "(.*?)"$/, (args, callback) => {
        this.customizeArgs = this.expandOptions(args);
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
        let q = d3.queue();

        let addShortcut = (row, cb) => {
            this.shortcutsHash[row.key] = row.value;
            cb();
        };

        table.hashes().forEach((row) => {
            q.defer(addShortcut, row);
        });

        q.awaitAll(callback);
    });

    this.Given(/^the node map$/, (docstring, callback) => {
        var q = d3.queue();

        var addNode = (name, ri, ci, cb) => {
            var lonLat = this.tableCoordToLonLat(ci, ri);
            if (name.match(/[a-z]/) ) {
                if (this.nameNodeHash[name]) throw new Error(util.format('*** duplicate node %s', name));
                this.addOSMNode(name, lonLat[0], lonLat[1], null);
            } else if (name.match(/[0-9]/) ) {
                if (this.locationHash[name]) throw new Error(util.format('*** duplicate node %s'), name);
                this.addLocation(name, lonLat[0], lonLat[1], null);
            }
            cb();
        };

        docstring.split(/\n/).forEach( (row,ri) => {
            row.split('').forEach( (cell,ci) => {
                if( cell.match(/[a-z0-9]/) ) {
                    q.defer(addNode, cell, ri, ci*0.5);
                }
            });
        });

        q.awaitAll(callback);
    });

    this.Given(/^the node locations$/, (table, callback) => {
        let q = d3.queue();

        let addNodeLocations = (row, cb) => {
            let name = row.node;
            if (this.findNodeByName(name)) throw new Error(util.format('*** duplicate node %s'), name);

            if (name.match(/[a-z]/)) {
                let id = row.id && parseInt(row.id);
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
        let q = d3.queue();

        let addNode = (row, cb) => {
            let name = row.node,
                node = this.findNodeByName(name);
            delete row.node;
            if (!node) throw new Error(util.format('*** unknown node %s', name));
            for (let key in row) {
                if (key=='id') {
                    node.setID( row[key] );
                } else {
                    node.addTag(key, row[key]);
                }
            }
            cb();
        };

        table.hashes().forEach((row) => q.defer(addNode, row));

        q.awaitAll(callback);
    });

    this.Given(/^the ways$/, (table, callback) => {
        if (this.osm_str) throw new Error('*** Map data already defined - did you pass an input file in this scenario?');

        let q = d3.queue();

        let addWay = (row, cb) => {
            let way = new OSM.Way(this.makeOSMId(), this.OSM_USER, this.OSM_TIMESTAMP, this.OSM_UID);

            let nodes = row.nodes;
            if (this.nameWayHash.nodes) throw new Error(util.format('*** duplicate way %s', nodes));

            for (let i=0; i<nodes.length; i++) {
                let c = nodes[i];
                if (!c.match(/[a-z]/)) throw new Error(util.format('*** ways can only use names a-z (%s)', c));
                let node = this.findNodeByName(c);
                if (!node) throw new Error(util.format('*** unknown node %s', c));
                way.addNode(node);
            }

            let tags = {
                highway: 'primary'
            };

            for (let key in row) {
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

        let q = d3.queue();

        let addRelation = (row, cb) => {
            let relation = new OSM.Relation(this.makeOSMId(), this.OSM_USER, this.OSM_TIMESTAMP, this.OSM_UID);

            for (let key in row) {
                let isNode = key.match(/^node:(.*)/),
                    isWay = key.match(/^way:(.*)/),
                    isColonSeparated = key.match(/^(.*):(.*)/);
                if (isNode) {
                    row[key].split(',').map(function(v) { return v.trim(); }).forEach((nodeName) => {
                        if (nodeName.length !== 1) throw new Error(util.format('*** invalid relation node member "%s"'), nodeName);
                        let node = this.findNodeByName(nodeName);
                        if (!node) throw new Error(util.format('*** unknown relation node member "%s"'), nodeName);
                        relation.addMember('node', node.id, isNode[1]);
                    });
                } else if (isWay) {
                    row[key].split(',').map(function(v) { return v.trim(); }).forEach((wayName) => {
                        let way = this.findWayByName(wayName);
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
        // TODO: Don't overwrite if it exists
        fs.writeFile(this.rasterCacheFile, data, callback);
        // we need this to pass it to the profiles
        this.environment = Object.assign({OSRM_RASTER_SOURCE: this.rasterCacheFile}, this.environment);
    });

    this.Given(/^the speed file$/, (data, callback) => {
        // TODO: Don't overwrite if it exists
        fs.writeFile(this.speedsCacheFile, data, callback);
    });

    this.Given(/^the turn penalty file$/, (data, callback) => {
        // TODO: Don't overwrite if it exists
        fs.writeFile(this.penaltiesCacheFile, data, callback);
    });

    this.Given(/^the profile file(?: "([^"]*)" extended with)?$/, (profile, data, callback) => {
        const lua_profiles_path = this.PROFILES_PATH.split(path.sep).join('/');
        let text = 'package.path = "' + lua_profiles_path + '/?.lua;" .. package.path\n';
        if (profile == null) {
            text += data + '\n';
        } else {
            text += 'local f = assert(io.open("' + lua_profiles_path + '/' + profile + '.lua", "r"))\n';
            text += 'local s = f:read("*all") .. [[\n' + data + '\n]]\n';
            text += 'f:close()\n';
            text += 'local m = assert(loadstring and loadstring(s) or load(s))\n';
            text += 'm()\n';
        }
        this.profileFile = this.profileCacheFile;
        // TODO: Don't overwrite if it exists
        fs.writeFile(this.profileCacheFile, text, callback);
    });

    this.Given(/^the data has been saved to disk$/, (callback) => {
        this.writeAndLinkOSM(callback);
    });

    this.Given(/^the data has been (extract|contract|partition|customiz)ed$/, (step, callback) => {
        this.reprocess(callback);
    });

    this.Given(/^osrm\-routed is stopped$/, (callback) => {
        this.OSRMLoader.shutdown(callback);
    });

    this.Given(/^data is loaded directly/, (callback) => {
        this.osrmLoader.setLoadMethod('directly');
        callback();
    });

    this.Given(/^data is loaded with datastore$/, (callback) => {
        this.osrmLoader.setLoadMethod('datastore');
        callback();
    });

    this.Given(/^the HTTP method "([^"]*)"$/, (method, callback) => {
        this.httpMethod = method;
        callback();
    });
};
