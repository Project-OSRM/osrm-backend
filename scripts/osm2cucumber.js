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
var fs = require('fs');
var parseString = require('xml2js').parseString;

var data = fs.readFileSync('filename.osm', 'utf8');

const items = parseString(data, (err, result) => {
var idmap = {};

console.log('Given the node locations');
console.log('    | node | lon       | lat       |');
result.osm.node.filter(n => !n.$.action || n.$.action !== 'delete').forEach(i => {
  var code = String.fromCharCode(97 + Object.keys(idmap).length)
  idmap[i.$.id] = code;
  console.log(`    | ${code} | ${i.$.lon} | ${i.$.lat} |`);
});

var allkeys = {};
var waytags = {};

result.osm.way.filter(n => !n.$.action || n.$.action !== 'delete').forEach(w => {
  if (!waytags[w.$.id]) waytags[w.$.id] = {};
  w.tag.forEach(t => { allkeys[t.$.k] = t.$.v; waytags[w.$.id][t.$.k] = t.$.v; });
});

console.log('And the ways');
console.log(`    | nodes |  ${Object.keys(allkeys).join(' | ')} |`);

result.osm.way.filter(n => !n.$.action || n.$.action !== 'delete').forEach(w => {
  console.log(`    | ${w.nd.map(n => idmap[n.$.ref]).join('')} | ${Object.keys(allkeys).map(k => waytags[w.$.id][k] || '').join(' | ')} |`);
});
});
