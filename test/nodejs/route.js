// Test route service functionality with turn-by-turn directions
import OSRM from '../../build/nodejs/lib/index.js';
import test from 'tape';
import { data_path as monaco_path, mld_data_path as monaco_mld_path, three_test_coordinates, two_test_coordinates } from './constants.js';
import flatbuffers from 'flatbuffers';
import { osrm } from '../../features/support/fbresult_generated.js';

const FBResult = osrm.engine.api.fbresult.FBResult;

test('route: routes Monaco and can return result in flatbuffers', (assert) => {
  assert.plan(5);
  const osrm = new OSRM(monaco_path);
  osrm.route({coordinates: two_test_coordinates, format: 'flatbuffers'}, (err, result) => {
    assert.ifError(err);
    assert.ok(result instanceof Buffer);
    const fb = FBResult.getRootAsFBResult(new flatbuffers.ByteBuffer(result));
    assert.equals(fb.waypointsLength(), 2);
    assert.equals(fb.routesLength(), 1);
    assert.ok(fb.routes(0).polyline);
  });
});

test('route: routes Monaco and can return result in flatbuffers if output format is passed explicitly', (assert) => {
  assert.plan(5);
  const osrm = new OSRM(monaco_path);
  osrm.route({coordinates: two_test_coordinates, format: 'flatbuffers'}, {output: 'buffer'}, (err, result) => {
    assert.ifError(err);
    assert.ok(result instanceof Buffer);
    const buf = new flatbuffers.ByteBuffer(result);
    const fb = FBResult.getRootAsFBResult(buf);
    assert.equals(fb.waypointsLength(), 2);
    assert.equals(fb.routesLength(), 1);
    assert.ok(fb.routes(0).polyline);
  });
});

test('route: throws error if required output is object in flatbuffers format', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(monaco_path);
  assert.throws(() => {
    osrm.route({coordinates: two_test_coordinates, format: 'flatbuffers'}, {format: 'object'}, (err, result) => {});
  });
});

test('route: throws error if required output is json_buffer in flatbuffers format', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(monaco_path);
  assert.throws(() => {
    osrm.route({coordinates: two_test_coordinates, format: 'flatbuffers'}, {format: 'json_buffer'}, (err, result) => {});
  });
});


test('route: routes Monaco', (assert) => {
  assert.plan(5);
  const osrm = new OSRM(monaco_path);
  osrm.route({coordinates: two_test_coordinates}, (err, route) => {
    assert.ifError(err);
    assert.ok(route.waypoints);
    assert.ok(route.routes);
    assert.ok(route.routes.length);
    assert.ok(route.routes[0].geometry);
  });
});

test('route: routes Monaco on MLD', (assert) => {
  assert.plan(5);
  const osrm = new OSRM({path: monaco_mld_path, algorithm: 'MLD'});
  osrm.route({coordinates: [[13.43864,52.51993],[13.415852,52.513191]]}, (err, route) => {
    assert.ifError(err);
    assert.ok(route.waypoints);
    assert.ok(route.routes);
    assert.ok(route.routes.length);
    assert.ok(route.routes[0].geometry);
  });
});

test('route: throws with too few or invalid args', (assert) => {
  assert.plan(4);
  const osrm = new OSRM(monaco_path);
  assert.throws(() => { osrm.route({coordinates: two_test_coordinates}); },
    /Two arguments required/);
  assert.throws(() => { osrm.route(null, (err, route) => {}); },
    /First arg must be an object/);
  assert.throws(() => { osrm.route({coordinates: two_test_coordinates}, true);},
    /last argument must be a callback function/);
  assert.throws(() => { osrm.route({coordinates: two_test_coordinates}, { format: 'invalid' }, (err, route) => {});},
    /format must be a string:/);
});

