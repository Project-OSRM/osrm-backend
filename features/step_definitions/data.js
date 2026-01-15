// Step definitions for setting up test data, profiles, and OSM scenarios
import util from 'util';
import path from 'path';
import fs from 'fs';
import d3 from 'd3-queue';
import * as OSM from '../lib/osm.js';
import { Given } from '@cucumber/cucumber';

Given(/^the profile "([^"]*)"$/, function (profile, callback) {
  this.profile = this.OSRM_PROFILE || profile;
  this.profileFile = path.join(this.PROFILES_PATH, `${this.profile}.lua`);
  callback();
});

Given(/^the extract extra arguments "(.*?)"$/, function (args, callback) {
  this.extractArgs = this.expandOptions(args);
  callback();
});

Given(/^the contract extra arguments "(.*?)"$/, function (args, callback) {
  this.contractArgs = this.expandOptions(args);
  callback();
});

Given(/^the partition extra arguments "(.*?)"$/, function (args, callback) {
  this.partitionArgs = this.expandOptions(args);
  callback();
});

Given(/^the customize extra arguments "(.*?)"$/, function (args, callback) {
  this.customizeArgs = this.expandOptions(args);
  callback();
});

Given(/^the data load extra arguments "(.*?)"$/, function (args, callback) {
  this.loaderArgs = this.expandOptions(args);
  callback();
});

Given(/^a grid size of ([0-9.]+) meters$/, function (meters, callback) {
  this.setGridSize(meters);
  callback();
});

Given(
  /^the origin ([-+]?[0-9]*\.?[0-9]+),([-+]?[0-9]*\.?[0-9]+)$/,
  function (lon, lat, callback) {
    this.setOrigin([parseFloat(lon), parseFloat(lat)]);
    callback();
  },
);

Given(/^the shortcuts$/, function (table, callback) {
  const q = d3.queue();

  const addShortcut = (row, cb) => {
    this.shortcutsHash[row.key] = row.value;
    cb();
  };

  table.hashes().forEach((row) => {
    q.defer(addShortcut, row);
  });

  q.awaitAll(callback);
});

Given(/^the node map$/, function (docstring, callback) {
  const q = d3.queue();

  const addNode = (name, ri, ci, cb) => {
    const lonLat = this.tableCoordToLonLat(ci, ri);
    if (name.match(/[a-z]/)) {
      if (this.nameNodeHash[name])
        throw new Error(util.format('*** duplicate node %s', name));
      this.addOSMNode(name, lonLat[0], lonLat[1], null);
    } else if (name.match(/[0-9]/)) {
      if (this.locationHash[name])
        throw new Error(util.format('*** duplicate node %s', name));
      this.addLocation(name, lonLat[0], lonLat[1], null);
    }
    cb();
  };

  docstring.split(/\n/).forEach((row, ri) => {
    row.split('').forEach((cell, ci) => {
      if (cell.match(/[a-z0-9]/)) {
        q.defer(addNode, cell, ri, ci * 0.5);
      }
    });
  });

  q.awaitAll(callback);
});

Given(/^the node locations$/, function (table, callback) {
  const q = d3.queue();

  const addNodeLocations = (row, cb) => {
    const name = row.node;
    if (this.findNodeByName(name))
      throw new Error(util.format('*** duplicate node %s', name));

    if (name.match(/[a-z]/)) {
      const id = row.id && parseInt(row.id);
      this.addOSMNode(name, row.lon, row.lat, id);
    } else {
      this.addLocation(name, row.lon, row.lat);
    }

    cb();
  };

  table.hashes().forEach((row) => q.defer(addNodeLocations, row));

  q.awaitAll(callback);
});

Given(/^the nodes$/, function (table, callback) {
  const q = d3.queue();

  const addNode = (row, cb) => {
    const name = row.node,
      node = this.findNodeByName(name);
    delete row.node;
    if (!node) throw new Error(util.format('*** unknown node %s', name));
    for (const key in row) {
      if (key == 'id') {
        node.setID(row[key]);
      } else {
        node.addTag(key, row[key]);
      }
    }
    cb();
  };

  table.hashes().forEach((row) => q.defer(addNode, row));

  q.awaitAll(callback);
});

Given(
  /^the ways( with locations)?$/,
  function (add_locations, table, callback) {
    if (this.osm_str)
      throw new Error(
        '*** Map data already defined - did you pass an input file in this scenario?',
      );

    const q = d3.queue();

    const addWay = (row, cb) => {
      const way = new OSM.Way(
        this.makeOSMId(),
        this.OSM_USER,
        this.OSM_TIMESTAMP,
        this.OSM_UID,
        !!add_locations,
      );

      const nodes = row.nodes;
      if (this.nameWayHash.nodes)
        throw new Error(util.format('*** duplicate way %s', nodes));

      for (let i = 0; i < nodes.length; i++) {
        const c = nodes[i];
        if (!c.match(/[a-z]/))
          throw new Error(
            util.format('*** ways can only use names a-z (%s)', c),
          );
        const node = this.findNodeByName(c);
        if (!node) throw new Error(util.format('*** unknown node %s', c));
        way.addNode(node);
      }

      const tags = {
        highway: 'primary',
      };

      for (const key in row) {
        tags[key] = row[key];
      }

      delete tags.nodes;

      if (row.highway === '(nil)') delete tags.highway;

      // Handle various ways to specify way names in test tables:
      // - undefined: use node sequence as name
      // - empty quotes: set empty name
      // - '(nil)' or empty: delete name tag entirely
      // - otherwise: use literal value
      if (row.name === undefined) tags.name = nodes;
      else if (row.name === '""' || row.name === '\'\'') tags.name = '';
      else if (row.name === '' || row.name === '(nil)') delete tags.name;
      else tags.name = row.name;

      way.setTags(tags);
      this.OSMDB.addWay(way);
      this.nameWayHash[nodes] = way;
      cb();
    };

    table.hashes().forEach((row) => q.defer(addWay, row));

    q.awaitAll(callback);
  },
);

