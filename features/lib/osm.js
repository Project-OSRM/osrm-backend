// OpenStreetMap data structures and XML generation utilities for synthetic test data
import builder from 'xmlbuilder';
import { ensureDecimal } from './utils.js';

// OpenStreetMap database for storing nodes, ways, and relations
class DB {
  constructor() {
    this.nodes = new Array();
    this.ways = new Array();
    this.relations = new Array();
  }

  // Adds OSM node to database
  addNode(node) {
    this.nodes.push(node);
  }

  // Adds OSM way to database
  addWay(way) {
    this.ways.push(way);
  }

  addRelation(relation) {
    this.relations.push(relation);
  }

  clear() {
    this.nodes = [];
    this.ways = [];
    this.relations = [];
  }

  // Converts database to OSM XML format for OSRM processing
  toXML(callback) {
    const xml = builder.create('osm', { encoding: 'UTF-8' });
    xml.att('generator', 'osrm-test').att('version', '0.6');

    this.nodes.forEach((n) => {
      const node = xml.ele('node', {
        id: n.id,
        version: 1,
        uid: n.OSM_UID,
        user: n.OSM_USER,
        timestamp: n.OSM_TIMESTAMP,
        lon: ensureDecimal(n.lon),
        lat: ensureDecimal(n.lat),
      });

      for (const k in n.tags) {
        node.ele('tag').att('k', k).att('v', n.tags[k]);
      }
    });

    this.ways.forEach((w) => {
      const way = xml.ele('way', {
        id: w.id,
        version: 1,
        uid: w.OSM_UID,
        user: w.OSM_USER,
        timestamp: w.OSM_TIMESTAMP,
      });

      w.nodes.forEach((k) => {
        const nd = way.ele('nd').att('ref', k.id);
        if (w.add_locations) {
          nd.att('lon', k.lon);
          nd.att('lat', k.lat);
        }
      });

      for (const k in w.tags) {
        way.ele('tag').att('k', k).att('v', w.tags[k]);
      }
    });

    this.relations.forEach((r) => {
      const relation = xml.ele('relation', {
        id: r.id,
        user: r.OSM_USER,
        timestamp: r.OSM_TIMESTAMP,
        uid: r.OSM_UID,
      });

      r.members.forEach((m) => {
        const d = {
          type: m.type,
          ref: m.id,
        };
        if (m.role) d.role = m.role;
        relation.ele('member', d);
      });

      for (const k in r.tags) {
        relation.ele('tag').att('k', k).att('v', r.tags[k]);
      }
    });

    callback(xml.end({ pretty: true, indent: '  ' }));
  }
}

class Node {
  constructor(id, OSM_USER, OSM_TIMESTAMP, OSM_UID, lon, lat, tags) {
    this.id = id;
    this.OSM_USER = OSM_USER;
    this.OSM_TIMESTAMP = OSM_TIMESTAMP;
    this.OSM_UID = OSM_UID;
    this.lon = lon;
    this.lat = lat;
    this.tags = tags;
  }

  addTag(k, v) {
    this.tags[k] = v;
  }

  setID(id) {
    this.id = id;
  }
}

class Way {
  constructor(id, OSM_USER, OSM_TIMESTAMP, OSM_UID, add_locations) {
    this.id = id;
    this.OSM_USER = OSM_USER;
    this.OSM_TIMESTAMP = OSM_TIMESTAMP;
    this.OSM_UID = OSM_UID;
    this.tags = {};
    this.nodes = [];
    this.add_locations = add_locations;
  }

  addNode(node) {
    this.nodes.push(node);
  }

  setTags(tags) {
    this.tags = tags;
  }
}

class Relation {
  constructor(id, OSM_USER, OSM_TIMESTAMP, OSM_UID) {
    this.id = id;
    this.OSM_USER = OSM_USER;
    this.OSM_TIMESTAMP = OSM_TIMESTAMP;
    this.OSM_UID = OSM_UID;
    this.members = [];
    this.tags = {};
  }

  addMember(memberType, id, role) {
    this.members.push({ type: memberType, id, role });
  }

  addTag(k, v) {
    this.tags[k] = v;
  }
}

export { DB, Node, Way, Relation };
