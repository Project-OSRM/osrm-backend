// Test OSRM constructor and basic functionality
import OSRM from '../../build/nodejs/lib/index.js';
import test from 'tape';
import { data_path as monaco_path, test_memory_path as test_memory_file, mld_data_path as monaco_mld_path } from './constants.js';
// Import all test modules
import './route.js';
import './trip.js';
import './match.js';
import './tile.js';
import './table.js';
import './nearest.js';

test('constructor: throws if new keyword is not used', (assert) => {
  assert.plan(1);
  assert.throws(() => { OSRM(); },
    /Class constructors cannot be invoked without 'new'/);
});

test('constructor: uses defaults with no parameter', (assert) => {
  assert.plan(1);
  const osrm = new OSRM();
  assert.ok(osrm);
});

test('constructor: does not accept more than one parameter', (assert) => {
  assert.plan(1);
  assert.throws(() => { new OSRM({}, {}); },
    /Only accepts one parameter/);
});

test('constructor: throws if necessary files do not exist', (assert) => {
  assert.plan(2);
  assert.throws(() => { new OSRM('missing.osrm'); },
    /Required files are missing, cannot continue/);

  assert.throws(() => { new OSRM({path: 'missing.osrm', algorithm: 'MLD'}); },
    /Required files are missing, cannot continue/);
});

test('constructor: takes a shared memory argument', (assert) => {
  assert.plan(1);
  const osrm = new OSRM({path: monaco_path, shared_memory: false});
  assert.ok(osrm);
});

test('constructor: takes a memory file', (assert) => {
  assert.plan(1);
  const osrm = new OSRM({path: monaco_path, memory_file: test_memory_file});
  assert.ok(osrm);
});

test('constructor: throws if shared_memory==false with no path defined', (assert) => {
  assert.plan(1);
  assert.throws(() => { new OSRM({shared_memory: false}); },
    /Shared_memory must be enabled if no path is specified/);
});

test('constructor: throws if given a non-bool shared_memory option', (assert) => {
  assert.plan(1);
  assert.throws(() => { new OSRM({path: monaco_path, shared_memory: 'a'}); },
    /Shared_memory option must be a boolean/);
});

test('constructor: throws if given a non-string/obj argument', (assert) => {
  assert.plan(1);
  assert.throws(() => { new OSRM(true); },
    /Parameter must be a path or options object/);
});

test('constructor: throws if given an unkown algorithm', (assert) => {
  assert.plan(1);
  assert.throws(() => { new OSRM({algorithm: 'Foo', shared_memory: true}); },
    /algorithm option must be one of 'CH', or 'MLD'/);
});

test('constructor: throws if given an invalid algorithm', (assert) => {
  assert.plan(1);
  assert.throws(() => { new OSRM({algorithm: 3, shared_memory: true}); },
    /algorithm option must be a string and one of 'CH', or 'MLD'/);
});

test('constructor: loads MLD if given as algorithm', (assert) => {
  assert.plan(1);
  const osrm = new OSRM({algorithm: 'MLD', path: monaco_mld_path});
  assert.ok(osrm);
});

test('constructor: loads CH if given as algorithm', (assert) => {
  assert.plan(1);
  const osrm = new OSRM({algorithm: 'CH', path: monaco_path});
  assert.ok(osrm);
});

test('constructor: throws if data doesn\'t match algorithm', (assert) => {
  assert.plan(1);
  assert.throws(() => { new OSRM({algorithm: 'MLD', path: monaco_path}); }, /Could not find any metrics for MLD/, 'MLD with CH data');
});

test('constructor: throws if dataset_name is not a string', (assert) => {
  assert.plan(3);
  assert.throws(() => { new OSRM({dataset_name: 1337, path: monaco_mld_path}); }, /dataset_name needs to be a string/, 'Does not accept int');
  assert.ok(new OSRM({dataset_name: '', shared_memory: true}), 'Does accept string');
  assert.throws(() => { new OSRM({dataset_name: 'unsued_name___', shared_memory: true}); }, /Could not find shared memory region/, 'Does not accept wrong name');
});

test('constructor: takes a default_radius argument', (assert) => {
  assert.plan(1);
  const osrm = new OSRM({algorithm: 'MLD', path: monaco_mld_path, default_radius: 1});
  assert.ok(osrm);
});

test('constructor: takes a default_radius unlimited argument', (assert) => {
  assert.plan(1);
  const osrm = new OSRM({algorithm: 'MLD', path: monaco_mld_path, default_radius: 'unlimited'});
  assert.ok(osrm);
});

test('constructor: throws if default_radius is not a number', (assert) => {
  assert.plan(3);
  assert.throws(() => { new OSRM({algorithm: 'MLD', path: monaco_mld_path, default_radius: 'abc'}); }, /default_radius must be unlimited or an integral number/, 'Does not accept invalid string');
  assert.ok(new OSRM({algorithm: 'MLD', path: monaco_mld_path, default_radius: 1}), 'Does accept number');
  assert.ok(new OSRM({algorithm: 'MLD', path: monaco_mld_path, default_radius: 'unlimited'}), 'Does accept unlimited');
});

test('constructor: parses custom limits', (assert) => {
  assert.plan(1);
  const osrm = new OSRM({
    path: monaco_mld_path,
    algorithm: 'MLD',
    max_locations_trip: 1,
    max_locations_viaroute: 1,
    max_locations_distance_table: 1,
    max_locations_map_matching: 1,
    max_results_nearest: 1,
    max_alternatives: 1,
    default_radius: 1
  });
  assert.ok(osrm);
});

test('constructor: throws on invalid custom limits', (assert) => {
  assert.plan(1);
  assert.throws(() => {
    const osrm = new OSRM({
      path: monaco_mld_path,
      algorithm: 'MLD',
      max_locations_trip: 'unlimited',
      max_locations_viaroute: true,
      max_locations_distance_table: false,
      max_locations_map_matching: 'a lot',
      max_results_nearest: null,
      max_alternatives: '10',
      default_radius: '10'
    });
  });
});
test('constructor: throws on invalid disable_feature_dataset option', (assert) => {
  assert.plan(1);
  assert.throws(() => {
    const osrm = new OSRM({
      path: monaco_path,
      disable_feature_dataset: ['NOT_EXIST'],
    });
  });
});

test('constructor: throws on non-array disable_feature_dataset', (assert) => {
  assert.plan(1);
  assert.throws(() => {
    const osrm = new OSRM({
      path: monaco_path,
      disable_feature_dataset: 'ROUTE_GEOMETRY',
    });
  });
});

test('constructor: ok on valid disable_feature_dataset option', (assert) => {
  assert.plan(1);
  const osrm = new OSRM({
    path: monaco_path,
    disable_feature_dataset: ['ROUTE_GEOMETRY'],
  });
  assert.ok(osrm);
});

test('constructor: ok on multiple overlapping disable_feature_dataset options', (assert) => {
  assert.plan(1);
  const osrm = new OSRM({
    path: monaco_path,
    disable_feature_dataset: ['ROUTE_GEOMETRY', 'ROUTE_STEPS'],
  });
  assert.ok(osrm);
});
