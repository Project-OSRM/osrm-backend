// Test trip service functionality for solving traveling salesman problem
import OSRM from '../../lib/index.js';
import test from 'tape';
import { data_path, mld_data_path, three_test_coordinates, two_test_coordinates } from './constants.js';
import flatbuffers from 'flatbuffers';
import { osrm } from '../../features/support/fbresult_generated.js';

const FBResult = osrm.engine.api.fbresult.FBResult;

test('trip: trip in Monaco with flatbuffers format', function(assert) {
    assert.plan(2);
    const osrm = new OSRM(data_path);
    osrm.trip({coordinates: two_test_coordinates, format: 'flatbuffers'}, function(err, trip) {
        assert.ifError(err);
        const fb = FBResult.getRootAsFBResult(new flatbuffers.ByteBuffer(trip));
        assert.equal(fb.routesLength(), 1);
    });
});

test('trip: trip in Monaco', function(assert) {
    assert.plan(2);
    const osrm = new OSRM(data_path);
    osrm.trip({coordinates: two_test_coordinates}, function(err, trip) {
        assert.ifError(err);
        for (let t = 0; t < trip.trips.length; t++) {
            assert.ok(trip.trips[t].geometry);
        }
    });
});

test('trip: trip in Monaco as a buffer', function(assert) {
    assert.plan(3);
    const osrm = new OSRM(data_path);
    osrm.trip({coordinates: two_test_coordinates}, { format: 'json_buffer' }, function(err, trip) {
        assert.ifError(err);
        assert.ok(trip instanceof Buffer);
        trip = JSON.parse(trip);
        for (let t = 0; t < trip.trips.length; t++) {
            assert.ok(trip.trips[t].geometry);
        }
    });
});

test('trip: trip with many locations in Monaco', function(assert) {
    assert.plan(2);

    const many = 5;

    const osrm = new OSRM(data_path);
    const opts = {coordinates: three_test_coordinates.slice(0, many)};
    osrm.trip(opts, function(err, trip) {
        assert.ifError(err);
        for (let t = 0; t < trip.trips.length; t++) {
            assert.ok(trip.trips[t].geometry);
        }
    });
});

test('trip: throws with too few or invalid args', function(assert) {
    assert.plan(3);
    const osrm = new OSRM(data_path);
    assert.throws(function() { osrm.trip({coordinates: two_test_coordinates}) },
        /Two arguments required/);
    assert.throws(function() { osrm.trip(null, function(err, trip) {}) },
        /First arg must be an object/);
    assert.throws(function() { osrm.trip({coordinates: two_test_coordinates}, { format: 'invalid' }, function(err, trip) {}) },
        /format must be a string:/);
});