test('route: provides no alternatives by default, but when requested it may (not guaranteed)', (assert) => {
  assert.plan(9);
  const osrm = new OSRM(monaco_path);
  const options = {coordinates: two_test_coordinates};

  osrm.route(options, (err, route) => {
    assert.ifError(err);
    assert.ok(route.routes);
    assert.equal(route.routes.length, 1);
  });
  options.alternatives = true;
  osrm.route(options, (err, route) => {
    assert.ifError(err);
    assert.ok(route.routes);
    assert.ok(route.routes.length >= 1);
  });
  options.alternatives = 3;
  osrm.route(options, (err, route) => {
    assert.ifError(err);
    assert.ok(route.routes);
    assert.ok(route.routes.length >= 1);
  });
});

test('route: throws with bad params', (assert) => {
  assert.plan(11);
  const osrm = new OSRM(monaco_path);
  assert.throws(() => { osrm.route({coordinates: []}, (err) => {}); });
  assert.throws(() => { osrm.route({}, (err, route) => {}); },
    /Must provide a coordinates property/);
  assert.throws(() => { osrm.route({coordinates: null}, (err, route) => {}); },
    /Coordinates must be an array of \(lon\/lat\) pairs/);
  assert.throws(() => { osrm.route({coordinates: [[three_test_coordinates[0]], [three_test_coordinates[1]]]}, (err, route) => {}); },
    /Coordinates must be an array of \(lon\/lat\) pairs/);
  assert.throws(() => { osrm.route({coordinates: [[true, 'stringish'], three_test_coordinates[1]]}, (err, route) => {}); },
    /Each member of a coordinate pair must be a number/);
  assert.throws(() => { osrm.route({coordinates: [[213.43864,252.51993],[413.415852,552.513191]]}, (err, route) => {}); },
    /Lng\/Lat coordinates must be within world bounds \(-180 < lng < 180, -90 < lat < 90\)/);
  assert.throws(() => { osrm.route({coordinates: [[13.438640], [52.519930]]}, (err, route) => {}); },
    /Coordinates must be an array of \(lon\/lat\) pairs/);
  assert.throws(() => { osrm.route({coordinates: two_test_coordinates, hints: null}, (err, route) => {}); },
    /Hints must be an array of strings\/null/);
  assert.throws(() => { osrm.route({coordinates: two_test_coordinates, steps: null}, (err, route) => {}); });
  assert.throws(() => { osrm.route({coordinates: two_test_coordinates, annotations: null}, (err, route) => {}); });
  const options = {
    coordinates: two_test_coordinates,
    alternateRoute: false,
    hints: three_test_coordinates[0]
  };
  assert.throws(() => { osrm.route(options, (err, route) => {}); },
    /Hint must be null or string/);
});

test('route: routes Monaco using shared memory', (assert) => {
  assert.plan(2);
  const osrm = new OSRM();
  osrm.route({coordinates: two_test_coordinates}, (err, route) => {
    assert.ifError(err);
    assert.ok(Array.isArray(route.routes));
  });
});

test('route: routes Monaco with geometry compression', (assert) => {
  assert.plan(2);
  const osrm = new OSRM(monaco_path);
  const options = {
    coordinates: two_test_coordinates,
  };
  osrm.route(options, (err, route) => {
    assert.ifError(err);
    assert.equal('string', typeof route.routes[0].geometry);
  });
});

test('route: routes Monaco without geometry compression', (assert) => {
  assert.plan(4);
  const osrm = new OSRM(monaco_path);
  const options = {
    coordinates: two_test_coordinates,
    geometries: 'geojson'
  };
  osrm.route(options, (err, route) => {
    assert.ifError(err);
    assert.ok(Array.isArray(route.routes));
    assert.ok(Array.isArray(route.routes[0].geometry.coordinates));
    assert.equal(route.routes[0].geometry.type, 'LineString');
  });
});

test('Test polyline6 geometries option', (assert) => {
  assert.plan(6);
  const osrm = new OSRM(monaco_path);
  const options = {
    coordinates: two_test_coordinates,
    continue_straight: false,
    overview: 'false',
    geometries: 'polyline6',
    steps: true
  };
  osrm.route(options, (err, first) => {
    assert.ifError(err);
    assert.ok(first.routes);
    assert.equal(first.routes.length, 1);
    assert.notOk(first.routes[0].geometry);
    assert.ok(first.routes[0].legs[0]);
    assert.equal(typeof first.routes[0].legs[0].steps[0].geometry, 'string');
  });
});

