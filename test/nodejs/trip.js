var OSRM = require('../../');
var test = require('tape');
var data_path = require('./constants').data_path;
var mld_data_path = require('./constants').mld_data_path;
var three_test_coordinates = require('./constants').three_test_coordinates;
var two_test_coordinates = require('./constants').two_test_coordinates;


test('trip: trip in Monaco', function(assert) {
    assert.plan(2);
    var osrm = new OSRM(data_path);
    osrm.trip({coordinates: two_test_coordinates}, function(err, trip) {
        assert.ifError(err);
        for (t = 0; t < trip.trips.length; t++) {
            assert.ok(trip.trips[t].geometry);
        }
    });
});

test('trip: trip in Monaco as a buffer', function(assert) {
    assert.plan(3);
    var osrm = new OSRM(data_path);
    osrm.trip({coordinates: two_test_coordinates}, { format: 'json_buffer' }, function(err, trip) {
        assert.ifError(err);
        assert.ok(trip instanceof Buffer);
        trip = JSON.parse(trip);
        for (t = 0; t < trip.trips.length; t++) {
            assert.ok(trip.trips[t].geometry);
        }
    });
});

test('trip: trip with many locations in Monaco', function(assert) {
    assert.plan(2);

    var many = 5;

    var osrm = new OSRM(data_path);
    var opts = {coordinates: three_test_coordinates.slice(0, many)};
    osrm.trip(opts, function(err, trip) {
        assert.ifError(err);
        for (t = 0; t < trip.trips.length; t++) {
            assert.ok(trip.trips[t].geometry);
        }
    });
});

test('trip: throws with too few or invalid args', function(assert) {
    assert.plan(3);
    var osrm = new OSRM(data_path);
    assert.throws(function() { osrm.trip({coordinates: two_test_coordinates}) },
        /Two arguments required/);
    assert.throws(function() { osrm.trip(null, function(err, trip) {}) },
        /First arg must be an object/);
    assert.throws(function() { osrm.trip({coordinates: two_test_coordinates}, { format: 'invalid' }, function(err, trip) {}) },
        /format must be a string:/);
});

test('trip: throws with bad params', function(assert) {
    assert.plan(14);
    var osrm = new OSRM(data_path);
    assert.throws(function () { osrm.trip({coordinates: []}, function(err) {}) });
    assert.throws(function() { osrm.trip({}, function(err, trip) {}) },
        /Must provide a coordinates property/);
    assert.throws(function() { osrm.trip({
        coordinates: null
    }, function(err, trip) {}) },
        /Coordinates must be an array of \(lon\/lat\) pairs/);
    assert.throws(function() { osrm.trip({
        coordinates: three_test_coordinates[0]
    }, function(err, trip) {}) },
        /Coordinates must be an array of \(lon\/lat\) pairs/);
    assert.throws(function() { osrm.trip({
        coordinates: [three_test_coordinates[0][0], three_test_coordinates[0][1]]
    }, function(err, trip) {}) },
        /Coordinates must be an array of \(lon\/lat\) pairs/);
    assert.throws(function() { osrm.trip({
        coordinates: two_test_coordinates,
        hints: null
    }, function(err, trip) {}) },
        /Hints must be an array of strings\/null/);
    var options = {
        coordinates: [three_test_coordinates[0], three_test_coordinates[1]],
        hints: three_test_coordinates[0]
    };
    assert.throws(function() { osrm.trip(options, function(err, trip) {}); },
        /Hint must be null or string/);
    options.hints = [null];
    assert.throws(function() { osrm.trip(options, function(err, trip) {}); },
        /Hints array must have the same length as coordinates array/);
    delete options.hints;
    options.geometries = 'false';
    assert.throws(function() { osrm.trip(options, function(err, trip) {}); },
        /'geometries' param must be one of \[polyline, polyline6, geojson\]/);
    delete options.geometries;
    options.source = false;
    assert.throws(function() { osrm.trip(options, function(err, trip) {}); },
        /Source must be a string: \[any, first\]/);
    options.source = 'false';
    assert.throws(function() { osrm.trip(options, function(err, trip) {}); },
        /'source' param must be one of \[any, first\]/);
    delete options.source;
    options.destination = true;
    assert.throws(function() { osrm.trip(options, function(err, trip) {}); },
        /Destination must be a string: \[any, last\]/);
    options.destination = 'true';
    assert.throws(function() { osrm.trip(options, function(err, trip) {}); },
        /'destination' param must be one of \[any, last\]/);
    options.roundtrip = 'any';
    assert.throws(function() { osrm.trip(options, function(err, trip) {}); },
        /'roundtrip' param must be a boolean/);
});

