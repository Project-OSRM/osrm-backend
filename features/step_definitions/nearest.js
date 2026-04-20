// Step definitions for testing nearest point snapping API
import util from 'util';

import flatbuffers from 'flatbuffers';
import { osrm } from '../support/fbresult_generated.js';
const FBResult = osrm.engine.api.fbresult.FBResult;
import { When } from '@cucumber/cucumber';

When(/^I request nearest I should get$/, async function (table) {
  await this.reprocessAndLoadData();
  const testRow = function (row, ri) {
    return new Promise((resolve, reject) => {
      const inNode = this.findNodeByName(row.in);
      if (!inNode) return reject(new Error(util.format('*** unknown in-node "%s"', row.in)));

      this.requestNearest(inNode, this.queryParams, (err, response, body) => {
        if (err) return reject(err);
        let coord;
        const headers = new Set(table.raw()[0]);

        const got = { in: row.in };

        if (body.length) {
          const json = JSON.parse(body);
          got.code = json.code;

          if (response.statusCode === 200) {

            if (headers.has('data_version')) {
              got.data_version = json.data_version || '';
            }

            if (json.waypoints && json.waypoints.length && row.out) {
              coord = json.waypoints[0].location;

              got.out = row.out;

              const outNode = this.findNodeByName(row.out);
              if (!outNode) return reject(new Error(util.format('*** unknown out-node "%s"', row.out)));

              Object.keys(row).forEach((key) => {
                if (key === 'out') {
                  if (this.FuzzyMatch.matchLocation(coord, outNode)) {
                    got[key] = row[key];
                  } else {
                    row[key] = util.format('%s [%d,%d]', row[key], outNode.lat, outNode.lon);
                  }
                } else if (key === 'nodes') {
                  const nodeNames = row.nodes.split(',').map(n => n.trim()).filter(n => n.length > 0);
                  if (nodeNames.length !== 2)
                    throw new Error(util.format('*** nodes column must be "from,to", got "%s"', row.nodes));
                  const fromNode = this.findNodeByName(nodeNames[0]);
                  const toNode = this.findNodeByName(nodeNames[1]);
                  if (!fromNode) throw new Error(util.format('*** unknown from-node "%s"', nodeNames[0]));
                  if (!toNode) throw new Error(util.format('*** unknown to-node "%s"', nodeNames[1]));
                  const actualNodes = json.waypoints[0].nodes;
                  if (actualNodes && actualNodes[0] === fromNode.id && actualNodes[1] === toNode.id) {
                    got.nodes = row.nodes;
                  } else {
                    row.nodes = util.format('%s [got: %s,%s]', row.nodes,
                      actualNodes ? actualNodes[0] : '?', actualNodes ? actualNodes[1] : '?');
                  }
                }
              });
            }

          }
          resolve(got);
        }
        else {
          resolve();
        }
      });
    });
  }.bind(this);

  await this.processRowsAndDiff(table, testRow);
});

When(/^I request nearest with flatbuffers I should get$/, async function (table) {
  await this.reprocessAndLoadData();
  const testRow = function (row, ri) {
    return new Promise((resolve, reject) => {
      const inNode = this.findNodeByName(row.in);
      if (!inNode) return reject(new Error(util.format('*** unknown in-node "%s"', row.in)));

      const outNode = this.findNodeByName(row.out);
      if (!outNode) return reject(new Error(util.format('*** unknown out-node "%s"', row.out)));

      this.queryParams.output = 'flatbuffers';
      this.requestNearest(inNode, this.queryParams, (err, response, body) => {
        if (err) return reject(err);
        let coord;

        if (response.statusCode === 200 && body.length) {
          const bytes = new Uint8Array(body.length);
          for (let indx = 0; indx < body.length; ++indx) {
            bytes[indx] = body.charCodeAt(indx);
          }
          const buf = new flatbuffers.ByteBuffer(bytes);
          const fb = FBResult.getRootAsFBResult(buf);
          const location = fb.waypoints(0).location();

          coord = [location.longitude(), location.latitude()];

          const got = { in: row.in, out: row.out };

          Object.keys(row).forEach((key) => {
            if (key === 'out') {
              if (this.FuzzyMatch.matchLocation(coord, outNode)) {
                got[key] = row[key];
              } else {
                row[key] = util.format('%s [%d,%d]', row[key], outNode.lat, outNode.lon);
              }
            }
          });

          resolve(got);
        }
        else {
          resolve();
        }
      });
    });
  }.bind(this);

  await this.processRowsAndDiff(table, testRow);
});
