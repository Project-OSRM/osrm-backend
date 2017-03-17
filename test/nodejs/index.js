var OSRM = require('../../');
var test = require('tape');
var berlin_path = require('./osrm-data-path').data_path;
var berlin_mld_path = require('./osrm-data-path').mld_data_path;
var berlin_corech_path = require('./osrm-data-path').corech_data_path;

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
    assert.plan(1);
    assert.throws(function() { new OSRM("missing.osrm"); },
        /Error opening missing.osrm.names/);
});

test('constructor: takes a shared memory argument', function(assert) {
    assert.plan(1);
    var osrm = new OSRM({path: berlin_path, shared_memory: false});
    assert.ok(osrm);
});

test('constructor: throws if shared_memory==false with no path defined', function(assert) {
    assert.plan(1);
    assert.throws(function() { new OSRM({shared_memory: false}); },
        /Shared_memory must be enabled if no path is specified/);
});

test('constructor: throws if given a non-bool shared_memory option', function(assert) {
    assert.plan(1);
    assert.throws(function() { new OSRM({path: berlin_path, shared_memory: 'a'}); },
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
    var osrm = new OSRM({algorithm: 'MLD', path: berlin_mld_path});
    assert.ok(osrm);
});

test('constructor: loads CH if given as algorithm', function(assert) {
    assert.plan(1);
    var osrm = new OSRM({algorithm: 'CH', path: berlin_path});
    assert.ok(osrm);
});

test('constructor: loads CoreCH if given as algorithm', function(assert) {
    assert.plan(1);
    var osrm = new OSRM({algorithm: 'CoreCH', path: berlin_corech_path});
    assert.ok(osrm);
});

require('./route.js');
require('./trip.js');
require('./match.js');
require('./tile.js');
require('./table.js');
require('./nearest.js');