test('route: routes Monaco with speed annotations options', (assert) => {
  assert.plan(17);
  const osrm = new OSRM(monaco_path);
  const options = {
    coordinates: two_test_coordinates,
    continue_straight: false,
    overview: 'false',
    geometries: 'polyline',
    steps: true,
    annotations: ['speed']
  };
  osrm.route(options, (err, first) => {
    assert.ifError(err);
    assert.ok(first.routes);
    assert.ok(first.routes[0].legs.every((l) => { return Array.isArray(l.steps) && l.steps.length > 0; }));
    assert.equal(first.routes.length, 1);
    assert.notOk(first.routes[0].geometry);
    assert.ok(first.routes[0].legs[0]);
    assert.ok(first.routes[0].legs.every(l => { return l.steps.length > 0; }), 'every leg has steps');
    assert.ok(first.routes[0].legs.every(l => { return l.annotation;}), 'every leg has annotations');
    assert.ok(first.routes[0].legs.every(l => { return l.annotation.speed;}), 'every leg has annotations for speed');
    assert.notOk(first.routes[0].legs.every(l => { return l.annotation.weight; }), 'has no annotations for weight');
    assert.notOk(first.routes[0].legs.every(l => { return l.annotation.datasources; }), 'has no annotations for datasources');
    assert.notOk(first.routes[0].legs.every(l => { return l.annotation.duration; }), 'has no annotations for duration');
    assert.notOk(first.routes[0].legs.every(l => { return l.annotation.distance; }), 'has no annotations for distance');
    assert.notOk(first.routes[0].legs.every(l => { return l.annotation.nodes; }), 'has no annotations for nodes');

    options.overview = 'full';
    osrm.route(options, (err, full) => {
      assert.ifError(err);
      options.overview = 'simplified';
      osrm.route(options, (err, simplified) => {
        assert.ifError(err);
        assert.notEqual(full.routes[0].geometry, simplified.routes[0].geometry);
      });
    });
  });
});

test('route: routes Monaco with several (duration, distance, nodes) annotations options', (assert) => {
  assert.plan(17);
  const osrm = new OSRM(monaco_path);
  const options = {
    coordinates: two_test_coordinates,
    continue_straight: false,
    overview: 'false',
    geometries: 'polyline',
    steps: true,
    annotations: ['duration', 'distance', 'nodes']
  };
  osrm.route(options, (err, first) => {
    assert.ifError(err);
    assert.ok(first.routes);
    assert.ok(first.routes[0].legs.every((l) => { return Array.isArray(l.steps) && l.steps.length > 0; }));
    assert.equal(first.routes.length, 1);
    assert.notOk(first.routes[0].geometry);
    assert.ok(first.routes[0].legs[0]);
    assert.ok(first.routes[0].legs.every(l => { return l.steps.length > 0; }), 'every leg has steps');
    assert.ok(first.routes[0].legs.every(l => { return l.annotation;}), 'every leg has annotations');
    assert.ok(first.routes[0].legs.every(l => { return l.annotation.distance;}), 'every leg has annotations for distance');
    assert.ok(first.routes[0].legs.every(l => { return l.annotation.duration;}), 'every leg has annotations for durations');
    assert.ok(first.routes[0].legs.every(l => { return l.annotation.nodes;}), 'every leg has annotations for nodes');
    assert.notOk(first.routes[0].legs.every(l => { return l.annotation.weight; }), 'has no annotations for weight');
    assert.notOk(first.routes[0].legs.every(l => { return l.annotation.datasources; }), 'has no annotations for datasources');
    assert.notOk(first.routes[0].legs.every(l => { return l.annotation.speed; }), 'has no annotations for speed');

    options.overview = 'full';
    osrm.route(options, (err, full) => {
      assert.ifError(err);
      options.overview = 'simplified';
      osrm.route(options, (err, simplified) => {
        assert.ifError(err);
        assert.notEqual(full.routes[0].geometry, simplified.routes[0].geometry);
      });
    });
  });
});

