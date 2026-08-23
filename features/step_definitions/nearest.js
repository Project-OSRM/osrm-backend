// Step definitions for testing nearest point snapping API
import util from 'util';

import flatbuffers from 'flatbuffers';
import { osrm } from '../support/fbresult_generated.js';
const FBResult = osrm.engine.api.fbresult.FBResult;   
import assert from 'node:assert';
import { When } from '@cucumber/cucumber';

When(/^I request nearest I should get$/, async function (table) {
  await this.reprocessAndLoadData();
  const testRow = function (row, _ri) {
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

            if (json.waypoints && json.waypoints.length) {
              if (headers.has('result_count')) {
                got.result_count = json.waypoints.length.toString();
              }

              if (headers.has('name')) {
                got.name = json.waypoints[0].name || '';
              }

              if (headers.has('snapping_distance')) {
                const dist = json.waypoints[0].distance;
                got.snapping_distance = dist != null ? dist.toFixed(1) : '';
              }

              if (row.out) {
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
  const testRow = function (row, _ri) {
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

When(/^I request nearest with flatbuffers for multiple coordinates I should get$/, async function (table) {
  await this.reprocessAndLoadData();
  const rows = table.hashes();

  const nodes = rows.map((row) => {
    const node = this.findNodeByName(row.in);
    if (!node) throw new Error(util.format('*** unknown in-node "%s"', row.in));
    return node;
  });

  this.queryParams.output = 'flatbuffers';

  const { res: response, body } = await new Promise((resolve, reject) => {
    this.requestNearestBatch(nodes, this.queryParams, (err, response, body) => {
      if (err) return reject(err);
      resolve({ res: response, body });
    });
  });

  assert.strictEqual(response.statusCode, 200, `expected 200, got ${response.statusCode}`);
  assert.ok(body.length, 'expected a non-empty flatbuffers body');

  const bytes = new Uint8Array(body.length);
  for (let indx = 0; indx < body.length; ++indx) {
    bytes[indx] = body.charCodeAt(indx);
  }
  const buf = new flatbuffers.ByteBuffer(bytes);
  const fb = FBResult.getRootAsFBResult(buf);

  assert.ok(!fb.error(), 'expected a successful (non-error) response');

  const groupsLength = fb.waypointsGroupedLength();
  assert.strictEqual(
    groupsLength,
    rows.length,
    `expected ${rows.length} waypoint groups, got ${groupsLength}`,
  );

  rows.forEach((row, i) => {
    const group = fb.waypointsGrouped(i);

    if (row.out === 'NoSegment') {
      assert.strictEqual(group.matched(), false, `coordinate ${i} (${row.in}): expected unmatched`);
      assert.ok(group.error(), `coordinate ${i} (${row.in}): expected an error object`);
      assert.strictEqual(group.error().code(), 'NoSegment');
      return;
    }

    assert.strictEqual(group.matched(), true, `coordinate ${i} (${row.in}): expected matched`);
    assert.ok(group.waypointsLength() > 0, `coordinate ${i} (${row.in}): expected at least one match`);

    const outNode = this.findNodeByName(row.out);
    if (!outNode) throw new Error(util.format('*** unknown out-node "%s"', row.out));

    const location = group.waypoints(0).location();
    const coord = [location.longitude(), location.latitude()];
    assert.ok(
      this.FuzzyMatch.matchLocation(coord, outNode),
      util.format('coordinate %d (%s): expected near %s, got [%d,%d]', i, row.in, row.out, coord[0], coord[1]),
    );
  });
});
When(/^I request nearest for multiple coordinates I should get$/, async function (table) {
  await this.reprocessAndLoadData();
  const rows = table.hashes();

  const nodes = rows.map((row) => {
    const node = this.findNodeByName(row.in);
    if (!node) throw new Error(util.format('*** unknown in-node "%s"', row.in));
    return node;
  });

  const params = Object.assign({}, this.queryParams);
  if (rows.some((row) => row.radius)) {
    params.radiuses = rows.map((row) => (row.radius ? row.radius : '')).join(';');
  }

  const { res: response, body } = await new Promise((resolve, reject) => {
    this.requestNearestBatch(nodes, params, (err, response, body) => {
      if (err) return reject(err);
      resolve({ res: response, body });
    });
  });

  assert.strictEqual(response.statusCode, 200, `expected 200, got ${response.statusCode}: ${body}`);
  const json = JSON.parse(body);
  assert.strictEqual(json.code, 'Ok', `expected code Ok, got ${json.code}: ${body}`);
  assert.strictEqual(
    json.waypoints.length,
    rows.length,
    `expected ${rows.length} waypoint groups, got ${json.waypoints.length}`,
  );

  rows.forEach((row, i) => {
    const group = json.waypoints[i];

    if (row.out === 'NoSegment') {
      assert.ok(
        !Array.isArray(group) && group.code === 'NoSegment',
        `coordinate ${i} (${row.in}): expected NoSegment, got ${JSON.stringify(group)}`,
      );
      return;
    }

    assert.ok(
      Array.isArray(group) && group.length,
      `coordinate ${i} (${row.in}): expected matches, got ${JSON.stringify(group)}`,
    );

    const outNode = this.findNodeByName(row.out);
    if (!outNode) throw new Error(util.format('*** unknown out-node "%s"', row.out));

    const coord = group[0].location;
    assert.ok(
      this.FuzzyMatch.matchLocation(coord, outNode),
      util.format('coordinate %d (%s): expected near %s, got [%d,%d]', i, row.in, row.out, coord[0], coord[1]),
    );
  });
});
