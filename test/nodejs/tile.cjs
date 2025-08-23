var OSRM = require('../../');
var test = require('tape');
var data_path = require('./constants').data_path;
var tile = require('./constants').test_tile;

test.test('tile check size coarse', function(assert) {
    assert.plan(2);
    var osrm = new OSRM(data_path);
    osrm.tile(tile.at, function(err, result) {
        assert.ifError(err);
        assert.equal(result.length, tile.size);
    });
});

test.test('tile interface pre-conditions', function(assert) {
    assert.plan(6);
    var osrm = new OSRM(data_path);

    assert.throws(function() { osrm.tile(null, function(err, result) {}) }, /must be an array \[x, y, z\]/);
    assert.throws(function() { osrm.tile([], function(err, result) {}) }, /must be an array \[x, y, z\]/);
    assert.throws(function() { osrm.tile([[]], function(err, result) {}) }, /must be an array \[x, y, z\]/);
    assert.throws(function() { osrm.tile(undefined, function(err, result) {}) }, /must be an array \[x, y, z\]/);
    assert.throws(function() { osrm.tile(17059, 11948, 15, function(err, result) {}) }, /must be an array \[x, y, z\]/);
    assert.throws(function() { osrm.tile([17059, 11948, -15], function(err, result) {}) }, /must be unsigned/);
});

test.test('tile fails to load with geometry disabled', function(assert) {
    assert.plan(1);
    var osrm = new OSRM({'path': data_path, 'disable_feature_dataset': ['ROUTE_GEOMETRY']});
    osrm.tile(tile.at, function(err, result) {
        console.log(err)
        assert.match(err.message, /DisabledDatasetException/);
    });
});
test.test('tile ok with steps disabled', function(assert) {
    assert.plan(2);
    var osrm = new OSRM({'path': data_path, 'disable_feature_dataset': ['ROUTE_STEPS']});
    osrm.tile(tile.at, function(err, result) {
        assert.ifError(err);
        assert.equal(result.length, tile.size);
    });
});
