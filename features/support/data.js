// Core data manipulation utilities for building synthetic test scenarios and OSM data
import fs from 'fs';
import util from 'util';
import d3 from 'd3-queue';

import CheapRuler from 'cheap-ruler';
import stripAnsi from 'strip-ansi';

import * as OSM from '../lib/osm.js';
import tableDiff from '../lib/table_diff.js';
import { ensureDecimal } from '../lib/utils.js';
import classes from './data_classes.js';
import { env } from './env.js';
import { runBinSync } from './run.js';

export default class Data {
  constructor(world) {
    this.world = world;
  }

  // Sets grid spacing in meters for coordinate calculations
  setGridSize(meters) {
    this.gridSize = parseFloat(meters);

    // the constant is calculated (with BigDecimal as: 1.0/(DEG_TO_RAD*EARTH_RADIUS_IN_METERS
    // see ApproximateDistance() in ExtractorStructs.h
    // it's only accurate when measuring along the equator, or going exactly north-south
    this.zoom =
      this.gridSize *
      0.8990679362704610899694577444566908445396483347536032203503e-5;
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
    return this.ruler.offset(
      this.origin,
      xCells * this.gridSize,
      yCells * this.gridSize,
    );
  }

  // Builds OSM ways from test table data with synthetic coordinates
  buildWaysFromTable(table, callback) {
    // add one unconnected way for each row
    const buildRow = (row, ri, cb) => {
      // comments ported directly from ruby suite:
      // NOTE: currently osrm crashes when processing an isolated oneway with just 2 nodes, so we use 4 edges
      // this is related to the fact that a oneway dead-end street doesn't make a lot of sense

      // if we stack ways on different x coordinates, routability tests get messed up, because osrm might pick a neighboring way if the one test can't be used.
      // instead we place all lines as a string on the same y coordinate. this prevents using neighboring ways.

      // add some nodes

      // Creates synthetic OSM node with calculated coordinates
      const makeFakeNode = (namePrefix, offset) => {
        const coord = this.offsetOriginBy(offset + env.WAY_SPACING * ri, 0);
        return new OSM.Node(
          this.makeOSMId(),
          env.OSM_USER,
          env.OSM_TIMESTAMP,
          env.OSM_UID,
          coord[0],
          coord[1],
          { name: util.format('%s%d', namePrefix, ri) },
        );
      };

      const nodes = ['a', 'b', 'c', 'd', 'e'].map((l, i) => makeFakeNode(l, i));

      nodes.forEach((node) => {
        this.OSMDB.addNode(node);
      });

      // ...with a way between them
      const way = new OSM.Way(
        this.makeOSMId(),
        env.OSM_USER,
        env.OSM_TIMESTAMP,
        env.OSM_UID,
      );

      nodes.forEach((node) => {
        way.addNode(node);
      });

      // remove tags that describe expected test result, reject empty tags
      const tags = {};
      for (const rkey in row) {
        if (
          !rkey.match(/^forw\b/) &&
          !rkey.match(/^backw\b/) &&
          !rkey.match(/^bothw\b/) &&
          row[rkey].length
        )
          tags[rkey] = row[rkey];
      }

      const wayTags = { highway: 'primary' };
      const nodeTags = {};

      for (const key in tags) {
        const nodeMatch = key.match(/node\/(.*)/);
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

      for (const k in nodeTags) {
        nodes[2].addTag(k, nodeTags[k]);
      }
      cb();
    };

    const q = d3.queue();
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
    const node = new OSM.Node(
      id,
      env.OSM_USER,
      env.OSM_TIMESTAMP,
      env.OSM_UID,
      lon,
      lat,
      { name },
    );
    this.OSMDB.addNode(node);
    this.nameNodeHash[name] = node;
  }

  // Adds named location for test coordinate references
  addLocation(name, lon, lat) {
    this.locationHash[name] = new classes.Location(lon, lat);
  }

  // Finds OSM node or location by single character name
  findNodeByName(s) {
    if (s.length !== 1)
      throw new Error(
        util.format('*** invalid node name "%s", must be single characters', s),
      );
    if (!s.match(/[a-z0-9]/))
      throw new Error(
        util.format('*** invalid node name "%s", must be alphanumeric', s),
      );

    let fromNode;
    if (s.match(/[a-z]/)) {
      fromNode = this.nameNodeHash[s.toString()];
    } else {
      fromNode = this.locationHash[s.toString()];
    }

    return fromNode;
  }

  // find a node based on an array containing lon/lat
  findNodeByLocation(node_location) {
    const searched_coordinate = new classes.Location(
      node_location[0],
      node_location[1],
    );
    for (const node in this.nameNodeHash) {
      const node_coordinate = new classes.Location(
        this.nameNodeHash[node].lon,
        this.nameNodeHash[node].lat,
      );
      if (
        this.FuzzyMatch.matchCoordinate(
          searched_coordinate,
          node_coordinate,
          this.zoom,
        )
      ) {
        return node;
      }
    }
    return '_';
  }

  findWayByName(s) {
    return (
      this.nameWayHash[s.toString()] ||
      this.nameWayHash[s.toString().split('').reverse().join('')]
    );
  }

  findRelationByName(s) {
    return (
      this.nameRelationHash[s.toString()] ||
      this.nameRelationHash[s.toString().split('').reverse().join('')]
    );
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

  writeOSM() {
    if (fs.existsSync(this.osmCacheFile))
      return;
    this.OSMDB.toXML((xml) => {
      fs.writeFileSync(this.osmCacheFile, xml);
    });
  }

  runAndStamp(what, extra_params, params) {
    const stampFile = `${this.osrmCacheFile}.stamp_${what}`;
    if (!fs.existsSync(stampFile)) {
      runBinSync(
        `osrm-${what}`,
        extra_params.concat(params),
        { env : this.environment },
        this.log
      );
      fs.writeFileSync(stampFile, 'ok');
    }
    return Promise.resolve();
  }

  extract() {
    return this.runAndStamp('extract', this.extractArgs, [
      '-p',
      this.profileFile,
      this.osmCacheFile,
    ]);
  }

  partition() {
    return this.runAndStamp('partition', this.partitionArgs, [
      this.osrmCacheFile
    ]);
  }

  customize() {
    return this.runAndStamp('customize', this.customizeArgs, [
      this.osrmCacheFile
    ]);
  }

  contract() {
    return this.runAndStamp('contract', this.contractArgs, [
      this.osrmCacheFile
    ]);
  }

  /**
   * Runs the complete extraction chain with one .osm file.
   */
  runExtractionChain() {
    this.extract();
    this.partition(); // mld
    this.customize(); // mld
    this.contract();  // ch
  }

  reprocess(callback) {
    this.writeOSM();
    this.runExtractionChain();
    callback();
  }

  // This is called on every "When I X I should Y"
  // On the first call in every scenario it should load the data
  // into osrm-routed or osrm-datastore
  async reprocessAndLoadData(callback) {
    if (!this.dataLoaded) {
      this.writeOSM();
      this.runExtractionChain();
      await env.osrmLoader.before(this);
      this.dataLoaded = true;
    }
    callback();
  }

  processRowsAndDiff(table, fn, callback) {
    const q = d3.queue(1);

    table.hashes().forEach((row, i) => {
      q.defer(fn, row, i);
    });

    q.awaitAll((err, actual) => {
      if (err)
        return callback(err);
      const diff = tableDiff(table, actual);
      if (diff) {
        if (env.PLATFORM_CI) {
          // the github report displays ANSI escapes as characters if passed to the
          // error callback.
          callback(stripAnsi(diff));
        } else {
          callback(diff);
        }
      } else {
        callback();
      }
    });
  }
}
