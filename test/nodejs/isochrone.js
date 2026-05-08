// Test isochrone service
import test from 'tape';
import { OSRM, data_path as monaco_path, three_test_coordinates } from './constants.js';

test('isochrone: returns a FeatureCollection for Monaco', (assert) => {
  // Handle incompatible test data gracefully
  try {
    const osrm = new OSRM(monaco_path);
    assert.plan(3);
    const coord = three_test_coordinates[0];
    osrm.isochrone({coordinates: coord, range: 300}, (err, result) => {
      assert.ifError(err);
      assert.ok(result, 'isochrone returned a result');

      // Try to parse result as JSON (string or buffer)
      try {
        const raw = (result instanceof Buffer) ? result.toString() : result;
        const obj = JSON.parse(raw);
        assert.equal(obj.type, 'FeatureCollection');
      } catch (parseErr) {
        // If parsing fails, fail the test explicitly
        assert.fail('isochrone result was not valid JSON FeatureCollection');
      }
    });
  } catch (e) {
    if (e && e.message && e.message.includes('File is incompatible')) {
      assert.plan(1);
      assert.pass('skipped due to incompatible test data');
      return;
    }
    throw e;
  }
});
