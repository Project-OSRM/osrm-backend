// OSM to Cucumber converter - transforms OpenStreetMap XML data into Cucumber test scenario format
/*********************************
 * Takes an XML `.osm` file and converts it into a cucumber scenario definition like
 *    Given the node locations
 *       | node | lon | lat |
 *       .....
 *    Given the ways
 *       | nodes | tag1 | tag2 | tag3 |
 *       .....
 *
 * Note that cucumber tests are limited to 26 nodes (labelled a-z), so
 * you'll need to use pretty small OSM extracts to get this to work.
 *****************************************/
import fs from 'fs';
import { XMLParser } from 'fast-xml-parser';

const data = fs.readFileSync('filename.osm', 'utf8');

const parser = new XMLParser({
  ignoreAttributes: false,
  attributeNamePrefix: '@_',
  isArray: (name) => ['node', 'way', 'tag', 'nd'].includes(name)
});

const result = parser.parse(data);
const idmap = {};

console.log('Given the node locations');
console.log('    | node | lon       | lat       |');
result.osm.node.filter(n => !n['@_action'] || n['@_action'] !== 'delete').forEach(i => {
  const code = String.fromCharCode(97 + Object.keys(idmap).length);
  idmap[i['@_id']] = code;
  console.log(`    | ${code} | ${i['@_lon']} | ${i['@_lat']} |`);
});

const allkeys = {};
const waytags = {};

result.osm.way.filter(n => !n['@_action'] || n['@_action'] !== 'delete').forEach(w => {
  if (!waytags[w['@_id']]) waytags[w['@_id']] = {};
  w.tag.forEach(t => { allkeys[t['@_k']] = t['@_v']; waytags[w['@_id']][t['@_k']] = t['@_v']; });
});

console.log('And the ways');
console.log(`    | nodes |  ${Object.keys(allkeys).join(' | ')} |`);

result.osm.way.filter(n => !n['@_action'] || n['@_action'] !== 'delete').forEach(w => {
  console.log(`    | ${w.nd.map(n => idmap[n['@_ref']]).join('')} | ${Object.keys(allkeys).map(k => waytags[w['@_id']][k] || '').join(' | ')} |`);
});