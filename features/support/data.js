// Core data manipulation utilities for building synthetic test scenarios and OSM data
import fs from 'fs';
import util from 'util';
import d3 from 'd3-queue';

import * as OSM from '../lib/osm.js';
import classes from './data_classes.js';
import tableDiff from '../lib/table_diff.js';
import { ensureDecimal, errorReason } from '../lib/utils.js';
import CheapRuler from 'cheap-ruler';

export default class Data {
  constructor() {}

  // Sets grid spacing in meters for coordinate calculations
  setGridSize(meters) {
    this.gridSize = parseFloat(meters);

    // the constant is calculated (with BigDecimal as: 1.0/(DEG_TO_RAD*EARTH_RADIUS_IN_METERS
    // see ApproximateDistance() in ExtractorStructs.h
    // it's only accurate when measuring along the equator, or going exactly north-south
    this.zoom = this.gridSize * 0.8990679362704610899694577444566908445396483347536032203503E-5;
  }

  // Sets base coordinate for test scenario positioning
  setOrigin(origin) {
    this.origin = origin;
    // we use C++ version of `cheap-ruler` inside OSRM in order to do distance calculations,
    // so here we use it too to have a bit more precise assertions
    this.ruler = new CheapRuler(this.origin[1], 'meters');
  }

  // Calculates coordinate offset by grid cells from origin
  offsetOriginBy(xCells, yCells) {
    return this.ruler.offset(this.origin, xCells * this.gridSize, yCells * this.gridSize);
  }

  // Builds OSM ways from test table data with synthetic coordinates
  buildWaysFromTable(table, callback) {
    // add one unconnected way for each row
    var buildRow = function (row, ri, cb) {
      // comments ported directly from ruby suite:
      // NOTE: currently osrm crashes when processing an isolated oneway with just 2 nodes, so we use 4 edges
      // this is related to the fact that a oneway dead-end street doesn't make a lot of sense

      // if we stack ways on different x coordinates, routability tests get messed up, because osrm might pick a neighboring way if the one test can't be used.
      // instead we place all lines as a string on the same y coordinate. this prevents using neighboring ways.

      // add some nodes

      // Creates synthetic OSM node with calculated coordinates
      var makeFakeNode = function (namePrefix, offset) {
        const coord = this.offsetOriginBy(offset + this.WAY_SPACING * ri, 0);
        return new OSM.Node(this.makeOSMId(), this.OSM_USER, this.OSM_TIMESTAMP,
          this.OSM_UID, coord[0],
          coord[1], {name: util.format('%s%d', namePrefix, ri)});
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
  }

  // Converts table grid coordinates to longitude/latitude
  tableCoordToLonLat(ci, ri) {
    return this.offsetOriginBy(ci, -ri).map(ensureDecimal);
  }

  // Adds named OSM node to test database
  addOSMNode(name, lon, lat, id) {
    id = id || this.makeOSMId();
    var node = new OSM.Node(id, this.OSM_USER, this.OSM_TIMESTAMP, this.OSM_UID, lon, lat, {name: name});
    this.OSMDB.addNode(node);
    this.nameNodeHash[name] = node;
  }

  // Adds named location for test coordinate references
  addLocation(name, lon, lat) {
    this.locationHash[name] = new classes.Location(lon, lat);
  }

  // Finds OSM node or location by single character name
  findNodeByName(s) {
    if (s.length !== 1) throw new Error(util.format('*** invalid node name "%s", must be single characters', s));
    if (!s.match(/[a-z0-9]/)) throw new Error(util.format('*** invalid node name "%s", must be alphanumeric', s));

    var fromNode;
    if (s.match(/[a-z]/)) {
      fromNode = this.nameNodeHash[s.toString()];
    } else {
      fromNode = this.locationHash[s.toString()];
    }

    return fromNode;
  }

  // find a node based on an array containing lon/lat
  findNodeByLocation(node_location) {
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
  }

  findWayByName(s) {
    return this.nameWayHash[s.toString()] || this.nameWayHash[s.toString().split('').reverse().join('')];
  }

  findRelationByName(s) {
    return this.nameRelationHash[s.toString()] || this.nameRelationHash[s.toString().split('').reverse().join('')];
  }

  makeOSMId() {
    this.osmID = this.osmID + 1;
    return this.osmID;
  }

  resetOSM() {
    this.OSMDB.clear();
    this.nameNodeHash = {};
    this.locationHash = {};
    this.shortcutsHash = {};
    this.nameWayHash = {};
    this.nameRelationHash = {};
    this.osmID = 0;
  }

  writeOSM(callback) {
    fs.exists(this.scenarioCacheFile, (exists) => {
      if (exists) callback();
      else {
        this.OSMDB.toXML((xml) => {
          fs.writeFile(this.scenarioCacheFile, xml, callback);
        });
      }
    });
  }

  linkOSM(callback) {
    fs.exists(this.inputCacheFile, (exists) => {
      if (exists) callback();
      else {
        fs.link(this.scenarioCacheFile, this.inputCacheFile, callback);
      }
    });
  }

  extractData(p, callback) {
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
  }

  contractData(p, callback) {
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
  }

  partitionData(p, callback) {
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
  }

  customizeData(p, callback) {
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
  }

  extractContractPartitionAndCustomize(callback) {
    // a shallow copy of scenario parameters to avoid data inconsistency
    // if a cucumber timeout occurs during deferred jobs
    let p = {extractArgs: this.extractArgs, contractArgs: this.contractArgs,
      partitionArgs: this.partitionArgs, customizeArgs: this.customizeArgs,
      profileFile: this.profileFile, inputCacheFile: this.inputCacheFile,
      processedCacheFile: this.processedCacheFile, environment: this.environment};
    let queue = d3.queue(1);
    queue.defer(this.extractData.bind(this), p);
    queue.defer(this.partitionData.bind(this), p);
    queue.defer(this.contractData.bind(this), p);
    queue.defer(this.customizeData.bind(this), p);
    queue.awaitAll(callback);
  }

  writeAndLinkOSM(callback) {
    let queue = d3.queue(1);
    queue.defer(this.writeOSM.bind(this));
    queue.defer(this.linkOSM.bind(this));
    queue.awaitAll(callback);
  }

  reprocess(callback) {
    let queue = d3.queue(1);
    queue.defer(this.writeAndLinkOSM.bind(this));
    queue.defer(this.extractContractPartitionAndCustomize.bind(this));
    queue.awaitAll(callback);
  }

  reprocessAndLoadData(callback) {
    let p = {loaderArgs: this.loaderArgs, inputFile: this.processedCacheFile};
    let queue = d3.queue(1);
    queue.defer(this.writeAndLinkOSM.bind(this));
    queue.defer(this.extractContractPartitionAndCustomize.bind(this));
    queue.defer(this.osrmLoader.load.bind(this.osrmLoader), p);
    queue.awaitAll(callback);
  }

  processRowsAndDiff(table, fn, callback) {
    var q = d3.queue(1);

    table.hashes().forEach((row, i) => { q.defer(fn, row, i); });

    q.awaitAll((err, actual) => {
      if (err) return callback(err);
      let diff = tableDiff(table, actual);
      if (diff) callback(diff);
      else callback();
    });
  }
}
