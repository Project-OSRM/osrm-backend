var OSRM = require('../../../build/bindings/node/Release/node-osrm.node').OSRM;
var test = require('tape');
var berlin_path = require('./osrm-data-path').data_path;

test('trip: trip in Berlin', function(assert) {
    assert.plan(2);
    var osrm = new OSRM(berlin_path);
    osrm.trip({coordinates: [[13.36761474609375,52.51663871100423],[13.374481201171875,52.506191342034576]]}, function(err, trip) {
        assert.ifError(err);
        for (t = 0; t < trip.trips.length; t++) {
            assert.ok(trip.trips[t].geometry);
        }
    });
});

test('trip: trip with many locations in Berlin', function(assert) {
    assert.plan(4);
    var osrm = new OSRM(berlin_path);
    var opts = {coordinates: [[13.36761474609375,52.51663871100423],[13.374481201171875,52.506191342034576],[13.404693603515625,52.50535544522142],[13.388900756835938,52.50159371284434],[13.386840820312498,52.518727886767266],[13.4088134765625,52.528754547664185],[13.41156005859375,52.51705655410405],[13.420486450195312,52.512042174642346],[13.413619995117188,52.50368360390624],[13.36212158203125,52.504101570196205],[13.35113525390625,52.52248815280757],[13.36761474609375,52.53460237630516],[13.383407592773438,52.53710835019913],[13.392333984375,52.536690697815736],[13.42529296875,52.532931647583325],[13.399200439453125,52.52415927884915],[13.390960693359375,52.51956352925745],[13.375167846679688,52.533349335723294],[13.37860107421875,52.520399155853454],[13.355255126953125,52.52081696319122],[13.385467529296875,52.5143405029259],[13.398857116699219,52.513086884218325],[13.399200439453125,52.50744515744915],[13.409500122070312,52.49783165855699],[13.424949645996094,52.500339730516934],[13.440055847167969,52.50786308797268],[13.428382873535156,52.511624283857785],[13.437652587890625,52.50451953251202],[13.443145751953125,52.5199813445422],[13.431129455566406,52.52520370034151],[13.418426513671875,52.52896341209634],[13.429069519042969,52.517474393230245],[13.418083190917969,52.528127948407935],[13.405036926269531,52.52833681581998],[13.384437561035156,52.53084314728766],[13.374481201171875,52.53084314728766],[13.3978271484375,52.532305107923925],[13.418769836425781,52.526039219655445],[13.441085815429688,52.51642978796417],[13.448638916015625,52.51601193890388],[13.44623565673828,52.50535544522142],[13.430442810058594,52.502638670794546],[13.358688354492188,52.520190250694526],[13.358001708984375,52.531887409851336],[13.367271423339842,52.528545682238736],[13.387870788574219,52.52958999943304],[13.406410217285156,52.53961418106945],[13.399543762207031,52.50556442091497],[13.374824523925781,52.50389258754797],[13.386154174804688,52.51099744023003],[13.40229034423828,52.49657756892365]]
};
    osrm.trip(opts, function(err, trip) {
        assert.ifError(err);
        for (t = 0; t < trip.trips.length; t++) {
            assert.ok(trip.trips[t].geometry);
        }
        assert.equal(opts.coordinates.length, trip.waypoints.length);
        var indexMap = trip.waypoints.map(function(wp, i) {
            return [i, wp.waypoint_index];
        });
        assert.ok(!indexMap.every(function(tuple) { return tuple[0] === tuple[1]; }));
    });
});

test('trip: throws with too few or invalid args', function(assert) {
    assert.plan(2);
    var osrm = new OSRM(berlin_path);
    assert.throws(function() { osrm.trip({coordinates: [[13.43864,52.51993],[13.415852,52.513191]]}) },
        /Two arguments required/);
    assert.throws(function() { osrm.trip(null, function(err, trip) {}) },
        /First arg must be an object/);
});