test('route: routes Monaco with options', (assert) => {
  assert.plan(17);
  const osrm = new OSRM(monaco_path);
  const options = {
    coordinates: two_test_coordinates,
    continue_straight: false,
    overview: 'false',
    geometries: 'polyline',
    steps: true,
    annotations: true
  };
  osrm.route(options, (err, first) => {
    assert.ifError(err);
    assert.ok(first.routes);
    assert.ok(first.routes[0].legs.every((l) => { return Array.isArray(l.steps) && l.steps.length > 0; }));
    assert.equal(first.routes.length, 1);
    assert.notOk(first.routes[0].geometry);
    assert.ok(first.routes[0].legs[0]);
    assert.ok(first.routes[0].legs.every(l => { return l.steps.length > 0; }), 'every leg has steps');
    assert.ok(first.routes[0].legs.every(l => { return l.annotation;}), 'every leg has annotations');
    assert.ok(first.routes[0].legs.every(l => { return l.annotation.distance;}), 'every leg has annotations for distance');
    assert.ok(first.routes[0].legs.every(l => { return l.annotation.duration;}), 'every leg has annotations for durations');
    assert.ok(first.routes[0].legs.every(l => { return l.annotation.nodes;}), 'every leg has annotations for nodes');
    assert.ok(first.routes[0].legs.every(l => { return l.annotation.weight; }), 'every leg has annotations for weight');
    assert.ok(first.routes[0].legs.every(l => { return l.annotation.datasources; }), 'every leg has annotations for datasources');
    assert.ok(first.routes[0].legs.every(l => { return l.annotation.speed; }), 'every leg has annotations for speed');

    options.overview = 'full';
    osrm.route(options, (err, full) => {
      assert.ifError(err);
      options.overview = 'simplified';
      osrm.route(options, (err, simplified) => {
        assert.ifError(err);
        assert.notEqual(full.routes[0].geometry, simplified.routes[0].geometry);
      });
    });
  });
});

test('route: routes Monaco with options', (assert) => {
  assert.plan(11);
  const osrm = new OSRM(monaco_path);
  const options = {
    coordinates: two_test_coordinates,
    continue_straight: false,
    overview: 'false',
    geometries: 'polyline',
    steps: true,
    annotations: true
  };
  osrm.route(options, (err, first) => {
    assert.ifError(err);
    assert.ok(first.routes);
    assert.ok(first.routes[0].legs.every((l) => { return Array.isArray(l.steps) && l.steps.length > 0; }));
    assert.equal(first.routes.length, 1);
    assert.notOk(first.routes[0].geometry);
    assert.ok(first.routes[0].legs[0]);
    assert.ok(first.routes[0].legs.every(l => { return l.steps.length > 0; }), 'every leg has steps');
    assert.ok(first.routes[0].legs.every(l => { return l.annotation;}), 'every leg has annotations');

    options.overview = 'full';
    osrm.route(options, (err, full) => {
      assert.ifError(err);
      options.overview = 'simplified';
      osrm.route(options, (err, simplified) => {
        assert.ifError(err);
        assert.notEqual(full.routes[0].geometry, simplified.routes[0].geometry);
      });
    });
  });
});

test('route: invalid route options', (assert) => {
  assert.plan(8);
  const osrm = new OSRM(monaco_path);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    continue_straight: []
  }, (err, route) => {}); },
  /must be boolean/);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    alternatives: []
  }, (err, route) => {}); },
  /must be boolean/);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    geometries: true
  }, (err, route) => {}); },
  /Geometries must be a string: \[polyline, polyline6, geojson\]/);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    overview: false
  }, (err, route) => {}); },
  /Overview must be a string: \[simplified, full, false\]/);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    overview: false
  }, (err, route) => {}); },
  /Overview must be a string: \[simplified, full, false\]/);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    overview: 'maybe'
  }, (err, route) => {}); },
  /'overview' param must be one of \[simplified, full, false\]/);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    geometries: 'maybe'
  }, (err, route) => {}); },
  /'geometries' param must be one of \[polyline, polyline6, geojson\]/);
  assert.throws(() => { osrm.route({
    coordinates: [[NaN, -NaN],[Infinity, -Infinity]]
  }, (err, route) => {}); },
  /Lng\/Lat coordinates must be valid numbers/);
});

