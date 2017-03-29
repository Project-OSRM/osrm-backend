var OSRM = require('../../');
var test = require('tape');
var data_path = require('./constants').data_path;
var three_test_coordinates = require('./constants').three_test_coordinates;
var two_test_coordinates = require('./constants').two_test_coordinates;

test('match: match in Monaco', function(assert) {
    assert.plan(5);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: three_test_coordinates,
        timestamps: [1424684612, 1424684616, 1424684620]
    };
    osrm.match(options, function(err, response) {
        assert.ifError(err);
        assert.equal(response.matchings.length, 1);
        assert.ok(response.matchings.every(function(m) {
            return !!m.distance && !!m.duration && Array.isArray(m.legs) && !!m.geometry && m.confidence > 0;
        }))
        assert.equal(response.tracepoints.length, 3);
        assert.ok(response.tracepoints.every(function(t) {
            return !!t.hint && !isNaN(t.matchings_index) && !isNaN(t.waypoint_index) && !!t.name;
        }));
    });
});

test('match: match in Monaco without timestamps', function(assert) {
    assert.plan(3);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: three_test_coordinates
    };
    osrm.match(options, function(err, response) {
        assert.ifError(err);
        assert.equal(response.tracepoints.length, 3);
        assert.equal(response.matchings.length, 1);
    });
});

test('match: match in Monaco without geometry compression', function(assert) {
    assert.plan(4);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: three_test_coordinates,
        geometries: 'geojson'
    };
    osrm.match(options, function(err, response) {
        assert.ifError(err);
        assert.equal(response.matchings.length, 1);
        assert.ok(response.matchings[0].geometry instanceof Object);
        assert.ok(Array.isArray(response.matchings[0].geometry.coordinates));
    });
});

test('match: match in Monaco with geometry compression', function(assert) {
    assert.plan(3);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: three_test_coordinates,
    };
    osrm.match(options, function(err, response) {
        assert.ifError(err);
        assert.equal(response.matchings.length, 1);
        assert.equal('string', typeof response.matchings[0].geometry);
    });
});

test('match: match in Monaco with speed annotations options', function(assert) {
    assert.plan(12);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: three_test_coordinates,
        timestamps: [1424684612, 1424684616, 1424684620],
        radiuses: [4.07, 4.07, 4.07],
        steps: true,
        annotations: ['speed'],
        overview: 'false',
        geometries: 'geojson'
    };
    osrm.match(options, function(err, response) {
        assert.ifError(err);
        assert.equal(response.matchings.length, 1);
        assert.ok(response.matchings[0].confidence > 0, 'has confidence');
        assert.ok(response.matchings[0].legs.every((l) => {return l.steps.length > 0; }), 'every leg has steps');
        assert.ok(response.matchings[0].legs.every((l) => {return l.annotation; }), 'every leg has annotations');
        assert.ok(response.matchings[0].legs.every((l) => {return l.annotation.speed; }), 'every leg has annotations for speed');
        assert.notOk(response.matchings[0].legs.every((l) => {return l.annotation.weight; }), 'has no annotations for weight')
        assert.notOk(response.matchings[0].legs.every((l) => {return l.annotation.datasources; }), 'has no annotations for datasources')
        assert.notOk(response.matchings[0].legs.every((l) => {return l.annotation.duration; }), 'has no annotations for duration')
        assert.notOk(response.matchings[0].legs.every((l) => {return l.annotation.distance; }), 'has no annotations for distance')
        assert.notOk(response.matchings[0].legs.every((l) => {return l.annotation.nodes; }), 'has no annotations for nodes')
        assert.equal(undefined, response.matchings[0].geometry);
    });
});