test('trip: throws with bad params', function(assert) {
    assert.plan(9);
    var osrm = new OSRM(berlin_path);
    assert.throws(function () { osrm.trip({coordinates: []}, function(err) {}) });
    assert.throws(function() { osrm.trip({}, function(err, trip) {}) },
        /Must provide a coordinates property/);
    assert.throws(function() { osrm.trip({
        coordinates: null
    }, function(err, trip) {}) },
        /Coordinates must be an array of \(lon\/lat\) pairs/);
    assert.throws(function() { osrm.trip({
        coordinates: [13.438640, 52.519930]
    }, function(err, trip) {}) },
        /Coordinates must be an array of \(lon\/lat\) pairs/);
    assert.throws(function() { osrm.trip({
        coordinates: [[13.438640], [52.519930]]
    }, function(err, trip) {}) },
        /Coordinates must be an array of \(lon\/lat\) pairs/);
    assert.throws(function() { osrm.trip({
        coordinates: [[13.43864,52.51993],[13.415852,52.513191]],
        hints: null
    }, function(err, trip) {}) },
        /Hints must be an array of strings\/null/);
    var options = {
        coordinates: [[13.43864,52.51993],[13.415852,52.513191]],
        printInstructions: false,
        hints: [13.438640, 52.519930]
    };
    assert.throws(function() { osrm.trip(options, function(err, trip) {}); },
        /Hint must be null or string/);
    options.hints = [null];
    assert.throws(function() { osrm.trip(options, function(err, trip) {}); },
        /Hints array must have the same length as coordinates array/);
    delete options.hints;
    options.geometries = 'false';
    assert.throws(function() { osrm.trip(options, function(err, trip) {}); },
        /'geometries' param must be one of \[polyline, geojson\]/);
});

test('trip: routes Berlin using shared memory', function(assert) {
    assert.plan(2);
    var osrm = new OSRM();
    osrm.trip({coordinates: [[13.43864,52.51993],[13.415852,52.513191]]}, function(err, trip) {
        assert.ifError(err);
            for (t = 0; t < trip.trips.length; t++) {
                assert.ok(trip.trips[t].geometry);
            }
    });
});

test('trip: routes Berlin with hints', function(assert) {
    assert.plan(5);
    var osrm = new OSRM(berlin_path);
    var options = {
        coordinates: [[13.43864,52.51993],[13.415852,52.513191]],
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

test('trip: trip through Berlin with geometry compression', function(assert) {
    assert.plan(2);
    var osrm = new OSRM(berlin_path);
    var options = {
        coordinates: [[13.43864,52.51993],[13.415852,52.513191]]
    };
    osrm.trip(options, function(err, trip) {
        assert.ifError(err);
        for (t = 0; t < trip.trips.length; t++) {
            assert.equal('string', typeof trip.trips[t].geometry);
        }
    });
});

test('trip: trip through Berlin without geometry compression', function(assert) {
    assert.plan(2);
    var osrm = new OSRM(berlin_path);
    var options = {
        coordinates: [[13.43864,52.51993],[13.415852,52.513191]],
        geometries: 'geojson'
    };
    osrm.trip(options, function(err, trip) {
        assert.ifError(err);
        for (t = 0; t < trip.trips.length; t++) {
            assert.ok(Array.isArray(trip.trips[t].geometry.coordinates));
        }
    });
});

test('trip: trip through Berlin with options', function(assert) {
    assert.plan(5);
    var osrm = new OSRM(berlin_path);
    var options = {
        coordinates: [[13.43864,52.51993],[13.415852,52.513191]],
        steps: false,
        overview: 'false'
    };
    osrm.trip(options, function(err, trip) {
        assert.ifError(err);
        assert.equal(trip.trips.length, 1);
        for (t = 0; t < trip.trips.length; t++) {
            assert.ok(trip.trips[t]);
            assert.notOk(trip.trips[t].legs.some(function(l) { return l.steps.length; }))
            assert.notOk(trip.trips[t].geometry);
        }
    });
});

test('trip: routes Berlin with null hints', function(assert) {
    assert.plan(1);
    var osrm = new OSRM(berlin_path);
    var options = {
        coordinates: [[13.43864,52.51993],[13.415852,52.513191]],
        printInstructions: false,
        hints: [null, null]
    };
    osrm.trip(options, function(err, second) {
        assert.ifError(err);
    });
});