test('route: integer bearing values no longer supported', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(monaco_path);
  const options = {
    coordinates: two_test_coordinates,
    bearings: [200, 250],
  };
  assert.throws(() => { osrm.route(options, (err, route) => {}); },
    /Bearing must be an array of \[bearing, range\] or null/);
});

test('route: valid bearing values', (assert) => {
  assert.plan(4);
  const osrm = new OSRM(monaco_path);
  const options = {
    coordinates: two_test_coordinates,
    bearings: [[200, 180], [250, 180]],
  };
  osrm.route(options, (err, route) => {
    assert.ifError(err);
    assert.ok(route.routes[0]);
  });
  options.bearings = [null, [360, 180]];
  osrm.route(options, (err, route) => {
    assert.ifError(err);
    assert.ok(route.routes[0]);
  });
});

test('route: invalid bearing values', (assert) => {
  assert.plan(6);
  const osrm = new OSRM(monaco_path);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    bearings: [[400, 180], [-250, 180]],
  }, (err, route) => {}); },
  /Bearing values need to be in range 0..360, 0..180/);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    bearings: [[200], [250, 180]],
  }, (err, route) => {}); },
  /Bearing must be an array of/);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    bearings: [[400, 109], [100, 720]],
  }, (err, route) => {}); },
  /Bearing values need to be in range 0..360, 0..180/);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    bearings: 400,
  }, (err, route) => {}); },
  /Bearings must be an array of arrays of numbers/);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    bearings: [[100, 100]],
  }, (err, route) => {}); },
  /Bearings array must have the same length as coordinates array/);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    bearings: [Infinity, Infinity],
  }, (err, route) => {}); },
  /Bearing must be an array of \[bearing, range\] or null/);
});

test('route: routes Monaco with hints', (assert) => {
  assert.plan(5);
  const osrm = new OSRM(monaco_path);
  const options = {
    coordinates: two_test_coordinates,
  };
  osrm.route(options, (err, first) => {
    assert.ifError(err);
    assert.ok(first.waypoints);
    const hints = first.waypoints.map((wp) => { return wp.hint; });
    assert.ok(hints.every((h) => { return typeof h === 'string'; }));

    options.hints = hints;

    osrm.route(options, (err, second) => {
      assert.ifError(err);
      assert.deepEqual(first, second);
    });
  });
});

test('route: routes Monaco with null hints', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(monaco_path);
  const options = {
    coordinates: two_test_coordinates,
    hints: [null, null]
  };
  osrm.route(options, (err, route) => {
    assert.ifError(err);
  });
});

test('route: throws on bad hints', (assert) => {
  assert.plan(2);
  const osrm = new OSRM(monaco_path);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    hints: ['', '']
  }, (err, route) => {});}, /Hint cannot be an empty string/);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    hints: [null]
  }, (err, route) => {});}, /Hints array must have the same length as coordinates array/);
});

test('route: routes Monaco with valid radius values', (assert) => {
  assert.plan(3);
  const osrm = new OSRM(monaco_path);
  const options = {
    coordinates: two_test_coordinates,
    radiuses: [100, 100]
  };
  osrm.route(options, (err, route) => {
    assert.ifError(err);
  });
  options.radiuses = [null, null];
  osrm.route(options, (err, route) => {
    assert.ifError(err);
  });
  options.radiuses = [100, null];
  osrm.route(options, (err, route) => {
    assert.ifError(err);
  });
});

test('route: throws on bad radiuses', (assert) => {
  assert.plan(3);
  const osrm = new OSRM(monaco_path);
  const options = {
    coordinates: two_test_coordinates,
    radiuses: [10, 10]
  };
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    radiuses: 10
  }, (err, route) => {}); },
  /Radiuses must be an array of non-negative doubles or null/);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    radiuses: ['magic', 'numbers']
  }, (err, route) => {}); },
  /Radius must be non-negative double or null/);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    radiuses: [10]
  }, (err, route) => {}); },
  /Radiuses array must have the same length as coordinates array/);
});

