'use strict';

const builder = require('xmlbuilder');
const ensureDecimal = require('./utils').ensureDecimal;

class DB {
    constructor () {
        this.nodes = new Array();
        this.ways = new Array();
        this.relations = new Array();
    }

    addNode (node) {
        this.nodes.push(node);
    }

    addWay (way) {
        this.ways.push(way);
    }

    addRelation (relation) {
        this.relations.push(relation);
    }

    clear () {
        this.nodes = [];
        this.ways = [];
        this.relations = [];
    }

    toXML (callback) {
        var xml = builder.create('osm', {'encoding':'UTF-8'});
        xml.att('generator', 'osrm-test')
            .att('version', '0.6');

        this.nodes.forEach((n) => {
            var node = xml.ele('node', {
                id: n.id,
                version: 1,
                uid: n.OSM_UID,
                user: n.OSM_USER,
                timestamp: n.OSM_TIMESTAMP,
                lon: ensureDecimal(n.lon),
                lat: ensureDecimal(n.lat)
            });

            for (var k in n.tags) {
                node.ele('tag')
                    .att('k', k)
                    .att('v', n.tags[k]);
            }
        });

        this.ways.forEach((w) => {
            var way = xml.ele('way', {
                id: w.id,
                version: 1,
                uid: w.OSM_UID,
                user: w.OSM_USER,
                timestamp: w.OSM_TIMESTAMP
            });

            w.nodes.forEach((k) => {
                way.ele('nd')
                    .att('ref', k.id);
            });

            for (var k in w.tags) {
                way.ele('tag')
                    .att('k', k)
                    .att('v', w.tags[k]);
            }
        });

        this.relations.forEach((r) => {
            var relation = xml.ele('relation', {
                id: r.id,
                user: r.OSM_USER,
                timestamp: r.OSM_TIMESTAMP,
                uid: r.OSM_UID
            });

            r.members.forEach((m) => {
                relation.ele('member', {
                    type: m.type,
                    ref: m.id,
                    role: m.role
                });
            });

            for (var k in r.tags) {
                relation.ele('tag')
                    .att('k', k)
                    .att('v', r.tags[k]);
            }
        });

        callback(xml.end({ pretty: true, indent: '  ' }));
    }
}

class Node {
    constructor (id, OSM_USER, OSM_TIMESTAMP, OSM_UID, lon, lat, tags) {
        this.id = id;
        this.OSM_USER = OSM_USER;
        this.OSM_TIMESTAMP = OSM_TIMESTAMP;
        this.OSM_UID = OSM_UID;
        this.lon = lon;
        this.lat = lat;
        this.tags = tags;
    }

    addTag (k, v) {
        this.tags[k] = v;
    }

    setID (id ) {
        this.id = id;
    }
}

class Way {
    constructor (id, OSM_USER, OSM_TIMESTAMP, OSM_UID) {
        this.id = id;
        this.OSM_USER = OSM_USER;
        this.OSM_TIMESTAMP = OSM_TIMESTAMP;
        this.OSM_UID = OSM_UID;
        this.tags = {};
        this.nodes = [];
    }

    addNode (node) {
        this.nodes.push(node);
    }

    setTags (tags) {
        this.tags = tags;
    }
}

class Relation {
    constructor (id, OSM_USER, OSM_TIMESTAMP, OSM_UID) {
        this.id = id;
        this.OSM_USER = OSM_USER;
        this.OSM_TIMESTAMP = OSM_TIMESTAMP;
        this.OSM_UID = OSM_UID;
        this.members = [];
        this.tags = {};
    }

    addMember (memberType, id, role) {
        this.members.push({type: memberType, id: id, role: role});
    }

    addTag (k, v) {
        this.tags[k] = v;
    }
}

module.exports = {
    DB: DB,
    Node: Node,
    Way: Way,
    Relation: Relation
};
