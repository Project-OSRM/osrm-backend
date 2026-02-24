// Test map matching service functionality for GPS trace alignment
import OSRM from '../../build/nodejs/lib/index.js';
import test from 'tape';
import { data_path, mld_data_path, three_test_coordinates, two_test_coordinates } from './constants.js';
import flatbuffers from 'flatbuffers';
import { osrm } from '../../features/support/fbresult_generated.js';

const FBResult = osrm.engine.api.fbresult.FBResult;


test('match: match in Monaco with flatbuffers format', (assert) => {
  assert.plan(2);
  const osrm = new OSRM(data_path);
  const options = {
    coordinates: three_test_coordinates,
    timestamps: [1424684612, 1424684616, 1424684620],
    format: 'flatbuffers'
  };
  osrm.match(options, (err, response) => {
    assert.ifError(err);
    const fb = FBResult.getRootAsFBResult(new flatbuffers.ByteBuffer(response));
    assert.equal(fb.routesLength(), 1);
  });
});

test('match: match in Monaco', (assert) => {
  assert.plan(5);
  const osrm = new OSRM(data_path);
  const options = {
    coordinates: three_test_coordinates,
    timestamps: [1424684612, 1424684616, 1424684620]
  };
  osrm.match(options, (err, response) => {
    assert.ifError(err);
    assert.equal(response.matchings.length, 1);
    assert.ok(response.matchings.every((m) => {
      return !!m.distance && !!m.duration && Array.isArray(m.legs) && !!m.geometry && m.confidence > 0;
    }));
    assert.equal(response.tracepoints.length, 3);
    assert.ok(response.tracepoints.every((t) => {
      return !!t.hint && !isNaN(t.matchings_index) && !isNaN(t.waypoint_index) && !!t.name;
    }));
  });
});

test('match: match in Monaco returning a buffer', (assert) => {
  assert.plan(6);
  const osrm = new OSRM(data_path);
  const options = {
    coordinates: three_test_coordinates,
    timestamps: [1424684612, 1424684616, 1424684620]
  };
  osrm.match(options, { format: 'json_buffer' }, (err, response) => {
    assert.ifError(err);
    assert.ok(response instanceof Buffer);
    response = JSON.parse(response);
    assert.equal(response.matchings.length, 1);
    assert.ok(response.matchings.every((m) => {
      return !!m.distance && !!m.duration && Array.isArray(m.legs) && !!m.geometry && m.confidence > 0;
    }));
    assert.equal(response.tracepoints.length, 3);
    assert.ok(response.tracepoints.every((t) => {
      return !!t.hint && !isNaN(t.matchings_index) && !isNaN(t.waypoint_index) && !!t.name;
    }));
  });
});

test('match: match in Monaco without timestamps', (assert) => {
  assert.plan(3);
  const osrm = new OSRM(data_path);
  const options = {
    coordinates: three_test_coordinates
  };
  osrm.match(options, (err, response) => {
    assert.ifError(err);
    assert.equal(response.tracepoints.length, 3);
    assert.equal(response.matchings.length, 1);
  });
});

test('match: match in Monaco without geometry compression', (assert) => {
  assert.plan(4);
  const osrm = new OSRM(data_path);
  const options = {
    coordinates: three_test_coordinates,
    geometries: 'geojson'
  };
  osrm.match(options, (err, response) => {
    assert.ifError(err);
    assert.equal(response.matchings.length, 1);
    assert.ok(response.matchings[0].geometry instanceof Object);
    assert.ok(Array.isArray(response.matchings[0].geometry.coordinates));
  });
});

test('match: match in Monaco with geometry compression', (assert) => {
  assert.plan(3);
  const osrm = new OSRM(data_path);
  const options = {
    coordinates: three_test_coordinates,
  };
  osrm.match(options, (err, response) => {
    assert.ifError(err);
    assert.equal(response.matchings.length, 1);
    assert.equal('string', typeof response.matchings[0].geometry);
  });
});

