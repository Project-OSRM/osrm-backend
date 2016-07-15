var test = require('tape');
var osrm = require('../lib/osrm-process');

test("extract: fails without configuration object", function(assert) {
    assert.plan(6);
    assert.throws(function() {
        osrm.extract();
    }, /first argument must be a configuration object/, "throws without parameter");
    assert.throws(function() {
        osrm.extract(4);
    }, /first argument must be a configuration object/, "throws with number parameter");
    assert.throws(function() {
        osrm.extract("test");
    }, /first argument must be a configuration object/, "throws with string parameter");
    assert.throws(function() {
        osrm.extract(null);
    }, /first argument must be a configuration object/, "throws with null parameter");
    assert.throws(function() {
        osrm.extract(undefined);
    }, /first argument must be a configuration object/, "throws with undefined parameter");
    assert.throws(function() {
        osrm.extract({}, {});
    }, /inputPath is missing from configuration object/, "throws with missing input path");
});

test("mode: enum values", function(assert) {
    assert.plan(12);
    assert.equals(0, osrm.mode.inaccessible);
    assert.equals(1, osrm.mode.driving);
    assert.equals(2, osrm.mode.cycling);
    assert.equals(3, osrm.mode.walking);
    assert.equals(4, osrm.mode.ferry);
    assert.equals(5, osrm.mode.train);
    assert.equals(6, osrm.mode.pushingBike);
    assert.equals(8, osrm.mode.stepsUp);
    assert.equals(9, osrm.mode.stepsDown);
    assert.equals(10, osrm.mode.riverUp);
    assert.equals(11, osrm.mode.riverDown);
    assert.equals(12, osrm.mode.route);
});

test("helper: durationIsValid", function(assert) {
    assert.plan(16);

    assert.equals(true, osrm.durationIsValid("0"));
    assert.equals(true, osrm.durationIsValid("00:01"));
    assert.equals(true, osrm.durationIsValid("00:01:01"));
    assert.equals(true, osrm.durationIsValid("61"));
    assert.equals(true, osrm.durationIsValid("24:01"));
    assert.equals(true, osrm.durationIsValid("00:01:60"));
    assert.equals(true, osrm.durationIsValid("PT15M"));

    assert.equals(false, osrm.durationIsValid(""));
    assert.equals(false, osrm.durationIsValid("PT15"));
    assert.equals(false, osrm.durationIsValid("PT15A"));
    assert.equals(false, osrm.durationIsValid("PT1H25:01"));
    assert.equals(false, osrm.durationIsValid("PT12501"));
    assert.equals(false, osrm.durationIsValid("PT0125:01"));
    assert.equals(false, osrm.durationIsValid("PT016001"));
    assert.equals(false, osrm.durationIsValid("PT240000"));
    assert.equals(false, osrm.durationIsValid("PT24:00:00"));
});

test("helper: parseDuration", function(assert) {
    assert.plan(24);

    assert.equals(0, osrm.parseDuration("00"));
    assert.equals(600, osrm.parseDuration("10"));
    assert.equals(60, osrm.parseDuration("00:01"));
    assert.equals(61, osrm.parseDuration("00:01:01"));
    assert.equals(3660, osrm.parseDuration("01:01"));

    // check all combinations of iso duration tokens
    assert.equals(61, osrm.parseDuration("PT1M1S"));
    assert.equals(3601, osrm.parseDuration("PT1H1S"));
    assert.equals(900, osrm.parseDuration("PT15M"));
    assert.equals(15, osrm.parseDuration("PT15S"));
    assert.equals(54000, osrm.parseDuration("PT15H"));
    assert.equals(4500, osrm.parseDuration("PT1H15M"));
    assert.equals(4501, osrm.parseDuration("PT1H15M1S"));
    assert.equals(8706, osrm.parseDuration("PT2H25M6S"));
    assert.equals(94501, osrm.parseDuration("P1DT2H15M1S"));
    assert.equals(345600, osrm.parseDuration("P4D"));
    assert.equals(14400, osrm.parseDuration("PT4H"));
    assert.equals(4260, osrm.parseDuration("PT71M"));
    assert.equals(8706, osrm.parseDuration("PT022506"));
    assert.equals(8706, osrm.parseDuration("PT02:25:06"));
    assert.equals(1814400, osrm.parseDuration("P3W"));

    assert.equals(900, osrm.parseDuration("PT15m"));
    assert.equals(4500, osrm.parseDuration("PT1h15m"));
    assert.equals(4542, osrm.parseDuration("PT1h15m42s"));
    assert.equals(177342, osrm.parseDuration("P2dT1h15m42s"));
});

var profile = require('./profile.js');

test("extract: use JS profile", function(assert) {
    osrm.extract({
        inputPath: "berlin-latest.osm.pbf"
    }, require('./profile.js'), function(err) {
        assert.ok(!err, "no error returned");
        assert.end();
    });
});
