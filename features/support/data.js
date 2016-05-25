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

    this.ensureDecimal = (i) => {
        if (parseInt(i) === i) return i.toFixed(1);
        else return i;
    };

    this.tableCoordToLonLat = (ci, ri) => {
        return [this.origin[0] + ci * this.zoom, this.origin[1] - ri * this.zoom].map(this.ensureDecimal);
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

    this.resetData = () => {
        this.resetOSM();
    };

    this.makeOSMId = () => {
        this.osmID = this.osmID + 1;
        return this.osmID;
    };

    this.resetOSM = () => {
        this.OSMDB.clear();
        this.osmData.reset();
        this.nameNodeHash = {};
        this.locationHash = {};
        this.nameWayHash = {};
        this.osmID = 0;
    };

    this.writeOSM = (callback) => {
        fs.exists(this.DATA_FOLDER, (exists) => {
            var mkDirFn = exists ? (cb) => { cb(); } : fs.mkdir.bind(fs.mkdir, this.DATA_FOLDER);
            mkDirFn((err) => {
                if (err) return callback(err);
                var osmPath = path.resolve(this.DATA_FOLDER, util.format('%s.osm', this.osmData.osmFile));
                fs.exists(osmPath, (exists) => {
                    if (!exists) fs.writeFile(osmPath, this.osmData.str, callback);
                    else callback();
                });
            });
        });
    };

    this.isExtracted = (callback) => {
        fs.exists(util.format('%s.osrm', this.osmData.extractedFile), (core) => {
            if (!core) return callback(false);
            fs.exists(util.format('%s.osrm.names', this.osmData.extractedFile), (names) => {
                if (!names) return callback(false);
                fs.exists(util.format('%s.osrm.restrictions', this.osmData.extractedFile), (restrictions) => {
                    return callback(restrictions);
                });
            });
        });
    };

    this.isContracted = (callback) => {
        fs.exists(util.format('%s.osrm.hsgr', this.osmData.contractedFile), callback);
    };

    this.writeTimestamp = (callback) => {
        fs.writeFile(util.format('%s.osrm.timestamp', this.osmData.contractedFile), this.OSM_TIMESTAMP, callback);
    };

    this.writeInputData = (callback) => {
        this.writeOSM((err) => {
            if (err) return callback(err);
            this.writeTimestamp(callback);
        });
    };

    this.extractData = (callback) => {
        this.logPreprocessInfo();
        this.log(util.format('== Extracting %s.osm...', this.osmData.osmFile), 'preprocess');
        var cmd = util.format('%s%s/osrm-extract %s.osm %s --profile %s/%s.lua >>%s 2>&1',
            this.LOAD_LIBRARIES, this.BIN_PATH, this.osmData.osmFile, this.extractArgs || '', this.PROFILES_PATH, this.profile, this.PREPROCESS_LOG_FILE);
        this.log(cmd);
        process.chdir(this.TEST_FOLDER);
        exec(cmd, (err) => {
            if (err) {
                this.log(util.format('*** Exited with code %d', err.code), 'preprocess');
                process.chdir('../');
                return callback(this.ExtractError(err.code, util.format('osrm-extract exited with code %d', err.code)));
            }

            var q = d3.queue();

            var rename = (file, cb) => {
                this.log(util.format('Renaming %s.%s to %s.%s', this.osmData.osmFile, file, this.osmData.extractedFile, file), 'preprocess');
                fs.rename([this.osmData.osmFile, file].join('.'), [this.osmData.extractedFile, file].join('.'), (err) => {
                    if (err) return cb(this.FileError(null, 'failed to rename data file after extracting'));
                    cb();
                });
            };

            var renameIfExists = (file, cb) => {
                fs.stat([this.osmData.osmFile, file].join('.'), (doesNotExistErr, exists) => {
                    if (exists) rename(file, cb);
                    else cb();
                });
            };

            ['osrm','osrm.names','osrm.restrictions','osrm.ebg','osrm.enw','osrm.edges','osrm.fileIndex','osrm.geometry','osrm.nodes','osrm.ramIndex','osrm.properties','osrm.icd'].forEach(file => {
                q.defer(rename, file);
            });

            ['osrm.edge_segment_lookup','osrm.edge_penalties'].forEach(file => {
                q.defer(renameIfExists, file);
            });

            q.awaitAll((err) => {
                this.log('Finished extracting ' + this.osmData.extractedFile, 'preprocess');
                process.chdir('../');
                callback(err);
            });
        });
    };

    this.contractData = (callback) => {
        this.logPreprocessInfo();
        this.log(util.format('== Contracting %s.osm...', this.osmData.extractedFile), 'preprocess');
        var cmd = util.format('%s%s/osrm-contract %s %s.osrm >>%s 2>&1',
            this.LOAD_LIBRARIES, this.BIN_PATH, this.contractArgs || '', this.osmData.extractedFile, this.PREPROCESS_LOG_FILE);
        this.log(cmd);
        process.chdir(this.TEST_FOLDER);
        exec(cmd, (err) => {
            if (err) {
                this.log(util.format('*** Exited with code %d', err.code), 'preprocess');
                process.chdir('../');
                return callback(this.ContractError(err.code, util.format('osrm-contract exited with code %d', err.code)));
            }

            var rename = (file, cb) => {
                this.log(util.format('Renaming %s.%s to %s.%s', this.osmData.extractedFile, file, this.osmData.contractedFile, file), 'preprocess');
                fs.rename([this.osmData.extractedFile, file].join('.'), [this.osmData.contractedFile, file].join('.'), (err) => {
                    if (err) return cb(this.FileError(null, 'failed to rename data file after contracting.'));
                    cb();
                });
            };

            var copy = (file, cb) => {
                this.log(util.format('Copying %s.%s to %s.%s', this.osmData.extractedFile, file, this.osmData.contractedFile, file), 'preprocess');
                fs.createReadStream([this.osmData.extractedFile, file].join('.'))
                    .pipe(fs.createWriteStream([this.osmData.contractedFile, file].join('.'))
                            .on('finish', cb)
                        )
                    .on('error', () => {
                        return cb(this.FileError(null, 'failed to copy data after contracting.'));
                    });
            };

            var q = d3.queue();

            ['osrm.hsgr','osrm.fileIndex','osrm.geometry','osrm.nodes','osrm.ramIndex','osrm.core','osrm.edges','osrm.datasource_indexes','osrm.datasource_names','osrm.level','osrm.icd'].forEach((file) => {
                q.defer(rename, file);
            });

            ['osrm.names','osrm.restrictions','osrm.properties','osrm'].forEach((file) => {
                q.defer(copy, file);
            });

            q.awaitAll((err) => {
                this.log('Finished contracting ' + this.osmData.contractedFile, 'preprocess');
                process.chdir('../');
                callback(err);
            });
        });
    };

    var noop = (cb) => cb();

    this.reprocess = (callback) => {
        this.writeAndExtract((e) => {
            if (e) return callback(e);
            this.isContracted((isContracted) => {
                var contractFn = (isContracted && !this.forceContract) ? noop : this.contractData;
                if (isContracted) this.log('Already contracted ' + this.osmData.contractedFile, 'preprocess');
                contractFn((e) => {
                    this.forceContract = false;
                    if (e) return callback(e);
                    this.logPreprocessDone();
                    callback();
                });
            });
        });
    };

    this.writeAndExtract = (callback) => {
        this.osmData.populate(() => {
            this.writeInputData((e) => {
                if (e) return callback(e);
                this.isExtracted((isExtracted) => {
                    var extractFn = (isExtracted && !this.forceExtract) ? noop : this.extractData;
                    if (isExtracted) this.log('Already extracted ' + this.osmData.extractedFile, 'preprocess');
                    extractFn((e) => {
                        this.forceExtract = false;
                        callback(e);
                    });
                });
            });
        });
    };

    this.reprocessAndLoadData = (callback) => {
        this.reprocess(() => {
            this.OSRMLoader.load(util.format('%s.osrm', this.osmData.contractedFile), callback);
        });
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
