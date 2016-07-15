// Car profile
var osrm = require('../lib/osrm-process');
const mode = osrm.mode;

let find_access_tag = require("./lib/access");
let get_destination = require("./lib/destination");

// Begin of globals
const barrier_whitelist = { 'cattle_grid': true, 'border_control': true, 'checkpoint': true, 'toll_booth': true, 'sally_port': true, 'gate': true, 'lift_gate': true, 'no': true, 'entrance': true };
const access_tag_whitelist = { 'yes': true, 'motorcar': true, 'motor_vehicle': true, 'vehicle': true, 'permissive': true, 'designated': true, 'destination': true };
const access_tag_blacklist = { 'no': true, 'private': true, 'agricultural': true, 'forestry': true, 'emergency': true, 'psv': true, 'delivery': true };
const access_tag_restricted = { 'destination': true, 'delivery': true };
const access_tags_hierarchy = [ "motorcar", "motor_vehicle", "vehicle", "access" ];
const service_tag_restricted = { 'parking_aisle': true };
const restriction_exception_tags = [ "motorcar", "motor_vehicle", "vehicle" ];

// A list of suffixes to suppress in name change instructions
const suffix_list = [ "N", "NE", "E", "SE", "S", "SW", "W", "NW", "North", "South", "West", "East" ];

const speed_profile = {
  'motorway': 90,
  'motorway_link': 45,
  'trunk': 85,
  'trunk_link': 40,
  'primary': 65,
  'primary_link': 30,
  'secondary': 55,
  'secondary_link': 25,
  'tertiary': 40,
  'tertiary_link': 20,
  'unclassified': 25,
  'residential': 25,
  'living_street': 10,
  'service': 15,
//  'track': 5,
  'ferry': 5,
  'movable': 5,
  'shuttle_train': 10,
  'default': 10
};


// surface/trackype/smoothness
// values were estimated from looking at the photos at the relevant wiki pages

// max speed for surfaces
const surface_speeds = {
  'asphalt': undefined,    // undefined mean no limit. removing the line has the same effect
  'concrete': undefined,
  'concrete:plates': undefined,
  'concrete:lanes': undefined,
  'paved': undefined,

  'cement': 80,
  'compacted': 80,
  'fine_gravel': 80,

  'paving_stones': 60,
  'metal': 60,
  'bricks': 60,

  'grass': 40,
  'wood': 40,
  'sett': 40,
  'grass_paver': 40,
  'gravel': 40,
  'unpaved': 40,
  'ground': 40,
  'dirt': 40,
  'pebblestone': 40,
  'tartan': 40,

  'cobblestone': 30,
  'clay': 30,

  'earth': 20,
  'stone': 20,
  'rocky': 20,
  'sand': 20,

  'mud': 10
};

// max speed for tracktypes
const tracktype_speeds = {
  'grade1': 60,
  'grade2': 40,
  'grade3': 30,
  'grade4': 25,
  'grade5': 20
};

// max speed for smoothnesses
const smoothness_speeds = {
  'intermediate': 80,
  'bad': 40,
  'very_bad': 20,
  'horrible': 10,
  'very_horrible': 5,
  'impassable': 0
};

// http://wiki.openstreetmap.org/wiki/Speed_limits
const maxspeed_table_default = {
  'urban': 50,
  'rural': 90,
  'trunk': 110,
  'motorway': 130
};

// List only exceptions
const maxspeed_table = {
  'ch:rural': 80,
  'ch:trunk': 100,
  'ch:motorway': 120,
  'de:living_street': 7,
  'ru:living_street': 20,
  'ru:urban': 60,
  'ua:urban': 60,
  'at:rural': 100,
  'de:rural': 100,
  'at:trunk': 100,
  'cz:trunk': 0,
  'ro:trunk': 100,
  'cz:motorway': 0,
  'de:motorway': 0,
  'ru:motorway': 110,
  'gb:nsl_single': (60*1609)/1000,
  'gb:nsl_dual': (70*1609)/1000,
  'gb:motorway': (70*1609)/1000,
  'uk:nsl_single': (60*1609)/1000,
  'uk:nsl_dual': (70*1609)/1000,
  'uk:motorway': (70*1609)/1000,
  'none': 140
};

// set profile properties
exports.uTurnPenalty                  = 20;
exports.trafficSignalPenalty          = 2;
exports.useTurnRestrictions           = true;
exports.continueStraightAtWaypoint    = true;

const side_road_speed_multiplier = 0.8;

const turn_penalty               = 10;
// 'note': this biases right-side driving.  Should be
// inverted for left-driving countries.
const turn_bias                  = 1.2;

