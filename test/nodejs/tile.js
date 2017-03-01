var OSRM = require('../../');
var test = require('tape');
var berlin_path = "test/data/berlin-latest.osrm";

test.test('tile check size coarse', function(assert) {
    assert.plan(2);
    var osrm = new OSRM(berlin_path);
    osrm.tile([17603, 10747, 15], function(err, result) {
        assert.ifError(err);
        assert.ok(result.length > 35000);
    });
});

// FIXME the size of the tile that is returned depends on the architecture
// See issue #3343 in osrm-backend
test.skip('tile', function(assert) {
    assert.plan(2);
    var osrm = new OSRM(berlin_path);
    osrm.tile([17603, 10747, 15], function(err, result) {
        assert.ifError(err);
        assert.equal(result.length, 35970);
    });
});