test('route: routes Monaco with valid approaches values', (assert) => {
  assert.plan(4);
  const osrm = new OSRM(monaco_path);
  const options = {
    coordinates: two_test_coordinates,
    approaches: [null, 'curb']
  };
  osrm.route(options, (err, route) => {
    assert.ifError(err);
  });
  options.approaches = [null, null];
  osrm.route(options, (err, route) => {
    assert.ifError(err);
  });
  options.approaches = ['opposite', 'opposite'];
  osrm.route(options, (err, route) => {
    assert.ifError(err);
  });
  options.approaches = ['unrestricted', null];
  osrm.route(options, (err, route) => {
    assert.ifError(err);
  });
});

test('route: throws on bad approaches', (assert) => {
  assert.plan(4);
  const osrm = new OSRM(monaco_path);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    approaches: 10
  }, (err, route) => {}); },
  /Approaches must be an arrays of strings/);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    approaches: ['curb']
  }, (err, route) => {}); },
  /Approaches array must have the same length as coordinates array/);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    approaches: ['curb', 'test']
  }, (err, route) => {}); },
  /'approaches' param must be one of \[curb, opposite, unrestricted\]/);
  assert.throws(() => { osrm.route({
    coordinates: two_test_coordinates,
    approaches: [10, 15]
  }, (err, route) => {}); },
  /Approach must be a string: \[curb, opposite, unrestricted\] or null/);
});

test('route: routes Monaco with custom limits on MLD', (assert) => {
  assert.plan(2);
  const osrm = new OSRM({
    path: monaco_mld_path,
    algorithm: 'MLD',
    max_alternatives: 10,
  });
  osrm.route({coordinates: two_test_coordinates, alternatives: 10}, (err, route) => {
    assert.ifError(err);
    assert.ok(Array.isArray(route.routes));
  });
});

test('route:  in Monaco with custom limits on MLD', (assert) => {
  assert.plan(1);
  const osrm = new OSRM({
    path: monaco_mld_path,
    algorithm: 'MLD',
    max_alternatives: 10,
  });
  osrm.route({coordinates: two_test_coordinates, alternatives: 11}, (err, route) => {
    console.log(err);
    assert.equal(err.message, 'TooBig');
  });
});

test('route: route in Monaco without motorways', (assert) => {
  assert.plan(3);
  const osrm = new OSRM({path: monaco_mld_path, algorithm: 'MLD'});
  const options = {
    coordinates: two_test_coordinates,
    exclude: ['motorway']
  };
  osrm.route(options, (err, response) => {
    assert.ifError(err);
    assert.equal(response.waypoints.length, 2);
    assert.equal(response.routes.length, 1);
  });
});


test('route: throws on invalid waypoints values needs at least two', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(monaco_path);
  const options = {
    steps: true,
    coordinates: three_test_coordinates,
    waypoints: [0]
  };
  assert.throws(() => { osrm.route(options, (err, response) => { }); },
    'At least two waypoints must be provided');
});

test('route: throws on invalid waypoints values, needs first and last coordinate indices', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(monaco_path);
  const options = {
    steps: true,
    coordinates: three_test_coordinates,
    waypoints: [1, 2]
  };
  assert.throws(() => { osrm.route(options, (err, response) => { console.log(err); }); },
    'First and last waypoints values must correspond to first and last coordinate indices');
});

test('route: throws on invalid waypoints values, order matters', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(monaco_path);
  const options = {
    steps: true,
    coordinates: three_test_coordinates,
    waypoints: [2, 0]
  };
  assert.throws(() => { osrm.route(options, (err, response) => { console.log(err); }); },
    'First and last waypoints values must correspond to first and last coordinate indices');
});

test('route: throws on invalid waypoints values, waypoints must correspond with a coordinate index', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(monaco_path);
  const options = {
    steps: true,
    coordinates: three_test_coordinates,
    waypoints: [0, 3, 2]
  };
  assert.throws(() => { osrm.route(options, (err, response) => { console.log(err); }); },
    'Waypoints must correspond with the index of an input coordinate');
});

