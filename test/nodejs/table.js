var OSRM = require('../../');
var test = require('tape');
var data_path = require('./constants').data_path;
var mld_data_path = require('./constants').mld_data_path;
var three_test_coordinates = require('./constants').three_test_coordinates;
var two_test_coordinates = require('./constants').two_test_coordinates;

test('table: test annotations paramater combination', function(assert) {
    assert.plan(12);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: [three_test_coordinates[0], three_test_coordinates[1]],
        annotations: ['distance']
    };
    osrm.table(options, function(err, table) {
        assert.ifError(err);
        assert.ok(table['distances'], 'distances table result should exist');
        assert.notOk(table['durations'], 'durations table result should not exist');
    });

    options = {
        coordinates: [three_test_coordinates[0], three_test_coordinates[1]],
        annotations: ['duration']
    };
    osrm.table(options, function(err, table) {
        assert.ifError(err);
        assert.ok(table['durations'], 'durations table result should exist');
        assert.notOk(table['distances'], 'distances table result should not exist');
    });

    options = {
        coordinates: [three_test_coordinates[0], three_test_coordinates[1]],
        annotations: ['duration', 'distance']
    };
    osrm.table(options, function(err, table) {
        assert.ifError(err);
        assert.ok(table['durations'], 'durations table result should exist');
        assert.ok(table['distances'], 'distances table result should exist');
    });

    options = {
        coordinates: [three_test_coordinates[0], three_test_coordinates[1]]
    };
    osrm.table(options, function(err, table) {
        assert.ifError(err);
        assert.ok(table['durations'], 'durations table result should exist');
        assert.notOk(table['distances'], 'distances table result should not exist');
    });
});

test('table: returns buffer', function(assert) {
    assert.plan(3);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: [three_test_coordinates[0], three_test_coordinates[1]],
    };
    osrm.table(options, { format: 'json_buffer' }, function(err, table) {
        assert.ifError(err);
        assert.ok(table instanceof Buffer);
        table = JSON.parse(table);
        assert.ok(table['durations'], 'distances table result should exist');
    });
});

var tables = ['distances', 'durations'];

tables.forEach(function(annotation) {
    test('table: ' + annotation + ' table in Monaco', function(assert) {
        assert.plan(11);
        var osrm = new OSRM(data_path);
        var options = {
            coordinates: [three_test_coordinates[0], three_test_coordinates[1]],
            annotations: [annotation.slice(0,-1)]
        };
        osrm.table(options, function(err, table) {
            assert.ifError(err);
            assert.ok(Array.isArray(table[annotation]), 'result must be an array');
            var row_count = table[annotation].length;
            for (var i = 0; i < row_count; ++i) {
                var column = table[annotation][i];
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

    test('table: ' + annotation + ' table in Monaco with sources/destinations', function(assert) {
        assert.plan(7);
        var osrm = new OSRM(data_path);
        var options = {
            coordinates: [three_test_coordinates[0], three_test_coordinates[1]],
            sources: [0],
            destinations: [0,1],
            annotations: [annotation.slice(0,-1)]
        };
        osrm.table(options, function(err, table) {
            assert.ifError(err);
            assert.ok(Array.isArray(table[annotation]), 'result must be an array');
            var row_count = table[annotation].length;
            for (var i = 0; i < row_count; ++i) {
                var column = table[annotation][i];
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

    test('table: ' + annotation + ' throws on invalid arguments', function(assert) {
        assert.plan(15);
        var osrm = new OSRM(data_path);
        var options = {annotations: [annotation.slice(0,-1)]};
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
        assert.throws(function() { osrm.table(options, { format: 'invalid' }, function(err, response) {}) },
            /format must be a string:/);

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

    test('table: ' + annotation + ' table in Monaco with hints', function(assert) {
        assert.plan(5);
        var osrm = new OSRM(data_path);
        var options = {
            coordinates: two_test_coordinates,
            generate_hints: true,   // true is default but be explicit here
            annotations: [annotation.slice(0,-1)]
        };
        osrm.table(options, function(err, table) {
            assert.ifError(err);

            function assertHasHints(waypoint) {
                assert.notStrictEqual(waypoint.hint, undefined);
            }

            table.sources.map(assertHasHints);
            table.destinations.map(assertHasHints);
        });
    });

    test('table: ' + annotation + ' table in Monaco without hints', function(assert) {
        assert.plan(5);
        var osrm = new OSRM(data_path);
        var options = {
            coordinates: two_test_coordinates,
            generate_hints: false,  // true is default
            annotations: [annotation.slice(0,-1)]
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

    test('table: ' + annotation + ' table in Monaco without motorways', function(assert) {
        assert.plan(2);
        var osrm = new OSRM({path: mld_data_path, algorithm: 'MLD'});
        var options = {
            coordinates: two_test_coordinates,
            exclude: ['motorway'],
            annotations: [annotation.slice(0,-1)]
        };
        osrm.table(options, function(err, response) {
            assert.equal(response[annotation].length, 2);
            assert.strictEqual(response.fallback_speed_cells, undefined);
        });
    });

    test('table: ' + annotation + ' table in Monaco with fallback speeds', function(assert) {
        assert.plan(2);
        var osrm = new OSRM({path: mld_data_path, algorithm: 'MLD'});
        var options = {
            coordinates: two_test_coordinates,
            annotations: [annotation.slice(0,-1)],
            fallback_speed: 1,
            fallback_coordinate: 'input'
        };
        osrm.table(options, function(err, response) {
            assert.equal(response[annotation].length, 2);
            assert.equal(response['fallback_speed_cells'].length, 0);
        });
    });

    test('table: ' + annotation + ' table in Monaco with invalid fallback speeds and fallback coordinates', function(assert) {
        assert.plan(4);
        var osrm = new OSRM({path: mld_data_path, algorithm: 'MLD'});
        var options = {
            coordinates: two_test_coordinates,
            annotations: [annotation.slice(0,-1)],
            fallback_speed: -1
        };

        assert.throws(()=>osrm.table(options, (err, res) => {}), /fallback_speed must be > 0/, "should throw on invalid fallback_speeds");

        options.fallback_speed = '10';
        assert.throws(()=>osrm.table(options, (err, res) => {}), /fallback_speed must be a number/, "should throw on invalid fallback_speeds");

        options.fallback_speed = 10;
        options.fallback_coordinate = 'bla';
        assert.throws(()=>osrm.table(options, (err, res) => {}), /fallback_coordinate' param must be one of \[input, snapped\]/, "should throw on invalid fallback_coordinate");

        options.fallback_coordinate = 10;
        assert.throws(()=>osrm.table(options, (err, res) => {}), /fallback_coordinate must be a string: \[input, snapped\]/, "should throw on invalid fallback_coordinate");

    });

    test('table: ' + annotation + ' table in Monaco with invalid scale factor', function(assert) {
        assert.plan(3);
        var osrm = new OSRM({path: mld_data_path, algorithm: 'MLD'});
        var options = {
            coordinates: two_test_coordinates,
            annotations: [annotation.slice(0,-1)],
            scale_factor: -1
        };

        assert.throws(()=>osrm.table(options, (err, res) => {}), /scale_factor must be > 0/, "should throw on invalid scale_factor value");

        options.scale_factor = '-1';
        assert.throws(()=>osrm.table(options, (err, res) => {}), /scale_factor must be a number/, "should throw on invalid scale_factor value");

        options.scale_factor = 0;
        assert.throws(()=>osrm.table(options, (err, res) => {}), /scale_factor must be > 0/, "should throw on invalid scale_factor value");

    });
});