test('match: match in Monaco with speed annotations options', (assert) => {
  assert.plan(12);
  const osrm = new OSRM(data_path);
  const options = {
    coordinates: three_test_coordinates,
    timestamps: [1424684612, 1424684616, 1424684620],
    radiuses: [4.07, 4.07, 4.07],
    steps: true,
    annotations: ['speed'],
    overview: 'false',
    geometries: 'geojson'
  };
  osrm.match(options, (err, response) => {
    assert.ifError(err);
    assert.equal(response.matchings.length, 1);
    assert.ok(response.matchings[0].confidence > 0, 'has confidence');
    assert.ok(response.matchings[0].legs.every((l) => {return l.steps.length > 0; }), 'every leg has steps');
    assert.ok(response.matchings[0].legs.every((l) => {return l.annotation; }), 'every leg has annotations');
    assert.ok(response.matchings[0].legs.every((l) => {return l.annotation.speed; }), 'every leg has annotations for speed');
    assert.notOk(response.matchings[0].legs.every((l) => {return l.annotation.weight; }), 'has no annotations for weight');
    assert.notOk(response.matchings[0].legs.every((l) => {return l.annotation.datasources; }), 'has no annotations for datasources');
    assert.notOk(response.matchings[0].legs.every((l) => {return l.annotation.duration; }), 'has no annotations for duration');
    assert.notOk(response.matchings[0].legs.every((l) => {return l.annotation.distance; }), 'has no annotations for distance');
    assert.notOk(response.matchings[0].legs.every((l) => {return l.annotation.nodes; }), 'has no annotations for nodes');
    assert.equal(undefined, response.matchings[0].geometry);
  });
});


test('match: match in Monaco with several (duration, distance, nodes) annotations options', (assert) => {
  assert.plan(12);
  const osrm = new OSRM(data_path);
  const options = {
    timestamps: [1424684612, 1424684616, 1424684620],
    coordinates: three_test_coordinates,
    timestamps: [1424684612, 1424684616, 1424684620],
    radiuses: [4.07, 4.07, 4.07],
    steps: true,
    annotations: ['duration','distance','nodes'],
    overview: 'false',
    geometries: 'geojson'
  };
  osrm.match(options, (err, response) => {
    assert.ifError(err);
    assert.equal(response.matchings.length, 1);
    assert.ok(response.matchings[0].confidence > 0, 'has confidence');
    assert.ok(response.matchings[0].legs.every((l) => {return l.steps.length > 0; }), 'every leg has steps');
    assert.ok(response.matchings[0].legs.every((l) => {return l.annotation; }), 'every leg has annotations');
    assert.ok(response.matchings[0].legs.every((l) => {return l.annotation.distance; }), 'every leg has annotations for distance');
    assert.ok(response.matchings[0].legs.every((l) => {return l.annotation.duration; }), 'every leg has annotations for durations');
    assert.ok(response.matchings[0].legs.every((l) => {return l.annotation.nodes; }), 'every leg has annotations for nodes');
    assert.notOk(response.matchings[0].legs.every((l) => {return l.annotation.weight; }), 'has no annotations for weight');
    assert.notOk(response.matchings[0].legs.every((l) => {return l.annotation.datasources; }), 'has no annotations for datasources');
    assert.notOk(response.matchings[0].legs.every((l) => {return l.annotation.speed; }), 'has no annotations for speed');
    assert.equal(undefined, response.matchings[0].geometry);
  });
});