test('trip: routes Monaco using shared memory', function(assert) {
    assert.plan(2);
    var osrm = new OSRM();
    osrm.trip({coordinates: two_test_coordinates}, function(err, trip) {
        assert.ifError(err);
            for (t = 0; t < trip.trips.length; t++) {
                assert.ok(trip.trips[t].geometry);
            }
    });
});

test('trip: routes Monaco with hints', function(assert) {
    assert.plan(5);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: two_test_coordinates,
        steps: false
    };
    osrm.trip(options, function(err, first) {
        assert.ifError(err);
        for (t = 0; t < first.trips.length; t++) {
            assert.ok(first.trips[t].geometry);
        }
        var hints = first.waypoints.map(function(wp) { return wp.hint; });
        assert.ok(hints.every(function(h) { return typeof h === 'string'; }));
        options.hints = hints;

        osrm.trip(options, function(err, second) {
            assert.ifError(err);
            assert.deepEqual(first, second);
        });
    });
});

test('trip: trip through Monaco with geometry compression', function(assert) {
    assert.plan(2);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: [three_test_coordinates[0], three_test_coordinates[1]]
    };
    osrm.trip(options, function(err, trip) {
        assert.ifError(err);
        for (t = 0; t < trip.trips.length; t++) {
            assert.equal('string', typeof trip.trips[t].geometry);
        }
    });
});

test('trip: trip through Monaco without geometry compression', function(assert) {
    assert.plan(2);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: two_test_coordinates,
        geometries: 'geojson'
    };
    osrm.trip(options, function(err, trip) {
        assert.ifError(err);
        for (t = 0; t < trip.trips.length; t++) {
            assert.ok(Array.isArray(trip.trips[t].geometry.coordinates));
        }
    });
});

test('trip: trip through Monaco with speed annotations options', function(assert) {
    assert.plan(12);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: two_test_coordinates,
        steps: true,
        annotations: ['speed'],
        overview: 'false'
    };
    osrm.trip(options, function(err, trip) {
        assert.ifError(err);
        assert.equal(trip.trips.length, 1);
        for (t = 0; t < trip.trips.length; t++) {
            assert.ok(trip.trips[t]);
            assert.ok(trip.trips[t].legs.every(function(l) { return l.steps.length > 0; }), 'every leg has steps')
            assert.ok(trip.trips[t].legs.every(function(l) { return l.annotation; }), 'every leg has annotations')
            assert.ok(trip.trips[t].legs.every(function(l) { return l.annotation.speed; }), 'every leg has annotations for speed')
            assert.notOk(trip.trips[t].legs.every(function(l) { return l.annotation.weight; }), 'has no annotations for weight')
            assert.notOk(trip.trips[t].legs.every(function(l) { return l.annotation.datasources; }), 'has no annotations for datasources')
            assert.notOk(trip.trips[t].legs.every(function(l) { return l.annotation.duration; }), 'has no annotations for duration')
            assert.notOk(trip.trips[t].legs.every(function(l) { return l.annotation.distance; }), 'has no annotations for distance')
            assert.notOk(trip.trips[t].legs.every(function(l) { return l.annotation.nodes; }), 'has no annotations for nodes')
            assert.notOk(trip.trips[t].geometry);
        }
    });
});

test('trip: trip through Monaco with several (duration, distance, nodes) annotations options', function(assert) {
    assert.plan(12);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: two_test_coordinates,
        steps: true,
        annotations: ['duration', 'distance', 'nodes'],
        overview: 'false'
    };
    osrm.trip(options, function(err, trip) {
        assert.ifError(err);
        assert.equal(trip.trips.length, 1);
        for (t = 0; t < trip.trips.length; t++) {
            assert.ok(trip.trips[t]);
            assert.ok(trip.trips[t].legs.every(function(l) { return l.steps.length > 0; }), 'every leg has steps')
            assert.ok(trip.trips[t].legs.every(function(l) { return l.annotation; }), 'every leg has annotations')
            assert.ok(trip.trips[t].legs.every(function(l) { return l.annotation.duration; }), 'every leg has annotations for duration')
            assert.ok(trip.trips[t].legs.every(function(l) { return l.annotation.distance; }), 'every leg has annotations for distance')
            assert.ok(trip.trips[t].legs.every(function(l) { return l.annotation.nodes; }), 'every leg has annotations for nodes')
            assert.notOk(trip.trips[t].legs.every(function(l) { return l.annotation.weight; }), 'has no annotations for weight')
            assert.notOk(trip.trips[t].legs.every(function(l) { return l.annotation.datasources; }), 'has no annotations for datasources')
            assert.notOk(trip.trips[t].legs.every(function(l) { return l.annotation.speed; }), 'has no annotations for speed')
            assert.notOk(trip.trips[t].geometry);
        }
    });
});

