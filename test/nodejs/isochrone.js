import test from 'tape';
import { OSRM, mld_data_path, two_test_coordinates } from './constants.js';

test('isochrone: returns a GeoJSON FeatureCollection on MLD', (assert) => {
  assert.plan(6);
  const osrm = new OSRM({path: mld_data_path, algorithm: 'MLD'});
  osrm.isochrone({
    coordinates: [two_test_coordinates[0]],
    contours: [300, 600],
    polygons: false,
  }, (err, result) => {
    assert.ifError(err);
    assert.equal(result.type, 'FeatureCollection');
    assert.notOk(Object.hasOwn(result, 'weight_name'));
    assert.ok(Array.isArray(result.features));
    assert.equal(result.features.length, 2);
    assert.ok(result.features.every((feature) =>
      feature.type === 'Feature' && feature.geometry.type === 'MultiLineString' &&
      Array.isArray(feature.geometry.coordinates) &&
      Number.isFinite(feature.properties.effective_contour) &&
      feature.properties.effective_contour <= feature.properties.contour));
  });
});

test('isochrone: reports the decisecond duration contour that was evaluated', (assert) => {
  assert.plan(3);
  const osrm = new OSRM({path: mld_data_path, algorithm: 'MLD'});
  osrm.isochrone({
    coordinates: [two_test_coordinates[0]],
    contours: [0.15],
  }, (err, result) => {
    assert.ifError(err);
    assert.equal(result.features.length, 1);
    assert.equal(result.features[0].properties.effective_contour, 0.1);
  });
});

test('isochrone: parses parameters and rejects invalid arguments', (assert) => {
  assert.plan(7);
  const osrm = new OSRM({path: mld_data_path, algorithm: 'MLD'});
  const options = { coordinates: [two_test_coordinates[0]] };

  assert.throws(() => { osrm.isochrone(options, () => {}); },
    /Contours must be a non-empty array of positive finite durations in seconds/);

  options.contours = [];
  assert.throws(() => { osrm.isochrone(options, () => {}); },
    /Contours must be a non-empty array of positive finite durations in seconds/);

  options.contours = [0];
  assert.throws(() => { osrm.isochrone(options, () => {}); },
    /Contours must be a non-empty array of positive finite durations in seconds/);

  options.contours = [Infinity];
  assert.throws(() => { osrm.isochrone(options, () => {}); },
    /Contours must be a non-empty array of positive finite durations in seconds/);

  options.contours = [300];
  options.direction = 'sideways';
  assert.throws(() => { osrm.isochrone(options, () => {}); },
    /Direction must be a string: \[outbound, inbound\]/);

  options.direction = 'outbound';
  options.polygons = 'true';
  assert.throws(() => { osrm.isochrone(options, () => {}); },
    /polygons must be of type Boolean/);

  options.polygons = false;
  assert.doesNotThrow(() => { osrm.isochrone(options, () => {}); });
});

test('isochrone: rejects flatbuffers output asynchronously', (assert) => {
  assert.plan(1);
  const osrm = new OSRM({path: mld_data_path, algorithm: 'MLD'});

  osrm.isochrone({
    coordinates: [two_test_coordinates[0]],
    contours: [300],
    format: 'flatbuffers',
  }, (err) => {
    assert.equal(err.message, 'The isochrone service only supports JSON/GeoJSON output.');
  });
});
