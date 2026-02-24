// Test nearest service functionality for finding closest waypoints on road network
import OSRM from '../../build/nodejs/lib/index.js';
import test from 'tape';
import { data_path, mld_data_path, three_test_coordinates, two_test_coordinates } from './constants.js';
import flatbuffers from 'flatbuffers';
import { osrm } from '../../features/support/fbresult_generated.js';

const FBResult = osrm.engine.api.fbresult.FBResult;


test('nearest with flatbuffers format', (assert) => {
  assert.plan(5);
  const osrm = new OSRM(data_path);
  osrm.nearest({
    coordinates: [three_test_coordinates[0]],
    format: 'flatbuffers'
  }, (err, result) => {
    assert.ifError(err);
    assert.ok(result instanceof Buffer);
    const fb = FBResult.getRootAsFBResult(new flatbuffers.ByteBuffer(result));
    assert.equals(fb.waypointsLength(), 1);
    assert.ok(fb.waypoints(0).location());
    assert.ok(fb.waypoints(0).name());
  });
});

test('nearest', (assert) => {
  assert.plan(4);
  const osrm = new OSRM(data_path);
  osrm.nearest({
    coordinates: [three_test_coordinates[0]]
  }, (err, result) => {
    assert.ifError(err);
    assert.equal(result.waypoints.length, 1);
    assert.equal(result.waypoints[0].location.length, 2);
    assert.ok(result.waypoints[0].hasOwnProperty('name'));
  });
});

test('nearest', (assert) => {
  assert.plan(5);
  const osrm = new OSRM(data_path);
  osrm.nearest({
    coordinates: [three_test_coordinates[0]]
  }, { format: 'json_buffer' }, (err, result) => {
    assert.ifError(err);
    assert.ok(result instanceof Buffer);
    result = JSON.parse(result);
    assert.equal(result.waypoints.length, 1);
    assert.equal(result.waypoints[0].location.length, 2);
    assert.ok(result.waypoints[0].hasOwnProperty('name'));
  });
});

test('nearest: can ask for multiple nearest pts', (assert) => {
  assert.plan(2);
  const osrm = new OSRM(data_path);
  osrm.nearest({
    coordinates: [three_test_coordinates[0]],
    number: 3
  }, (err, result) => {
    assert.ifError(err);
    assert.equal(result.waypoints.length, 3);
  });
});

test('nearest: throws on invalid args', (assert) => {
  assert.plan(7);
  const osrm = new OSRM(data_path);
  const options = {};
  assert.throws(() => { osrm.nearest(options); },
    /Two arguments required/);
  assert.throws(() => { osrm.nearest(null, (err, res) => {}); },
    /First arg must be an object/);
  options.coordinates = [43.73072];
  assert.throws(() => { osrm.nearest(options, (err, res) => {}); },
    /Coordinates must be an array of /);
  options.coordinates = [three_test_coordinates[0], three_test_coordinates[1]];
  assert.throws(() => { osrm.nearest(options, (err, res) => {}); },
    /Exactly one coordinate pair must be provided/);
  options.coordinates = [three_test_coordinates[0]];
  options.number = 3.14159;
  assert.throws(() => { osrm.nearest(options, (err, res) => {}); },
    /Number must be an integer greater than or equal to 1/);
  options.number = 0;
  assert.throws(() => { osrm.nearest(options, (err, res) => {}); },
    /Number must be an integer greater than or equal to 1/);

  options.number = 1;
  assert.throws(() => { osrm.nearest(options, { format: 'invalid' }, (err, res) => {}); },
    /format must be a string:/);
});

test('nearest: nearest in Monaco without motorways', (assert) => {
  assert.plan(2);
  const osrm = new OSRM({path: mld_data_path, algorithm: 'MLD'});
  const options = {
    coordinates: [two_test_coordinates[0]],
    exclude: ['motorway']
  };
  osrm.nearest(options, (err, response) => {
    assert.ifError(err);
    assert.equal(response.waypoints.length, 1);
  });
});

test('nearest: throws on disabled geometry', (assert) => {
  assert.plan(1);
  const osrm = new OSRM({path: data_path, 'disable_feature_dataset': ['ROUTE_GEOMETRY']});
  const options = {
    coordinates: [two_test_coordinates[0]],
  };
  osrm.nearest(options, (err, response) => {
    console.log(err);
    assert.match(err.message, /DisabledDatasetException/);
  });
});
test('nearest: ok on disabled geometry', (assert) => {
  assert.plan(2);
  const osrm = new OSRM({path: data_path, 'disable_feature_dataset': ['ROUTE_GEOMETRY']});
  const options = {
    coordinates: [two_test_coordinates[0]],
    skip_waypoints: true,
  };
  osrm.nearest(options, (err, response) => {
    assert.ifError(err);
    assert.notok(response.waypoints);

  });
});

test('nearest: ok on disabled steps', (assert) => {
  assert.plan(2);
  const osrm = new OSRM({path: data_path, 'disable_feature_dataset': ['ROUTE_STEPS']});
  const options = {
    coordinates: [two_test_coordinates[0]],
  };
  osrm.nearest(options, (err, response) => {
    assert.ifError(err);
    assert.equal(response.waypoints.length, 1);
  });
});