const obey_oneway                = true;
const ignore_areas               = true;

const speed_reduction = 0.8;

exports.getNameSuffixList = function() {
  return suffix_list;
};

exports.getExceptions = function() {
  return restriction_exception_tags;
};

// returns forward,backward psv lane count
function getPSVCounts(way) {
    const psv = way.tag("lanes:psv")
    const psv_forward = way.tag("lanes:psv:forward");
    const psv_backward = way.tag("lanes:psv:backward");

    let fw = 0;
    let bw = 0;
    if (psv) {
        fw = parseFloat(psv)
        if (isNaN(fw)) {
            fw = 0;
        }
    }
    if (psv_forward) {
        fw = parseFloat(psv_forward);
        if (isNaN(fw)) {
            fw = 0;
        }
    }
    if (psv_backward) {
        bw = parseFloat(psv_backward);
        if (isNaN(bw)) {
            bw = 0;
        }
    }
    return [fw, bw];
}

// this is broken for left-sided driving. It needs to switch left && right in case of left-sided driving
function getTurnLanes(way) {
    let [fw_psv, bw_psv] = getPSVCounts(way);

    let turn_lanes = way.tag("turn:lanes");
    let turn_lanes_fw = way.tag("turn:lanes:forward");
    let turn_lanes_bw = way.tag("turn:lanes:backward");

    if (fw_psv || bw_psv) {
        if (turn_lanes) {
            turn_lanes = osrm.trimLaneString(turn_lanes, bw_psv, fw_psv);
        }
        if (turn_lanes_fw) {
            turn_lanes_fw = osrm.trimLaneString(turn_lanes_fw, bw_psv, fw_psv);
        }
        // backwards turn lanes need to treat bw_psv as fw_psv && vice versa
        if (turn_lanes_bw) {
          turn_lanes_bw = osrm.trimLaneString(turn_lanes_bw, fw_psv, bw_psv);
        }
    }

    return [turn_lanes, turn_lanes_fw, turn_lanes_bw];
}

function parse_maxspeed(source) {
  if (!source) {
    return 0;
  }
  let n = getInteger(source);
  if (typeof n !== 'undefined') {
    if (source.indexOf('mph') >= 0 || source.indexOf('mp/h') >= 0) {
      n = (n*1609)/1000;
    }
  } else {
    // parse maxspeed like FR:urban
    source = source.toLowerCase();
    n = maxspeed_table[source]
    if (typeof n === 'undefined') {
      let highway_type = source.substr(3);
      n = maxspeed_table_default[highway_type];
      if (typeof n === 'undefined') {
        n = 0;
      }
    }
  }
  return n;
}

exports.processNode = function(node, result) {
  // parse access && barrier tags
  const access = find_access_tag(node, access_tags_hierarchy);
  if (access) {
    if (access_tag_blacklist[access]) {
      result.barrier = true;
    }
  } else {
    const barrier = node.tag("barrier");
    if (barrier) {
      //  make an exception for rising bollard barriers
      const bollard = node.tag("bollard");
      const rising_bollard = (bollard === "rising");

      if (!rising_bollard && !barrier_whitelist[barrier]) {
        result.barrier = true;
      }
    }
  }

  // check if node is a traffic light
  const tag = node.tag("highway");
  if (tag === "traffic_signals") {
    result.trafficLights = true
  }
};

function getInteger(str, def) {
    var match = str.match(/\d+/);
    if (match) {
      return parseInt(match[0]);
    } else {
      return def;
    }
}