test('match: match in Monaco with all options', (assert) => {
  assert.plan(8);
  const osrm = new OSRM(data_path);
  const options = {
    coordinates: three_test_coordinates,
    timestamps: [1424684612, 1424684616, 1424684620],
    radiuses: [4.07, 4.07, 4.07],
    steps: true,
    annotations: true,
    overview: 'false',
    geometries: 'geojson',
    gaps: 'split',
    tidy: false
  };
  osrm.match(options, (err, response) => {
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

test('match: throws on missing arguments', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(data_path);
  assert.throws(() => { osrm.match({}); },
    /Two arguments required/);
});

test('match: throws on non-object arg', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(data_path);
  assert.throws(() => { osrm.match(null, (err, response) => {}); },
    /First arg must be an object/);
});

test('match: throws on invalid coordinates param', (assert) => {
  assert.plan(4);
  const osrm = new OSRM(data_path);
  const options = {
    coordinates: ''
  };
  assert.throws(() => { osrm.match(options, (err, response) => {}); },
    /Coordinates must be an array of \(lon\/lat\) pairs/);
  options.coordinates = [three_test_coordinates[0]];
  assert.throws(() => { osrm.match(options, (err, response) => {}); },
    /At least two coordinates must be provided/);
  options.coordinates = three_test_coordinates[0];
  assert.throws(() => { osrm.match(options, (err, response) => {}); },
    /Coordinates must be an array of \(lon\/lat\) pairs/);
  options.coordinates = [three_test_coordinates[0][0], three_test_coordinates[0][1]];
  assert.throws(() => { osrm.match(options, (err, response) => {}); },
    /Coordinates must be an array of \(lon\/lat\) pairs/);
});

test('match: throws on invalid timestamps param', (assert) => {
  assert.plan(3);
  const osrm = new OSRM(data_path);
  const options = {
    coordinates: three_test_coordinates,
    timestamps: 'timestamps'
  };
  assert.throws(() => { osrm.match(options, (err, response) => {}); },
    /Timestamps must be an array of integers \(or undefined\)/);
  options.timestamps = ['invalid', 'timestamp', 'array'];
  assert.throws(() => { osrm.match(options, (err, response) => {}); },
    /Timestamps array items must be numbers/);
  options.timestamps = [1424684612, 1424684616];
  assert.throws(() => { osrm.match(options, (err, response) => {}); },
    /Timestamp array must have the same size as the coordinates array/);
});

test('match: throws on invalid gaps param', (assert) => {
  assert.plan(2);
  const osrm = new OSRM(data_path);
  const options = {
    coordinates: three_test_coordinates,
    gaps: ['invalid gaps param']
  };
  assert.throws(() => { osrm.match(options, (err, response) => {}); },
    /Gaps must be a string: \[split, ignore\]/);
  options.gaps = 'invalid gaps param';
  assert.throws(() => { osrm.match(options, (err, response) => {}); },
    /'gaps' param must be one of \[split, ignore\]/);
});

test('match: throws on invalid tidy param', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(data_path);
  const options = {
    coordinates: three_test_coordinates,
    tidy: 'invalid tidy param'
  };
  assert.throws(() => { osrm.match(options, (err, response) => {}); },
    /tidy must be of type Boolean/);
});

test('match: throws on invalid config param', (assert) => {
  assert.plan(1);
  const osrm = new OSRM({path: mld_data_path, algorithm: 'MLD'});
  const options = {
    coordinates: three_test_coordinates,
  };
  assert.throws(() => { osrm.match(options, { format: 'invalid' }, (err, response) => {}); },
    /format must be a string:/);
});

test('match: match in Monaco without motorways', (assert) => {
  assert.plan(3);
  const osrm = new OSRM({path: mld_data_path, algorithm: 'MLD'});
  const options = {
    coordinates: three_test_coordinates,
    exclude: ['motorway']
  };
  osrm.match(options, (err, response) => {
    assert.ifError(err);
    assert.equal(response.tracepoints.length, 3);
    assert.equal(response.matchings.length, 1);
  });
});

test('match: throws on invalid waypoints values needs at least two', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(data_path);
  const options = {
    steps: true,
    coordinates: three_test_coordinates,
    waypoints: [0]
  };
  assert.throws(() => { osrm.match(options, (err, response) => {}); },
    'At least two waypoints must be provided');
});

test('match: throws on invalid waypoints values, needs first and last coordinate indices', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(data_path);
  const options = {
    steps: true,
    coordinates: three_test_coordinates,
    waypoints: [1, 2]
  };
  assert.throws(() => { osrm.match(options, (err, response) => {console.log(err);}); },
    'First and last waypoints values must correspond to first and last coordinate indices');
});

test('match: throws on invalid waypoints values, order matters', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(data_path);
  const options = {
    steps: true,
    coordinates: three_test_coordinates,
    waypoints: [2, 0]
  };
  assert.throws(() => { osrm.match(options, (err, response) => {console.log(err);}); },
    'First and last waypoints values must correspond to first and last coordinate indices');
});

test('match: throws on invalid waypoints values, waypoints must correspond with a coordinate index', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(data_path);
  const options = {
    steps: true,
    coordinates: three_test_coordinates,
    waypoints: [0, 3, 2]
  };
  assert.throws(() => { osrm.match(options, (err, response) => {console.log(err);}); },
    'Waypoints must correspond with the index of an input coordinate');
});

