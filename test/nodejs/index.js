var OSRM = require('../../');
var test = require('tape');
var monaco_path = require('./constants').data_path;
var test_memory_file = require('./constants').test_memory_file;
var monaco_mld_path = require('./constants').mld_data_path;
var monaco_corech_path = require('./constants').corech_data_path;

test('constructor: throws if new keyword is not used', function(assert) {
    assert.plan(1);
    assert.throws(function() { OSRM(); },
      /Cannot call constructor as function, you need to use 'new' keyword/);
});

test('constructor: uses defaults with no parameter', function(assert) {
    assert.plan(1);
    var osrm = new OSRM();
    assert.ok(osrm);
});

test('constructor: does not accept more than one parameter', function(assert) {
    assert.plan(1);
    assert.throws(function() { new OSRM({}, {}); },
        /Only accepts one parameter/);
});

test('constructor: throws if necessary files do not exist', function(assert) {
    assert.plan(2);
    assert.throws(function() { new OSRM('missing.osrm'); },
        /Required files are missing, cannot continue/);

    assert.throws(function() { new OSRM({path: 'missing.osrm', algorithm: 'MLD'}); },
        /Required files are missing, cannot continue/);
});

test('constructor: takes a shared memory argument', function(assert) {
    assert.plan(1);
    var osrm = new OSRM({path: monaco_path, shared_memory: false});
    assert.ok(osrm);
});

test('constructor: takes a memory file', function(assert) {
    assert.plan(1);
    var osrm = new OSRM({path: monaco_path, memory_file: test_memory_file});
    assert.ok(osrm);
});

test('constructor: throws if shared_memory==false with no path defined', function(assert) {
    assert.plan(1);
    assert.throws(function() { new OSRM({shared_memory: false}); },
        /Shared_memory must be enabled if no path is specified/);
});

test('constructor: throws if given a non-bool shared_memory option', function(assert) {
    assert.plan(1);
    assert.throws(function() { new OSRM({path: monaco_path, shared_memory: 'a'}); },
        /Shared_memory option must be a boolean/);
});

test('constructor: throws if given a non-string/obj argument', function(assert) {
    assert.plan(1);
    assert.throws(function() { new OSRM(true); },
        /Parameter must be a path or options object/);
});

test('constructor: throws if given an unkown algorithm', function(assert) {
    assert.plan(1);
    assert.throws(function() { new OSRM({algorithm: 'Foo', shared_memory: true}); },
        /algorithm option must be one of 'CH', 'CoreCH', or 'MLD'/);
});

test('constructor: throws if given an invalid algorithm', function(assert) {
    assert.plan(1);
    assert.throws(function() { new OSRM({algorithm: 3, shared_memory: true}); },
        /algorithm option must be a string and one of 'CH', 'CoreCH', or 'MLD'/);
});

test('constructor: loads MLD if given as algorithm', function(assert) {
    assert.plan(1);
    var osrm = new OSRM({algorithm: 'MLD', path: monaco_mld_path});
    assert.ok(osrm);
});

test('constructor: loads CH if given as algorithm', function(assert) {
    assert.plan(1);
    var osrm = new OSRM({algorithm: 'CH', path: monaco_path});
    assert.ok(osrm);
});

test('constructor: loads CoreCH if given as algorithm', function(assert) {
    assert.plan(1);
    var osrm = new OSRM({algorithm: 'CoreCH', path: monaco_corech_path});
    assert.ok(osrm);
});

test('constructor: autoswitches to CoreCH for a CH dataset if capable', function(assert) {
    assert.plan(1);
    var osrm = new OSRM({algorithm: 'CH', path: monaco_corech_path});
    assert.ok(osrm);
});

test('constructor: throws if data doesn\'t match algorithm', function(assert) {
    assert.plan(3);
    assert.throws(function() { new OSRM({algorithm: 'CoreCH', path: monaco_mld_path}); }, /Could not find any metrics for CH/, 'CoreCH with MLD data');
    assert.ok(new OSRM({algorithm: 'CoreCH', path: monaco_path}), 'CoreCH with CH data');
    assert.throws(function() { new OSRM({algorithm: 'MLD', path: monaco_path}); }, /Could not find any metrics for MLD/, 'MLD with CH data');
});

test('constructor: throws if dataset_name is not a string', function(assert) {
    assert.plan(3);
    assert.throws(function() { new OSRM({dataset_name: 1337, path: monaco_mld_path}); }, /dataset_name needs to be a string/, 'Does not accept int');
    assert.ok(new OSRM({dataset_name: "", shared_memory: true}), 'Does accept string');
    assert.throws(function() { new OSRM({dataset_name: "unsued_name___", shared_memory: true}); }, /Could not find shared memory region/, 'Does not accept wrong name');
});

test('constructor: parses custom limits', function(assert) {
    assert.plan(1);
    var osrm = new OSRM({
        path: monaco_mld_path,
        algorithm: 'MLD',
        max_locations_trip: 1,
        max_locations_viaroute: 1,
        max_locations_distance_table: 1,
        max_locations_map_matching: 1,
        max_results_nearest: 1,
        max_alternatives: 1,
    });
    assert.ok(osrm);
});

test('constructor: throws on invalid custom limits', function(assert) {
    assert.plan(1);
    assert.throws(function() {
        var osrm = new OSRM({
            path: monaco_mld_path,
            algorithm: 'MLD',
            max_locations_trip: 'unlimited',
            max_locations_viaroute: true,
            max_locations_distance_table: false,
            max_locations_map_matching: 'a lot',
            max_results_nearest: null,
            max_alternatives: '10'
        })
    });
});

require('./route.js');
require('./trip.js');
require('./match.js');
require('./tile.js');
require('./table.js');
require('./nearest.js');