exports.processWay = function(way, result) {
  let highway = way.tag("highway");
  const route = way.tag("route");
  const bridge = way.tag("bridge");

  if (!(highway || route || bridge)) {
    return;
  }

  // we dont route over areas
  if (ignore_areas && way.tag("area") === "yes") {
    return;
  }

  // check if oneway tag is unsupported
  const oneway = way.tag("oneway");
  if (oneway === "reversible") {
    return;
  }

  if (way.tag("impassable") === "yes") {
    return;
  }

  if (way.tag("status") === "impassable") {
    return;
  }

  // Check if we are allowed to access the way
  const access = find_access_tag(way, access_tags_hierarchy);
  if (access_tag_blacklist[access]) {
    return;
  }

  result.forwardMode = mode.driving;
  result.backwardMode = mode.driving;

  // handling ferries && piers
  const route_speed = speed_profile[route];
  if (route_speed > 0) {
    highway = route;
    const duration = way.tag("duration");
    if (duration && osrm.durationIsValid(duration)) {
      result.duration = Math.max(osrm.parseDuration(duration), 1);
    }
    result.forwardMode = mode.ferry;
    result.backwardMode = mode.ferry;
    result.forwardSpeed = route_speed;
    result.backwardSpeed = route_speed;
  }

  // handling movable bridges
  const bridge_speed = speed_profile[bridge];
  const capacity_car = way.tag("capacity:car")
  if (bridge_speed > 0 && capacity_car) {
    highway = bridge;
    const duration = way.tag("duration")
    if (duration && durationIsValid(duration)) {
      result.duration = Math.max(osrm.parseDuration(duration), 1);
    }
    result.forwardSpeed = bridge_speed;
    result.backwardSpeed = bridge_speed;
  }

  // leave early of this way is !accessible
  if (highway === '') {
    return;
  }

  if (result.forwardSpeed == -1) {
    const highway_speed = speed_profile[highway];
    let max_speed = parse_maxspeed(way.tag("maxspeed"));
    // Set the avg speed on the way if it is accessible by road class
    if (highway_speed) {
      if (max_speed > highway_speed) {
        result.forwardSpeed = max_speed;
        result.backwardSpeed = max_speed;
        // max_speed = Infinity;
      } else {
        result.forwardSpeed = highway_speed;
        result.backwardSpeed = highway_speed;
      }
    } else {
      // Set the avg speed on ways that are marked accessible
      if (access_tag_whitelist[access]) {
        result.forwardSpeed = speed_profile["default"];
        result.backwardSpeed = speed_profile["default"];
      }
    }
    if (max_speed == 0) {
      max_speed = Infinity;
    }
    result.forwardSpeed = Math.min(result.forwardSpeed, max_speed);
    result.backwardSpeed = Math.min(result.backwardSpeed, max_speed);
  }

  if (result.forwardSpeed == -1 && result.backwardSpeed == -1) {
    return;
  }

  // reduce speed on special side roads
  const sideway = way.tag("side_road");
  if (sideway === "yes" || sideway === "rotary") {
    result.forwardSpeed = result.forwardSpeed * side_road_speed_multiplier;
    result.backwardSpeed = result.backwardSpeed * side_road_speed_multiplier;
  }

  // reduce speed on bad surfaces
  const surface = way.tag("surface");
  const tracktype = way.tag("tracktype");
  const smoothness = way.tag("smoothness");

  if (surface && surface_speeds[surface]) {
    result.forwardSpeed = Math.min(surface_speeds[surface], result.forwardSpeed);
    result.backwardSpeed = Math.min(surface_speeds[surface], result.backwardSpeed);
  }
  if (tracktype && tracktype_speeds[tracktype]) {
    result.forwardSpeed = Math.min(tracktype_speeds[tracktype], result.forwardSpeed);
    result.backwardSpeed = Math.min(tracktype_speeds[tracktype], result.backwardSpeed);
  }
  if (smoothness && smoothness_speeds[smoothness]) {
    result.forwardSpeed = Math.min(smoothness_speeds[smoothness], result.forwardSpeed);
    result.backwardSpeed = Math.min(smoothness_speeds[smoothness], result.backwardSpeed);
  }

  // parse the remaining tags
  const name = way.tag("name");
  const pronunciation = way.tag("name:pronunciation");
  const ref = way.tag("ref");
  const junction = way.tag("junction");
  // const barrier = way.tag("barrier", "")
  // const cycleway = way.tag("cycleway", "")
  const service = way.tag("service");

  // Set the name that will be used for instructions
  let has_ref = ref && ref != "";
  let has_name = name && name != "";
  let has_pronunciation = pronunciation && pronunciation != "";

  if (has_name && has_ref) {
    result.name = name + " (" + ref + ")"
  } else if (has_ref) {
    result.name = ref
  } else if (has_name) {
    result.name = name
  }

  if (has_pronunciation) {
    result.pronunciation = pronunciation
  }

  let turn_lanes = ""
  let turn_lanes_forward = ""
  let turn_lanes_backward = ""

  turn_lanes, turn_lanes_forward, turn_lanes_backward = getTurnLanes(way)
  if (turn_lanes && turn_lanes != "") {
    result.turnLanesForward = turn_lanes;
    result.turnLanesBackward = turn_lanes;
  } else {
    if (turn_lanes_forward && turn_lanes_forward != "") {
        result.turnLanesForward = turn_lanes_forward;
    }

    if (turn_lanes_backward && turn_lanes_backward != "") {
        result.turnLanesBackward = turn_lanes_backward;
    }
  }


  if (junction && junction === "roundabout") {
    result.roundabout = true
  }

  // Set access restriction flag if access is allowed under certain restrictions only
  if (access != "" && access_tag_restricted[access]) {
    result.isAccessRestriction = true
  }

  // Set access restriction flag if service is allowed under certain restrictions only
  if (service && service != "" && service_tag_restricted[service]) {
    result.isAccessRestriction = true
  }

  // Set direction according to tags on way
  if (obey_oneway) {
    if (oneway === "-1") {
      result.forwardMode = mode.inaccessible;
    } else if (oneway === "yes" ||
               oneway === "1" ||
               oneway === "true" ||
               junction === "roundabout" ||
               (highway === "motorway" && oneway != "no")) {
      result.backwardMode = mode.inaccessible;

      // If we're on a oneway && there is no ref tag, re-use destination tag as ref.
      let destination = get_destination(way);
      let has_destination = destination != "";

      if (has_destination && has_name && !has_ref) {
        result.name = name + " (" + destination + ")";
      }

      result.destinations = destination;
    }
  }

  // Override speed settings if explicit forward/backward maxspeeds are given
  let maxspeed_forward = parse_maxspeed(way.tag("maxspeed:forward"));
  let maxspeed_backward = parse_maxspeed(way.tag("maxspeed:backward"));
  if (maxspeed_forward && maxspeed_forward > 0) {
    if (mode.inaccessible != result.forwardMode && mode.inaccessible != result.backwardMode) {
      result.backwardSpeed = result.forwardSpeed;
    }
    result.forwardSpeed = maxspeed_forward;
  }
  if (maxspeed_backward && maxspeed_backward > 0) {
    result.backwardSpeed = maxspeed_backward;
  }

  // Override speed settings if advisory forward/backward maxspeeds are given
  let advisory_speed = parse_maxspeed(way.tag("maxspeed:advisory"));
  let advisory_forward = parse_maxspeed(way.tag("maxspeed:advisory:forward"));
  let advisory_backward = parse_maxspeed(way.tag("maxspeed:advisory:backward"));
  // apply bi-directional advisory speed first
  if (advisory_speed && advisory_speed > 0) {
    if (mode.inaccessible != result.forwardMode) {
      result.forwardSpeed = advisory_speed;
    }
    if (mode.inaccessible != result.backwardMode) {
      result.backwardSpeed = advisory_speed;
    }
  }
  if (advisory_forward && advisory_forward > 0) {
    if (mode.inaccessible != result.forwardMode && mode.inaccessible != result.backwardMode) {
      result.backwardSpeed = result.forwardSpeed;
    }
    result.forwardSpeed = advisory_forward;
  }
  if (advisory_backward && advisory_backward > 0) {
    result.backwardSpeed = advisory_backward;
  }

  let width = Infinity;
  let lanes = Infinity;
  if (result.forwardSpeed > 0 || result.backwardSpeed > 0) {
    const width_string = way.tag("width")
    if (width_string) {
      width = getInteger(width_string, width);
    }

    const lanes_string = way.tag("lanes")
    if (lanes_string) {
      lanes = getInteger(lanes_string, lanes);
    }
  }

  let is_bidirectional = result.forwardMode != mode.inaccessible && result.backwardMode != mode.inaccessible;

  // scale speeds to get better avg driving times
  if (result.forwardSpeed > 0) {
    let scaled_speed = result.forwardSpeed * speed_reduction + 11;
    let penalized_speed = Infinity;
    if (width <= 3 || (lanes <= 1 && is_bidirectional)) {
      penalized_speed = result.forwardSpeed / 2;
    }
    result.forwardSpeed = Math.min(penalized_speed, scaled_speed);
  }

  if (result.backwardSpeed > 0) {
    let scaled_speed = result.backwardSpeed * speed_reduction + 11;
    let penalized_speed = Infinity;
    if (width <= 3 || (lanes <= 1 && is_bidirectional)) {
      penalized_speed = result.backwardSpeed / 2;
    }
    result.backwardSpeed = Math.min(penalized_speed, scaled_speed);
  }

  // only allow this road as start point if it !a ferry
  result.isStartpoint = result.forwardMode == mode.driving || result.backwardMode == mode.driving;
};

exports.getTurnPenalty = function(angle) {
  //-- compute turn penalty as angle^2, with a left/right bias
  k = turn_penalty/(90.0*90.0);
  if (angle>=0) {
    return angle*angle*k/turn_bias;
  } else {
    return angle*angle*k*turn_bias;
  }
};
