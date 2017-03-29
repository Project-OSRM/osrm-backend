var OSRM = require('../../');
var test = require('tape');
var data_path = require('./constants').data_path;
var three_test_coordinates = require('./constants').three_test_coordinates;
var two_test_coordinates = require('./constants').two_test_coordinates;


test('table: distance table in Monaco', function(assert) {
    assert.plan(11);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: [three_test_coordinates[0], three_test_coordinates[1]]
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
                    // and finite (not nan, inf etc.)
                    assert.ok(Number.isFinite(column[j]), 'distance is finite number');
                }
            }
        }
        assert.equal(options.coordinates.length, row_count);
    });
});

test('table: distance table in Monaco with sources/destinations', function(assert) {
    assert.plan(7);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: [three_test_coordinates[0], three_test_coordinates[1]],
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
                    // and finite (not nan, inf etc.)
                    assert.ok(Number.isFinite(column[j]), 'distance is finite number');
                }
            }
        }
        assert.equal(options.sources.length, row_count);
    });
});

test('table: throws on invalid arguments', function(assert) {
    assert.plan(14);
    var osrm = new OSRM(data_path);
    var options = {};
    assert.throws(function() { osrm.table(options); },
        /Two arguments required/);
    options.coordinates = null;
    assert.throws(function() { osrm.table(options, function() {}); },
        /Coordinates must be an array of \(lon\/lat\) pairs/);
    options.coordinates = [three_test_coordinates[0]];
    assert.throws(function() { osrm.table(options, function(err, response) {}) },
        /At least two coordinates must be provided/);
    options.coordinates = three_test_coordinates[0];
    assert.throws(function() { osrm.table(options, function(err, response) {}) },
        /Coordinates must be an array of \(lon\/lat\) pairs/);
    options.coordinates = [three_test_coordinates[0][0], three_test_coordinates[0][1]];
    assert.throws(function() { osrm.table(options, function(err, response) {}) },
        /Coordinates must be an array of \(lon\/lat\) pairs/);

    options.coordinates = two_test_coordinates;
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

    assert.throws(function() { osrm.route({coordinates: two_test_coordinates, generate_hints: null}, function(err, route) {}) },
        /generate_hints must be of type Boolean/);
});

test('table: throws on invalid arguments', function(assert) {
    assert.plan(1);
    var osrm = new OSRM(data_path);
    assert.throws(function() { osrm.table(null, function() {}); },
        /First arg must be an object/);
});

test('table: distance table in Monaco with hints', function(assert) {
    assert.plan(5);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: two_test_coordinates,
        generate_hints: true  // true is default but be explicit here
    };
    osrm.table(options, function(err, table) {
        console.log(table);
        assert.ifError(err);

        function assertHasHints(waypoint) {
            assert.notStrictEqual(waypoint.hint, undefined);
        }

        table.sources.map(assertHasHints);
        table.destinations.map(assertHasHints);
    });
});

test('table: distance table in Monaco without hints', function(assert) {
    assert.plan(5);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: two_test_coordinates,
        generate_hints: false  // true is default
    };
    osrm.table(options, function(err, table) {
        assert.ifError(err);

        function assertHasNoHints(waypoint) {
            assert.strictEqual(waypoint.hint, undefined);
        }

        table.sources.map(assertHasNoHints);
        table.destinations.map(assertHasNoHints);
    });
});
