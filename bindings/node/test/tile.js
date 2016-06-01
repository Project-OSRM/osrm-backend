var OSRM = require('../../../build/bindings/node/Release/node-osrm.node').OSRM;
var test = require('tape');

test('tile', function(assert) {
    assert.plan(2);
    var osrm = new OSRM();
    osrm.tile([17603, 10747, 15], function(err, result) {
        assert.ifError(err);
        assert.equal(result.length, 18222);
    });
});