test('match: throws on invalid waypoints values, waypoints must be an array', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(data_path);
  const options = {
    steps: true,
    coordinates: three_test_coordinates,
    waypoints: 'string'
  };
  assert.throws(() => { osrm.match(options, (err, response) => { console.log(err); }); },
    'Waypoints must be an array of integers corresponding to the input coordinates.');
});

test('match: throws on invalid waypoints values, waypoints must be an array of integers', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(data_path);
  const options = {
    steps: true,
    coordinates: three_test_coordinates,
    waypoints: [0,1,'string']
  };
  assert.throws(() => { osrm.match(options, (err, response) => { console.log(err); }); },
    'Waypoint values must be an array of integers');
});

test('match: error on split trace', (assert) => {
  assert.plan(1);
  const osrm = new OSRM(data_path);
  const four_coords = Array.from(three_test_coordinates);
  four_coords.push([7.41902,43.73487]);
  const options = {
    steps: true,
    coordinates: four_coords,
    timestamps: [1700, 1750, 1424684616, 1424684620],
    waypoints: [0,3]
  };
  osrm.match(options, (err, response) => {
    assert.ok(err, 'Errors with NoMatch');
  });
});

test('match: match in Monaco with waypoints', (assert) => {
  assert.plan(6);
  const osrm = new OSRM(data_path);
  const options = {
    steps: true,
    coordinates: three_test_coordinates,
    waypoints: [0,2]
  };
  osrm.match(options, (err, response) => {
    assert.ifError(err);
    assert.equal(response.matchings.length, 1);
    assert.equal(response.matchings[0].legs.length, 1);
    assert.ok(response.matchings.every((m) => {
      return !!m.distance && !!m.duration && Array.isArray(m.legs) && !!m.geometry && m.confidence > 0;
    }));
    assert.equal(response.tracepoints.length, 3);
    assert.ok(response.tracepoints.every((t) => {
      return !!t.hint && !isNaN(t.matchings_index) && !isNaN(t.waypoint_index) && !!t.name;
    }));
  });
});

test('match: throws on disabled geometry', (assert) => {
  assert.plan(1);
  const osrm = new OSRM({'path': data_path, 'disable_feature_dataset': ['ROUTE_GEOMETRY']});
  const options = {
    coordinates: three_test_coordinates.concat(three_test_coordinates),
  };
  osrm.match(options, (err, route) => {
    console.log(err);
    assert.match(err.message, /DisabledDatasetException/);
  });
});

test('match: ok on disabled geometry', (assert) => {
  assert.plan(2);
  const osrm = new OSRM({'path': data_path, 'disable_feature_dataset': ['ROUTE_GEOMETRY']});
  const options = {
    steps: false,
    overview: 'false',
    annotations: false,
    skip_waypoints: true,
    coordinates: three_test_coordinates.concat(three_test_coordinates),
  };
  osrm.match(options, (err, response) => {
    assert.ifError(err);
    assert.equal(response.matchings.length, 1);
  });
});

test('match: throws on disabled steps', (assert) => {
  assert.plan(1);
  const osrm = new OSRM({'path': data_path, 'disable_feature_dataset': ['ROUTE_STEPS']});
  const options = {
    steps: true,
    coordinates: three_test_coordinates.concat(three_test_coordinates),
  };
  osrm.match(options, (err, route) => {
    console.log(err);
    assert.match(err.message, /DisabledDatasetException/);
  });
});

test('match: ok on disabled steps', (assert) => {
  assert.plan(8);
  const osrm = new OSRM({'path': data_path, 'disable_feature_dataset': ['ROUTE_STEPS']});
  const options = {
    steps: false,
    overview: 'simplified',
    annotations: true,
    coordinates: three_test_coordinates.concat(three_test_coordinates),
  };
  osrm.match(options, (err, response) => {
    assert.ifError(err);
    assert.ok(response.tracepoints);
    assert.ok(response.matchings);
    assert.equal(response.matchings.length, 1);
    assert.ok(response.matchings[0].geometry, 'the match has geometry');
    assert.ok(response.matchings[0].legs, 'the match has legs');
    assert.notok(response.matchings[0].legs.every(l => { return l.steps.length > 0; }), 'every leg has steps');
    assert.ok(response.matchings[0].legs.every(l => { return l.annotation;}), 'every leg has annotations');
  });
});