test('trip: trip through Monaco with options', function(assert) {
    assert.plan(6);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: two_test_coordinates,
        steps: true,
        annotations: true,
        overview: 'false'
    };
    osrm.trip(options, function(err, trip) {
        assert.ifError(err);
        assert.equal(trip.trips.length, 1);
        for (t = 0; t < trip.trips.length; t++) {
            assert.ok(trip.trips[t]);
            assert.ok(trip.trips[t].legs.every(function(l) { return l.steps.length > 0; }), 'every leg has steps')
            assert.ok(trip.trips[t].legs.every(function(l) { return l.annotation; }), 'every leg has annotations')
            assert.notOk(trip.trips[t].geometry);
        }
    });
});

test('trip: routes Monaco with null hints', function(assert) {
    assert.plan(1);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: [three_test_coordinates[0], three_test_coordinates[1]],
        hints: [null, null]
    };
    osrm.trip(options, function(err, second) {
        assert.ifError(err);
    });
});

test('trip: service combinations that are not implemented', function(assert) {
    assert.plan(3);
    var osrm = new OSRM(data_path);

    // fixed start, non-roundtrip
    var options = {
        coordinates: two_test_coordinates,
        source: 'first',
        roundtrip: false
    };
    osrm.trip(options, function(err, second) {
        assert.equal('NotImplemented', err.message);
    });

    // fixed start, fixed end, non-roundtrip
    options.source = 'any';
    options.destination = 'any';
    osrm.trip(options, function(err, second) {
        assert.equal('NotImplemented', err.message);
    });

    // fixed end, non-roundtrip
    delete options.source;
    options.destination = 'last';
    osrm.trip(options, function(err, second) {
        assert.equal('NotImplemented', err.message);
    });

});

test('trip: fixed start and end combinations', function(assert) {
    var osrm = new OSRM(data_path);

    var options = {
        coordinates: two_test_coordinates,
        source: 'first',
        destination: 'last',
        roundtrip: false,
        geometries: 'geojson'
    };

    // fixed start and end, non-roundtrip
    osrm.trip(options, function(err, fseTrip) {
        assert.ifError(err);
        assert.equal(1, fseTrip.trips.length);
        var coordinates = fseTrip.trips[0].geometry.coordinates;
        assert.notEqual(JSON.stringify(coordinates[0]), JSON.stringify(coordinates[coordinates.length - 1]));
    });

    // variations of roundtrip

    var roundtripChecks = function(options) {
        osrm.trip(options, function(err, trip) {
            assert.ifError(err);
            assert.equal(1, trip.trips.length);
            var coordinates = trip.trips[0].geometry.coordinates;
            assert.equal(JSON.stringify(coordinates[0]), JSON.stringify(coordinates[coordinates.length - 1]));
        });
    }

    // roundtrip, source and destination not specified
    roundtripChecks({coordinates: options.coordinates, geometries: options.geometries});

    // roundtrip, fixed destination
    options.roundtrip = true;
    delete options.source;
    roundtripChecks(options);

    //roundtrip, fixed source
    delete options.destination;
    options.source = 'first';
    roundtripChecks(options);

    // roundtrip, non-fixed source, non-fixed destination
    options.source = 'any';
    options.destination = 'any';
    roundtripChecks(options);

    assert.end();
});

test('trip: trip in Monaco without motorways', function(assert) {
    assert.plan(3);
    var osrm = new OSRM({path: mld_data_path, algorithm: 'MLD'});
    var options = {
        coordinates: two_test_coordinates,
        exclude: ['motorway']
    };
    osrm.trip(options, function(err, response) {
        assert.ifError(err);
        assert.equal(response.waypoints.length, 2);
        assert.equal(response.trips.length, 1);
    });
});

