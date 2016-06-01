var OSRM = require('../../../build/bindings/node/Release/node-osrm.node').OSRM;
var test = require('tape');
var berlin_path = require('./osrm-data-path').data_path;

test('table: distance table in Berlin', function(assert) {
    assert.plan(9);
    var osrm = new OSRM(berlin_path);
    var options = {
        coordinates: [[13.43864,52.51993],[13.415852,52.513191]]
    };
    osrm.table(options, function(err, table) {
        assert.ifError(err);
        assert.ok(Array.isArray(table.durations), 'result must be an array');
        var row_count = table.durations.length;
        for (var i = 0; i < row_count; ++i) {
            var column = table.durations[i];
            var column_count = column.length;
            assert.equal(row_count, column_count);
            for (var j = 0; j < column_count; ++j) {
                if (i == j) {
                    // check that diagonal is zero
                    assert.equal(0, column[j], 'diagonal must be zero');
                } else {
                    // everything else is non-zero
                    assert.notEqual(0, column[j], 'other entries must be non-zero');
                }
            }
        }
        assert.equal(options.coordinates.length, row_count);
    });
});

test('table: distance table in Berlin with sources/destinations', function(assert) {
    assert.plan(6);
    var osrm = new OSRM(berlin_path);
    var options = {
        coordinates: [[13.43864,52.51993],[13.415852,52.513191]],
        sources: [0],
        destinations: [0,1]
    };
    osrm.table(options, function(err, table) {
        assert.ifError(err);
        assert.ok(Array.isArray(table.durations), 'result must be an array');
        var row_count = table.durations.length;
        for (var i = 0; i < row_count; ++i) {
            var column = table.durations[i];
            var column_count = column.length;
            assert.equal(options.destinations.length, column_count);
            for (var j = 0; j < column_count; ++j) {
                if (i == j) {
                    // check that diagonal is zero
                    assert.equal(0, column[j], 'diagonal must be zero');
                } else {
                    // everything else is non-zero
                    assert.notEqual(0, column[j], 'other entries must be non-zero');
                }
            }
        }
        assert.equal(options.sources.length, row_count);
    });
});

test('table: throws on invalid arguments', function(assert) {
    assert.plan(13);
    var osrm = new OSRM(berlin_path);
    var options = {};
    assert.throws(function() { osrm.table(options); },
        /Two arguments required/);
    options.coordinates = null;
    assert.throws(function() { osrm.table(options, function() {}); },
        /Coordinates must be an array of \(lon\/lat\) pairs/);
    options.coordinates = [[13.393252,52.542648]];
    assert.throws(function() { osrm.table(options, function(err, response) {}) },
        /At least two coordinates must be provided/);
    options.coordinates = [13.393252,52.542648];
    assert.throws(function() { osrm.table(options, function(err, response) {}) },
        /Coordinates must be an array of \(lon\/lat\) pairs/);
    options.coordinates = [[13.393252],[52.542648]];
    assert.throws(function() { osrm.table(options, function(err, response) {}) },
        /Coordinates must be an array of \(lon\/lat\) pairs/);

    options.coordinates = [[13.393252,52.542648],[13.393252,52.542648]];
    options.sources = true;
    assert.throws(function() { osrm.table(options, function(err, response) {}) },
        /Sources must be an array of indices \(or undefined\)/);
    options.sources = [0, 4];
    assert.throws(function() { osrm.table(options, function(err, response) {}) },
        /Source indices must be less than or equal to the number of coordinates/);
    options.sources = [0.3, 1.1];
    assert.throws(function() { osrm.table(options, function(err, response) {}) },
        /Source must be an integer/);

    options.destinations = true;
    delete options.sources;
    assert.throws(function() { osrm.table(options, function(err, response) {}) },
        /Destinations must be an array of indices \(or undefined\)/);
    options.destinations = [0, 4];
    assert.throws(function() { osrm.table(options, function(err, response) {}) },
        /Destination indices must be less than or equal to the number of coordinates/);
    options.destinations = [0.3, 1.1];
    assert.throws(function() { osrm.table(options, function(err, response) {}) },
        /Destination must be an integer/);

    // does not throw: the following two have been changed in OSRM v5
    options.sources = [0, 1];
    delete options.destinations;
    assert.doesNotThrow(function() { osrm.table(options, function(err, response) {}) },
        /Both sources and destinations need to be specified/);
    options.destinations = [0, 1];
    assert.doesNotThrow(function() { osrm.table(options, function(err, response) {}) },
        /You can either specify sources and destinations, or coordinates/);
});

test('table: throws on invalid arguments', function(assert) {
    assert.plan(1);
    var osrm = new OSRM(berlin_path);
    assert.throws(function() { osrm.table(null, function() {}); },
        /First arg must be an object/);
});