test('route: throws on invalid waypoints values, waypoints must be an array', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(monaco_path);
  const options = {
    steps: true,
    coordinates: three_test_coordinates,
    waypoints: 'string'
  };
  assert.throws(() => { osrm.route(options, (err, response) => { console.log(err); }); },
    'Waypoints must be an array of integers corresponding to the input coordinates.');
});

test('route: throws on invalid waypoints values, waypoints must be an array of integers', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(monaco_path);
  const options = {
    steps: true,
    coordinates: three_test_coordinates,
    waypoints: [0,1,'string']
  };
  assert.throws(() => { osrm.route(options, (err, response) => { console.log(err); }); },
    'Waypoint values must be an array of integers');
});

test('route: throws on invalid waypoints values, waypoints must be an array of integers in increasing order', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(monaco_path);
  const options = {
    steps: true,
    coordinates: three_test_coordinates.concat(three_test_coordinates),
    waypoints: [0,2,1,5]
  };
  assert.throws(() => { osrm.route(options, (err, response) => { console.error(`response: ${response}`); console.error(`error: ${err}`); }); },
    /Waypoints must be supplied in increasing order/);
});

test('route: throws on invalid snapping values', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(monaco_path);
  const options = {
    steps: true,
    coordinates: three_test_coordinates.concat(three_test_coordinates),
    snapping: 'zing'
  };
  assert.throws(() => { osrm.route(options, (err, response) => { console.error(`response: ${response}`); console.error(`error: ${err}`); }); },
    /'snapping' param must be one of \[default, any\]/);
});

test('route: snapping parameter passed through OK', (assert) => {
  assert.plan(2);
  const osrm = new OSRM(monaco_path);
  osrm.route({snapping: 'any', coordinates: [[7.448205209414596,43.754001097311544],[7.447122039202185,43.75306156811368]]}, (err, route) => {
    assert.ifError(err);
    assert.equal(Math.round(route.routes[0].distance * 10), 1315); // Round it to nearest 0.1m to eliminate floating point comparison error
  });
});

test('route: throws on disabled geometry', (assert) => {
  assert.plan(1);
  const osrm = new OSRM({'path': monaco_path, 'disable_feature_dataset': ['ROUTE_GEOMETRY']});
  const options = {
    coordinates: three_test_coordinates,
  };
  osrm.route(options, (err, route) => {
    console.log(err);
    assert.match(err.message, /DisabledDatasetException/);
  });
});

test('route: ok on disabled geometry', (assert) => {
  assert.plan(2);
  const osrm = new OSRM({'path': monaco_path, 'disable_feature_dataset': ['ROUTE_GEOMETRY']});
  const options = {
    steps: false,
    overview: 'false',
    annotations: false,
    skip_waypoints: true,
    coordinates: three_test_coordinates,
  };
  osrm.route(options, (err, response) => {
    assert.ifError(err);
    assert.equal(response.routes.length, 1);
  });
});

test('route: throws on disabled steps', (assert) => {
  assert.plan(1);
  const osrm = new OSRM({'path': monaco_path, 'disable_feature_dataset': ['ROUTE_STEPS']});
  const options = {
    steps: true,
    coordinates: three_test_coordinates,
  };
  osrm.route(options, (err, route) => {
    console.log(err);
    assert.match(err.message, /DisabledDatasetException/);
  });
});

test('route: ok on disabled steps', (assert) => {
  assert.plan(8);
  const osrm = new OSRM({'path': monaco_path, 'disable_feature_dataset': ['ROUTE_STEPS']});
  const options = {
    steps: false,
    overview: 'simplified',
    annotations: true,
    coordinates: three_test_coordinates,
  };
  osrm.route(options, (err, response) => {
    assert.ifError(err);
    assert.ok(response.waypoints);
    assert.ok(response.routes);
    assert.equal(response.routes.length, 1);
    assert.ok(response.routes[0].geometry, 'the route has geometry');
    assert.ok(response.routes[0].legs, 'the route has legs');
    assert.notok(response.routes[0].legs.every(l => { return l.steps.length > 0; }), 'every leg has steps');
    assert.ok(response.routes[0].legs.every(l => { return l.annotation;}), 'every leg has annotations');
  });
});
