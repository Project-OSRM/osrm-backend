// Test tile service functionality for vector tile generation
import OSRM from '../../build/nodejs/lib/index.js';
import test from 'tape';
import { data_path, test_tile as tile } from './constants.js';

test.test('tile check size coarse', (assert) => {
  assert.plan(2);
  const osrm = new OSRM(data_path);
  osrm.tile(tile.at, (err, result) => {
    assert.ifError(err);
    assert.equal(result.length, tile.size);
  });
});

test.test('tile interface pre-conditions', (assert) => {
  assert.plan(6);
  const osrm = new OSRM(data_path);

  assert.throws(() => { osrm.tile(null, (err, result) => {}); }, /must be an array \[x, y, z\]/);
  assert.throws(() => { osrm.tile([], (err, result) => {}); }, /must be an array \[x, y, z\]/);
  assert.throws(() => { osrm.tile([[]], (err, result) => {}); }, /must be an array \[x, y, z\]/);
  assert.throws(() => { osrm.tile(undefined, (err, result) => {}); }, /must be an array \[x, y, z\]/);
  assert.throws(() => { osrm.tile(17059, 11948, 15, (err, result) => {}); }, /must be an array \[x, y, z\]/);
  assert.throws(() => { osrm.tile([17059, 11948, -15], (err, result) => {}); }, /must be unsigned/);
});

test.test('tile fails to load with geometry disabled', (assert) => {
  assert.plan(1);
  const osrm = new OSRM({'path': data_path, 'disable_feature_dataset': ['ROUTE_GEOMETRY']});
  osrm.tile(tile.at, (err, result) => {
    console.log(err);
    assert.match(err.message, /DisabledDatasetException/);
  });
});
test.test('tile ok with steps disabled', (assert) => {
  assert.plan(2);
  const osrm = new OSRM({'path': data_path, 'disable_feature_dataset': ['ROUTE_STEPS']});
  osrm.tile(tile.at, (err, result) => {
    assert.ifError(err);
    assert.equal(result.length, tile.size);
  });
});
