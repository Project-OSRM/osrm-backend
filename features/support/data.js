'use strict';

const fs = require('fs');
const util = require('util');
const d3 = require('d3-queue');

const OSM = require('../lib/osm');
const classes = require('./data_classes');
const tableDiff = require('../lib/table_diff');
const ensureDecimal = require('../lib/utils').ensureDecimal;
const errorReason = require('../lib/utils').errorReason;

const child_process = require('child_process');

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

    // find a node based on an array containing lon/lat
    this.findNodeByLocation = (node_location) => {
        var searched_coordinate = new classes.Location(node_location[0],node_location[1]);
        for (var node in this.nameNodeHash)
        {
            var node_coordinate = new classes.Location(this.nameNodeHash[node].lon,this.nameNodeHash[node].lat);
            if (this.FuzzyMatch.matchCoordinate(searched_coordinate, node_coordinate, this.zoom))
            {
                return node;
            }
        }
        return '_';
    };

    this.findWayByName = (s) => {
        return this.nameWayHash[s.toString()] || this.nameWayHash[s.toString().split('').reverse().join('')];
    };

    this.findRelationByName = (s) => {
        return this.nameRelationHash[s.toString()] || this.nameRelationHash[s.toString().split('').reverse().join('')];
    };

    this.makeOSMId = () => {
        this.osmID = this.osmID + 1;
        return this.osmID;
    };

    this.resetOSM = () => {
        this.OSMDB.clear();
        this.nameNodeHash = {};
        this.locationHash = {};
        this.shortcutsHash = {};
        this.nameWayHash = {};
        this.nameRelationHash = {};
        this.osmID = 0;
    };

    this.writeOSM = (callback) => {
        fs.exists(this.scenarioCacheFile, (exists) => {
            if (exists) callback();
            else {
                this.OSMDB.toXML((xml) => {
                    fs.writeFile(this.scenarioCacheFile, xml, (err) => {
                        if (err) callback(err); 
                        var params = ['cat',this.scenarioCacheFile,'-O','-o',this.scenarioCacheFilePBF, '--no-progress'];
                        let child = child_process.execFile('/usr/local/bin/osmium', params, {maxBuffer: 1024 * 1024 * 1000, env: {}}, (err,stdout,stderr) => {
                            callback(err);
                        });
                    });
                });
            }
        });
    };

    this.linkOSM = (callback) => {
        fs.exists(this.inputCacheFile, (exists) => {
            if (exists) callback();
            else {
                fs.link(this.scenarioCacheFile, this.inputCacheFile, (err) => {
                    if (err) callback(err);
                    fs.link(this.scenarioCacheFilePBF, this.inputCacheFilePBF, callback);
                });
            }
        });
    };

    this.extractData = (p, callback) => {
        let stamp = p.processedCacheFile + '.stamp_extract';
        fs.exists(stamp, (exists) => {
            if (exists) return callback();

            this.runBin('osrm-extract', util.format('%s --profile %s %s', p.extractArgs, p.profileFile, p.inputCacheFile), p.environment, (err) => {
                if (err) {
                    return callback(new Error(util.format('osrm-extract %s: %s', errorReason(err), err.cmd)));
                }
                fs.writeFile(stamp, 'ok', callback);
            });
        });
    };

    this.contractData = (p, callback) => {
        let stamp = p.processedCacheFile + '.stamp_contract';
        fs.exists(stamp, (exists) => {
            if (exists) return callback();

            this.runBin('osrm-contract', util.format('%s %s', p.contractArgs, p.processedCacheFile), p.environment, (err) => {
                if (err) {
                    return callback(new Error(util.format('osrm-contract %s: %s', errorReason(err), err)));
                }
                fs.writeFile(stamp, 'ok', callback);
            });
        });
    };

    this.partitionData = (p, callback) => {
        let stamp = p.processedCacheFile + '.stamp_partition';
        fs.exists(stamp, (exists) => {
            if (exists) return callback();

            this.runBin('osrm-partition', util.format('%s %s', p.partitionArgs, p.processedCacheFile), p.environment, (err) => {
                if (err) {
                    return callback(new Error(util.format('osrm-partition %s: %s', errorReason(err), err.cmd)));
                }
                fs.writeFile(stamp, 'ok', callback);
            });
        });
    };

    this.customizeData = (p, callback) => {
        let stamp = p.processedCacheFile + '.stamp_customize';
        fs.exists(stamp, (exists) => {
            if (exists) return callback();

            this.runBin('osrm-customize', util.format('%s %s', p.customizeArgs, p.processedCacheFile), p.environment, (err) => {
                if (err) {
                    return callback(new Error(util.format('osrm-customize %s: %s', errorReason(err), err)));
                }
                fs.writeFile(stamp, 'ok', callback);
            });
        });
    };

    this.valhallaBuildConfig = (p, callback) => {
        let stamp = p.processedCacheFile + '.stamp_valhalla_config';
        fs.exists(stamp, (exists) => {
            if (exists) return callback();

            // valhalla_build_config --mjolnir-tile-dir ${PWD}/valhalla_tiles 
            //                       --mjolnir-tile-extract ${PWD}/valhalla_tiles.tar
            //                       --mjolnir-timezone ${PWD}/valhalla_tiles/timezones.sqlite
            //                       --mjolnir-admin ${PWD}/valhalla_tiles/admins.sqlite > valhalla.json

            var params = [`--mjolnir-tile-dir`, `${p.inputCacheDir}/${p.scenarioID}_valhalla_tiles`,
                          `--mjolnir-tile-extract`, `${p.inputCacheDir}/${p.scenarioID}_valhalla_tiles.tar`,
                          `--mjolnir-timezone`, `${p.inputCacheDir}/${p.scenarioID}_valhalla_tiles/timezones.sqlite`,
                          `--mjolnir-admin`, `${p.inputCacheDir}/${p.scenarioID}_valhalla_tiles/admins.sqlite`];

            child_process.execFile(`${process.env.VALHALLA_HOME}/scripts/valhalla_build_config`, params, {}, (error, stdout, stderr) => {
                if (error) { throw error; }
                // Disable heirarchy calculation - it's slow, and not needed for small tests.
                var config = JSON.parse(stdout);
                config.mjolnir.hierarchy = false;
                fs.writeFile(`${p.inputCacheDir}/${p.scenarioID}_valhalla_config.json`, JSON.stringify(config), (err) => {
                    if (err) throw err;
                    fs.writeFile(stamp, 'ok', callback);
                })
            });
        });
    };

    this.valhallaBuildTiles = (p, callback) => {
        let stamp = p.processedCacheFile + '.stamp_valhalla_tiles';
        fs.exists(stamp, (exists) => {
            if (exists) return callback();

            var params = [`-c`,`${p.inputCacheDir}/${p.scenarioID}_valhalla_config.json`,
                          `${p.inputCacheFilePBF}`];

            child_process.execFile(`${process.env.VALHALLA_HOME}/valhalla_build_tiles`, params, {}, (error, stdout, stderr) => {
                if (error) { throw error; }
                console.log(stdout);
                console.log(stderr);
                fs.writeFile(stamp, 'ok', callback);
            });
        });

    };

    this.valhallaTarTiles = (p, callback) => {
        let stamp = p.processedCacheFile + '.stamp_valhalla_tartiles';
        fs.exists(stamp, (exists) => {
            if (exists) return callback();
            var params = [`cf`,`${p.inputCacheDir}/${p.scenarioID}_valhalla_tiles.tar`,
                          `-C`, p.inputCacheDir, `${p.scenarioID}_valhalla_tiles`];

            child_process.execFile('/usr/bin/tar', params, {}, (error, stdout, stderr) => {
                if (error) { throw error; }
                console.log(stdout);
                console.log(stderr);
                fs.writeFile(stamp, 'ok', callback);
            });

        });
    }

    this.extractContractPartitionAndCustomize = (callback) => {
        // a shallow copy of scenario parameters to avoid data inconsistency
        // if a cucumber timeout occurs during deferred jobs
        let p = {extractArgs: this.extractArgs, contractArgs: this.contractArgs,
            partitionArgs: this.partitionArgs, customizeArgs: this.customizeArgs,
            profileFile: this.profileFile, inputCacheFile: this.inputCacheFile,
            inputCacheFilePBF: this.inputCacheFilePBF,
            processedCacheFile: this.processedCacheFile, environment: this.environment,
            inputCacheDir: this.featureProcessedCacheDirectory,
            scenarioID: this.scenarioID };
        let queue = d3.queue(1);
        queue.defer(this.extractData.bind(this), p);
        queue.defer(this.partitionData.bind(this), p);
        queue.defer(this.contractData.bind(this), p);
        queue.defer(this.valhallaBuildConfig.bind(this), p);
        queue.defer(this.valhallaBuildTiles.bind(this), p);
        queue.defer(this.valhallaTarTiles.bind(this), p);
        queue.awaitAll(callback);
    };

    this.writeAndLinkOSM = (callback) => {
        let queue = d3.queue(1);
        let p = { profileFile: this.profileFile, inputCacheFile: this.inputCacheFile,
            inputCacheFilePBF: this.inputCacheFilePBF,
            processedCacheFile: this.processedCacheFile, environment: this.environment,
            inputCacheDir: this.featureProcessedCacheDirectory,
            scenarioID: this.scenarioID };
        queue.defer(this.writeOSM.bind(this));
        queue.defer(this.linkOSM.bind(this));
        queue.awaitAll(callback);
    };

    this.reprocess = (callback) => {
        let queue = d3.queue(1);
        queue.defer(this.writeAndLinkOSM.bind(this));
        queue.defer(this.extractContractPartitionAndCustomize.bind(this));
        queue.awaitAll(callback);
    };

    this.reprocessAndLoadData = (callback) => {
        let queue = d3.queue(1);
        queue.defer(this.writeAndLinkOSM.bind(this));
        queue.defer(this.extractContractPartitionAndCustomize.bind(this));
        //queue.defer(this.osrmLoader.load.bind(this.osrmLoader), this.processedCacheFile);
        queue.defer(this.osrmLoader.load.bind(this.osrmLoader), `${this.featureProcessedCacheDirectory}/${this.scenarioID}_valhalla_config.json`);
        queue.awaitAll(callback);
    };

    this.processRowsAndDiff = (table, fn, callback) => {
        var q = d3.queue(1);

        table.hashes().forEach((row, i) => { q.defer(fn, row, i); });

        q.awaitAll((err, actual) => {
            if (err) return callback(err);
            let diff = tableDiff(table, actual);
            if (diff) callback(diff);
            else callback();
        });
    };
};