test('trip: throws with bad params', function(assert) {
    assert.plan(14);
    const osrm = new OSRM(data_path);
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
    const options = {
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
    const osrm = new OSRM();
    osrm.trip({coordinates: two_test_coordinates}, function(err, trip) {
        assert.ifError(err);
            for (let t = 0; t < trip.trips.length; t++) {
                assert.ok(trip.trips[t].geometry);
            }
    });
});

test('trip: routes Monaco with hints', function(assert) {
    assert.plan(5);
    const osrm = new OSRM(data_path);
    const options = {
        coordinates: two_test_coordinates,
        steps: false
    };
    osrm.trip(options, function(err, first) {
        assert.ifError(err);
        for (let t = 0; t < first.trips.length; t++) {
            assert.ok(first.trips[t].geometry);
        }
        const hints = first.waypoints.map(function(wp) { return wp.hint; });
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
    const osrm = new OSRM(data_path);
    const options = {
        coordinates: [three_test_coordinates[0], three_test_coordinates[1]]
    };
    osrm.trip(options, function(err, trip) {
        assert.ifError(err);
        for (let t = 0; t < trip.trips.length; t++) {
            assert.equal('string', typeof trip.trips[t].geometry);
        }
    });
});

test('trip: trip through Monaco without geometry compression', function(assert) {
    assert.plan(2);
    const osrm = new OSRM(data_path);
    const options = {
        coordinates: two_test_coordinates,
        geometries: 'geojson'
    };
    osrm.trip(options, function(err, trip) {
        assert.ifError(err);
        for (let t = 0; t < trip.trips.length; t++) {
            assert.ok(Array.isArray(trip.trips[t].geometry.coordinates));
        }
    });
});

test('trip: trip through Monaco with speed annotations options', function(assert) {
    assert.plan(12);
    const osrm = new OSRM(data_path);
    const options = {
        coordinates: two_test_coordinates,
        steps: true,
        annotations: ['speed'],
        overview: 'false'
    };
    osrm.trip(options, function(err, trip) {
        assert.ifError(err);
        assert.equal(trip.trips.length, 1);
        for (let t = 0; t < trip.trips.length; t++) {
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
    const osrm = new OSRM(data_path);
    const options = {
        coordinates: two_test_coordinates,
        steps: true,
        annotations: ['duration', 'distance', 'nodes'],
        overview: 'false'
    };
    osrm.trip(options, function(err, trip) {
        assert.ifError(err);
        assert.equal(trip.trips.length, 1);
        for (let t = 0; t < trip.trips.length; t++) {
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
    const osrm = new OSRM(data_path);
    const options = {
        coordinates: two_test_coordinates,
        steps: true,
        annotations: true,
        overview: 'false'
    };
    osrm.trip(options, function(err, trip) {
        assert.ifError(err);
        assert.equal(trip.trips.length, 1);
        for (let t = 0; t < trip.trips.length; t++) {
            assert.ok(trip.trips[t]);
            assert.ok(trip.trips[t].legs.every(function(l) { return l.steps.length > 0; }), 'every leg has steps')
            assert.ok(trip.trips[t].legs.every(function(l) { return l.annotation; }), 'every leg has annotations')
            assert.notOk(trip.trips[t].geometry);
        }
    });
});

test('trip: routes Monaco with null hints', function(assert) {
    assert.plan(1);
    const osrm = new OSRM(data_path);
    const options = {
        coordinates: [three_test_coordinates[0], three_test_coordinates[1]],
        hints: [null, null]
    };
    osrm.trip(options, function(err, second) {
        assert.ifError(err);
    });
});

test('trip: service combinations that are not implemented', function(assert) {
    assert.plan(1);
    const osrm = new OSRM(data_path);

    // no fixed start, no fixed end, non-roundtrip
    const options = {
        coordinates: two_test_coordinates,
        source: 'any',
        destination: 'any',
        roundtrip: false
    };
    osrm.trip(options, function(err, second) {
        assert.equal('NotImplemented', err.message);
    });
});

test('trip: fixed start and end combinations', function(assert) {
    assert.plan(21);
    const osrm = new OSRM(data_path);

    const options = {
        coordinates: two_test_coordinates,
        source: 'first',
        destination: 'last',
        roundtrip: false,
        geometries: 'geojson'
    };

    // variations of non roundtrip
    const nonRoundtripChecks = function(options) {
        osrm.trip(options, function(err, fseTrip) {
            assert.ifError(err);
            assert.equal(1, fseTrip.trips.length);
            const coordinates = fseTrip.trips[0].geometry.coordinates;
            assert.notEqual(JSON.stringify(coordinates[0]), JSON.stringify(coordinates[coordinates.length - 1]));
        });
    };

    // variations of roundtrip
    const roundtripChecks = function(options) {
        osrm.trip(options, function(err, trip) {
            assert.ifError(err);
            assert.equal(1, trip.trips.length);
            const coordinates = trip.trips[0].geometry.coordinates;
            assert.equal(JSON.stringify(coordinates[0]), JSON.stringify(coordinates[coordinates.length - 1]));
        });
    };

    // fixed start and end, non-roundtrip
    nonRoundtripChecks(options);

    // fixed start, non-roundtrip
    delete options.destination;
    options.source = 'first';
    nonRoundtripChecks(options);

    // fixed end, non-roundtrip
    delete options.source;
    options.destination = 'last';
    nonRoundtripChecks(options);

    // roundtrip, source and destination not specified
    roundtripChecks({coordinates: options.coordinates, geometries: options.geometries});

    // roundtrip, fixed destination
    options.roundtrip = true;
    delete options.source;
    options.destination = 'last';
    roundtripChecks(options);

    //roundtrip, fixed source
    delete options.destination;
    options.source = 'first';
    roundtripChecks(options);

    // roundtrip, non-fixed source, non-fixed destination
    options.source = 'any';
    options.destination = 'any';
    roundtripChecks(options);
});

test('trip: trip in Monaco without motorways', function(assert) {
    assert.plan(3);
    const osrm = new OSRM({path: mld_data_path, algorithm: 'MLD'});
    const options = {
        coordinates: two_test_coordinates,
        exclude: ['motorway']
    };
    osrm.trip(options, function(err, response) {
        assert.ifError(err);
        assert.equal(response.waypoints.length, 2);
        assert.equal(response.trips.length, 1);
    });
});


test('trip: throws on disabled geometry', function (assert) {
    assert.plan(1);
    const osrm = new OSRM({'path': data_path, 'disable_feature_dataset': ['ROUTE_GEOMETRY']});
    const options = {
        coordinates: three_test_coordinates.concat(three_test_coordinates),
    };
    osrm.trip(options, function(err, route) {
        console.log(err)
        assert.match(err.message, /DisabledDatasetException/);
    });
});

test('trip: ok on disabled geometry', function (assert) {
    assert.plan(2);
    const osrm = new OSRM({'path': data_path, 'disable_feature_dataset': ['ROUTE_GEOMETRY']});
    const options = {
        steps: false,
        overview: 'false',
        annotations: false,
        skip_waypoints: true,
        coordinates: three_test_coordinates.concat(three_test_coordinates),
    };
    osrm.trip(options, function(err, response) {
        assert.ifError(err);
        assert.equal(response.trips.length, 1);
    });
});

test('trip: throws on disabled steps', function (assert) {
    assert.plan(1);
    const osrm = new OSRM({'path': data_path, 'disable_feature_dataset': ['ROUTE_STEPS']});
    const options = {
        steps: true,
        coordinates: three_test_coordinates.concat(three_test_coordinates),
    };
    osrm.trip(options, function(err, route) {
        console.log(err)
        assert.match(err.message, /DisabledDatasetException/);
    });
});

test('trip: ok on disabled steps', function (assert) {
    assert.plan(8);
    const osrm = new OSRM({'path': data_path, 'disable_feature_dataset': ['ROUTE_STEPS']});
    const options = {
        steps: false,
        overview: 'simplified',
        annotations: true,
        coordinates: three_test_coordinates.concat(three_test_coordinates),
    };
    osrm.trip(options, function(err, response) {
        assert.ifError(err);
        assert.ok(response.waypoints);
        assert.ok(response.trips);
        assert.equal(response.trips.length, 1);
        assert.ok(response.trips[0].geometry, "trip has geometry");
        assert.ok(response.trips[0].legs, "trip has legs");
        assert.notok(response.trips[0].legs.every(l => { return l.steps.length > 0; }), 'every leg has steps');
        assert.ok(response.trips[0].legs.every(l => { return l.annotation;}), 'every leg has annotations');
    });
});
