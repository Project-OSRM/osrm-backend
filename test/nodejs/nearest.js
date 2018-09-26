var OSRM = require('../../');
var test = require('tape');
var data_path = require('./constants').data_path;
var mld_data_path = require('./constants').mld_data_path;
var three_test_coordinates = require('./constants').three_test_coordinates;
var two_test_coordinates = require('./constants').two_test_coordinates;


test('nearest', function(assert) {
    assert.plan(4);
    var osrm = new OSRM(data_path);
    osrm.nearest({
        coordinates: [three_test_coordinates[0]]
    }, function(err, result) {
        assert.ifError(err);
        assert.equal(result.waypoints.length, 1);
        assert.equal(result.waypoints[0].location.length, 2);
        assert.ok(result.waypoints[0].hasOwnProperty('name'));
    });
});

test('nearest', function(assert) {
    assert.plan(5);
    var osrm = new OSRM(data_path);
    osrm.nearest({
        coordinates: [three_test_coordinates[0]]
    }, { format: 'json_buffer' }, function(err, result) {
        assert.ifError(err);
        assert.ok(result instanceof Buffer);
        result = JSON.parse(result);
        assert.equal(result.waypoints.length, 1);
        assert.equal(result.waypoints[0].location.length, 2);
        assert.ok(result.waypoints[0].hasOwnProperty('name'));
    });
});

test('nearest: can ask for multiple nearest pts', function(assert) {
    assert.plan(2);
    var osrm = new OSRM(data_path);
    osrm.nearest({
        coordinates: [three_test_coordinates[0]],
        number: 3
    }, function(err, result) {
        assert.ifError(err);
        assert.equal(result.waypoints.length, 3);
    });
});

test('nearest: throws on invalid args', function(assert) {
    assert.plan(7);
    var osrm = new OSRM(data_path);
    var options = {};
    assert.throws(function() { osrm.nearest(options); },
        /Two arguments required/);
    assert.throws(function() { osrm.nearest(null, function(err, res) {}); },
        /First arg must be an object/);
    options.coordinates = [43.73072];
    assert.throws(function() { osrm.nearest(options, function(err, res) {}); },
        /Coordinates must be an array of /);
    options.coordinates = [three_test_coordinates[0], three_test_coordinates[1]];
    assert.throws(function() { osrm.nearest(options, function(err, res) {}); },
        /Exactly one coordinate pair must be provided/);
    options.coordinates = [three_test_coordinates[0]];
    options.number = 3.14159;
    assert.throws(function() { osrm.nearest(options, function(err, res) {}); },
        /Number must be an integer greater than or equal to 1/);
    options.number = 0;
    assert.throws(function() { osrm.nearest(options, function(err, res) {}); },
        /Number must be an integer greater than or equal to 1/);

    options.number = 1;
    assert.throws(function() { osrm.nearest(options, { format: 'invalid' }, function(err, res) {}); },
        /format must be a string:/);
});

test('nearest: nearest in Monaco without motorways', function(assert) {
    assert.plan(2);
    var osrm = new OSRM({path: mld_data_path, algorithm: 'MLD'});
    var options = {
        coordinates: [two_test_coordinates[0]],
        exclude: ['motorway']
    };
    osrm.nearest(options, function(err, response) {
        assert.ifError(err);
        assert.equal(response.waypoints.length, 1);
    });
});