Given(/^the relations$/, function (table, callback) {
  if (this.osm_str)
    throw new Error(
      '*** Map data already defined - did you pass an input file in this scenario?',
    );

  const q = d3.queue();

  const addRelation = (headers, row, cb) => {
    const relation = new OSM.Relation(
      this.makeOSMId(),
      this.OSM_USER,
      this.OSM_TIMESTAMP,
      this.OSM_UID,
    );

    let name = null;
    for (const index in row) {
      const key = headers[index];
      const value = row[index];
      // Parse relation member column headers:
      // - "node" or "node:role" for node members
      // - "way" or "way:role" for way members
      // - "relation" or "relation:role" for relation members
      // - "tag:value" for regular OSM tags
      const isNode = key.match(/^node:?(.*)/),
        isWay = key.match(/^way:?(.*)/),
        isRelation = key.match(/^relation:?(.*)/),
        isColonSeparated = key.match(/^(.*):(.*)/);
      if (isNode) {
        value
          .split(',')
          .map((v) => {
            return v.trim();
          })
          .forEach((nodeName) => {
            if (nodeName.length !== 1)
              throw new Error(
                util.format('*** invalid relation node member "%s"', nodeName),
              );
            const node = this.findNodeByName(nodeName);
            if (!node)
              throw new Error(
                util.format('*** unknown relation node member "%s"', nodeName),
              );
            relation.addMember('node', node.id, isNode[1]);
          });
      } else if (isWay) {
        value
          .split(',')
          .map((v) => {
            return v.trim();
          })
          .forEach((wayName) => {
            const way = this.findWayByName(wayName);
            if (!way)
              throw new Error(
                util.format('*** unknown relation way member "%s"', wayName),
              );
            relation.addMember('way', way.id, isWay[1]);
          });
      } else if (isRelation) {
        value
          .split(',')
          .map((v) => {
            return v.trim();
          })
          .forEach((relName) => {
            const otherrelation = this.findRelationByName(relName);
            if (!otherrelation)
              throw new Error(
                util.format(
                  '*** unknown relation relation member "%s"',
                  relName,
                ),
              );
            relation.addMember('relation', otherrelation.id, isRelation[1]);
          });
      } else if (isColonSeparated && isColonSeparated[1] !== 'restriction') {
        throw new Error(
          util.format(
            '*** unknown relation member type "%s:%s", must be either "node" or "way"',
            isColonSeparated[1],
            isColonSeparated[2],
          ),
        );
      } else {
        relation.addTag(key, value);
        if (key.match(/name/)) name = value;
      }
    }
    relation.uid = this.OSM_UID;

    if (name) {
      this.nameRelationHash[name] = relation;
    }

    this.OSMDB.addRelation(relation);

    cb();
  };

  const headers = table.raw()[0];
  table.rows().forEach((row) => q.defer(addRelation, headers, row));

  q.awaitAll(callback);
});

Given(/^the input file ([^"]*)$/, (file, callback) => {
  if (path.extname(file) !== '.osm')
    throw new Error('*** Input file must be in .osm format');
  fs.readFile(file, 'utf8', function (err, data) {
    if (!err) this.osm_str = data.toString();
    callback(err);
  });
});

Given(/^the raster source$/, function (data, callback) {
  // TODO: Don't overwrite if it exists
  fs.writeFile(this.rasterCacheFile, data, callback);
  // we need this to pass it to the profiles
  this.environment = Object.assign(
    { OSRM_RASTER_SOURCE: this.rasterCacheFile },
    this.environment,
  );
});

Given(/^the speed file$/, function (data, callback) {
  // TODO: Don't overwrite if it exists
  fs.writeFile(this.speedsCacheFile, data, callback);
});

Given(/^the turn penalty file$/, function (data, callback) {
  // TODO: Don't overwrite if it exists
  fs.writeFile(this.penaltiesCacheFile, data, callback);
});

Given(
  /^the profile file(?: "([^"]*)" initialized with)?$/,
  function (profile, data, callback) {
    const lua_profiles_path = this.PROFILES_PATH.split(path.sep).join('/');
    let text = `package.path = "${lua_profiles_path}/?.lua;" .. package.path\n`;
    if (profile == null) {
      text += `${data}\n`;
    } else {
      text += `local functions = require("${profile}")\n`;
      text += 'functions.setup_parent = functions.setup\n';
      text += 'functions.setup = function()\n';
      text += 'local profile = functions.setup_parent()\n';
      text += `${data}\n`;
      text += 'return profile\n';
      text += 'end\n';
      text += 'return functions\n';
    }
    this.profileFile = this.profileCacheFile;
    // TODO: Don't overwrite if it exists
    fs.writeFile(this.profileCacheFile, text, callback);
  },
);

Given(/^the data has been saved to disk$/, function (callback) {
  this.writeAndLinkOSM(callback);
});

Given(
  /^the data has been (extract|contract|partition|customiz)ed$/,
  function (step, callback) {
    this.reprocess(callback);
  },
);

Given(/^osrm-routed is stopped$/, function (callback) {
  this.OSRMLoader.shutdown(callback);
});

Given(/^data is loaded directly/, function (callback) {
  this.osrmLoader.setLoadMethod('directly');
  callback();
});

Given(/^data is loaded with datastore$/, function (callback) {
  this.osrmLoader.setLoadMethod('datastore');
  callback();
});

Given(/^the HTTP method "([^"]*)"$/, function (method, callback) {
  this.httpMethod = method;
  callback();
});