test('match: match in Monaco with several (duration, distance, nodes) annotations options', function(assert) {
    assert.plan(12);
    var osrm = new OSRM(data_path);
    var options = {
        timestamps: [1424684612, 1424684616, 1424684620],
        coordinates: three_test_coordinates,
        timestamps: [1424684612, 1424684616, 1424684620],
        radiuses: [4.07, 4.07, 4.07],
        steps: true,
        annotations: ['duration','distance','nodes'],
        overview: 'false',
        geometries: 'geojson'
    };
    osrm.match(options, function(err, response) {
        assert.ifError(err);
        assert.equal(response.matchings.length, 1);
        assert.ok(response.matchings[0].confidence > 0, 'has confidence');
        assert.ok(response.matchings[0].legs.every((l) => {return l.steps.length > 0; }), 'every leg has steps');
        assert.ok(response.matchings[0].legs.every((l) => {return l.annotation; }), 'every leg has annotations');
        assert.ok(response.matchings[0].legs.every((l) => {return l.annotation.distance; }), 'every leg has annotations for distance');
        assert.ok(response.matchings[0].legs.every((l) => {return l.annotation.duration; }), 'every leg has annotations for durations');
        assert.ok(response.matchings[0].legs.every((l) => {return l.annotation.nodes; }), 'every leg has annotations for nodes');
        assert.notOk(response.matchings[0].legs.every((l) => {return l.annotation.weight; }), 'has no annotations for weight')
        assert.notOk(response.matchings[0].legs.every((l) => {return l.annotation.datasources; }), 'has no annotations for datasources')
        assert.notOk(response.matchings[0].legs.every((l) => {return l.annotation.speed; }), 'has no annotations for speed')
        assert.equal(undefined, response.matchings[0].geometry);
    });
});

test('match: match in Monaco with all options', function(assert) {
    assert.plan(8);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: three_test_coordinates,
        timestamps: [1424684612, 1424684616, 1424684620],
        radiuses: [4.07, 4.07, 4.07],
        steps: true,
        annotations: true,
        overview: 'false',
        geometries: 'geojson'
    };
    osrm.match(options, function(err, response) {
        assert.ifError(err);
        assert.equal(response.matchings.length, 1);
        assert.ok(response.matchings[0].confidence > 0, 'has confidence');
        assert.ok(response.matchings[0].legs.every((l) => {return l.steps.length > 0; }), 'every leg has steps');
        assert.ok(response.matchings[0].legs.every((l) => {return l.annotation; }), 'every leg has annotations');
        assert.ok(response.matchings[0].legs.every((l) => {return l.annotation.distance; }), 'every leg has annotations for distance');
        assert.ok(response.matchings[0].legs.every((l) => {return l.annotation.duration; }), 'every leg has annotations for durations');
        assert.equal(undefined, response.matchings[0].geometry);
    });
});

test('match: throws on missing arguments', function(assert) {
    assert.plan(1);
    var osrm = new OSRM(data_path);
    assert.throws(function() { osrm.match({}) },
        /Two arguments required/);
});

test('match: throws on non-object arg', function(assert) {
    assert.plan(1);
    var osrm = new OSRM(data_path);
    assert.throws(function() { osrm.match(null, function(err, response) {}) },
        /First arg must be an object/);
});

test('match: throws on invalid coordinates param', function(assert) {
    assert.plan(4);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: ''
    };
    assert.throws(function() { osrm.match(options, function(err, response) {}) },
        /Coordinates must be an array of \(lon\/lat\) pairs/);
    options.coordinates = [three_test_coordinates[0]];
    assert.throws(function() { osrm.match(options, function(err, response) {}) },
        /At least two coordinates must be provided/);
    options.coordinates = three_test_coordinates[0]
    assert.throws(function() { osrm.match(options, function(err, response) {}) },
        /Coordinates must be an array of \(lon\/lat\) pairs/);
    options.coordinates = [three_test_coordinates[0][0], three_test_coordinates[0][1]];
    assert.throws(function() { osrm.match(options, function(err, response) {}) },
        /Coordinates must be an array of \(lon\/lat\) pairs/);
});

test('match: throws on invalid timestamps param', function(assert) {
    assert.plan(3);
    var osrm = new OSRM(data_path);
    var options = {
        coordinates: three_test_coordinates,
        timestamps: 'timestamps'
    };
    assert.throws(function() { osrm.match(options, function(err, response) {}) },
        /Timestamps must be an array of integers \(or undefined\)/);
    options.timestamps = ['invalid', 'timestamp', 'array'];
    assert.throws(function() { osrm.match(options, function(err, response) {}) },
        /Timestamps array items must be numbers/);
    options.timestamps = [1424684612, 1424684616];
    assert.throws(function() { osrm.match(options, function(err, response) {}) },
        /Timestamp array must have the same size as the coordinates array/);
});
