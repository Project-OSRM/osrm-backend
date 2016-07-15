var osrm = require('../lib/osrm-process');
const mode = osrm.mode;

// Testbot profile

// Moves at fixed, well-known speeds, practical for testing speed and travel
// times:

// Primary road:  36km/h = 36000m/3600s = 100m/10s
// Secondary road:  18km/h = 18000m/3600s = 100m/20s
// Tertiary road:  12km/h = 12000m/3600s = 100m/30s

const speedProfile = {
  primary: 36,
  secondary: 18,
  tertiary: 12,
  steps: 6,
  default: 24
};


// these settings are read directly by osrm

exports.continueStraightAtWaypoint = true;
exports.useTurnRestrictions = true;
exports.trafficSignalPenalty = 7; // seconds
exports.uTurnPenalty = 20;

exports.processNode = function(node, result) {
  const trafficSignal = node.tag('highway');

  if (trafficSignal === 'traffic_signals') {
    result.trafficLights = true;
    // TODO: a way to set the penalty value
  }
};

exports.processWay = function(way, result) {
  const highway = way.tag('highway');
  const name = way.tag('name');
  const oneway = way.tag('oneway');
  const route = way.tag('route');
  const duration = way.tag('duration');
  const maxspeed = +(way.tag('maxspeed') || 0);
  const maxspeedForward = +(way.tag('maxspeed:forward') || 0);
  const maxspeedBackward = +(way.tag('maxspeed:backward') || 0);
  const junction = way.tag('junction');

  if (name) {
    result.name = name;
  }

  result.forwardMode = mode.driving;
  result.backwardMode = mode.driving;

  if (osrm.durationIsValid(duration)) {
    result.duration = Math.max(1, osrm.parseDuration(duration));
    result.forwardMode = mode.route;
    result.backwardMode = mode.route;
  }
  else {
    var speedForw = speedProfile[highway] || speedProfile.default;
    var speedBack = speedForw;

    if (highway === 'river') {
      var tempSpeed = speedForw;
      result.forwardMode = mode.riverDown;
      result.backwardMode = mode.riverUp;
      speedForw = tempSpeed * 1.5;
      speedBack = tempSpeed / 1.5;
    }
    else if (highway === 'steps') {
      result.forwardMode = mode.stepsDown;
      result.backwardMode = mode.stepsUp;
    }

    if ((typeof maxspeedForward !== 'undefined') && maxspeedForward > 0) {
      speedForw = maxspeedForward;
    } else if ((typeof maxspeed !== 'undefined') && maxspeed > 0 &&
               speedForw > maxspeed) {
      speedForw = maxspeed;
    }

    if ((typeof maxspeedBackward !== 'undefined') && maxspeedBackward > 0) {
      speedBack = maxspeedBackward;
    } else if ((typeof maxspeed !== 'undefined') && maxspeed > 0 &&
               speedBack > maxspeed) {
      speedBack = maxspeed;
    }

    result.forwardSpeed = speedForw;
    result.backwardSpeed = speedBack;
  }

  if (oneway === 'no' || oneway === '0' || oneway === 'false') {
    // nothing to do
  } else if (oneway === '-1') {
    result.forwardMode = mode.inaccessible;
  } else if (oneway === 'yes' || oneway === '1' || oneway === 'true' ||
             junction === 'roundabout') {
    result.backward_mode = mode.inaccessible;
  }

  if (junction === 'roundabout') {
    result.roundabout = true;
  }
};
