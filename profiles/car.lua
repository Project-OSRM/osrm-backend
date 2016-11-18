-- Car profile
local find_access_tag = require("lib/access").find_access_tag
local get_destination = require("lib/destination").get_destination
local set_classification = require("lib/guidance").set_classification
local get_turn_lanes = require("lib/guidance").get_turn_lanes
local Set = require('lib/set')
local Sequence = require('lib/sequence')
local Directional = require('lib/directional')

-- Begin of globals
barrier_whitelist = Set {
  'cattle_grid',
  'border_control',
  'checkpoint',
  'toll_booth',
  'sally_port',
  'gate',
  'lift_gate',
  'no',
  'entrance'
}

access_tag_whitelist = Set {
  'yes',
  'motorcar',
  'motor_vehicle',
  'vehicle',
  'permissive',
  'designated',
  'destination'
}

access_tag_blacklist = Set {
  'no',
  'private',
  'agricultural',
  'forestry',
  'emergency',
  'psv',
  'delivery'
}

access_tag_restricted = Set {
  'destination',
  'delivery'
}

access_tags_hierarchy = Sequence {
  'motorcar',
  'motor_vehicle',
  'vehicle',
  'access'
}

service_tag_restricted = Set {
  'parking_aisle',
  'parking'
}

service_tag_forbidden = Set {
  'emergency_access'
}

restrictions = Sequence {
  'motorcar', "motor_vehicle", "vehicle" }

-- A list of suffixes to suppress in name change instructions
-- Note: a Set does not work here because it's read from C++
suffix_list = {
  'N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW', 'North', 'South', 'West', 'East'
}

speed_profile = {
  motorway = 90,
  motorway_link = 45,
  trunk = 85,
  trunk_link = 40,
  primary = 65,
  primary_link = 30,
  secondary = 55,
  secondary_link = 25,
  tertiary = 40,
  tertiary_link = 20,
  unclassified = 25,
  residential = 25,
  living_street = 10,
  service = 15,
--track = 5,
  ferry = 5,
  movable = 5,
  shuttle_train = 10,
  default = 10
}

-- service speeds
service_speeds = {
  alley = 5,
  parking = 5,
  parking_aisle = 5,
  driveway = 5,
  ["drive-through"] = 5
}

-- surface/trackype/smoothness
-- values were estimated from looking at the photos at the relevant wiki pages

-- max speed for surfaces
surface_speeds = {
  asphalt = nil,    -- nil mean no limit. removing the line has the same effect
  concrete = nil,
  ["concrete:plates"] = nil,
  ["concrete:lanes"] = nil,
  paved = nil,

  cement = 80,
  compacted = 80,
  fine_gravel = 80,

  paving_stones = 60,
  metal = 60,
  bricks = 60,

  grass = 40,
  wood = 40,
  sett = 40,
  grass_paver = 40,
  gravel = 40,
  unpaved = 40,
  ground = 40,
  dirt = 40,
  pebblestone = 40,
  tartan = 40,

  cobblestone = 30,
  clay = 30,

  earth = 20,
  stone = 20,
  rocky = 20,
  sand = 20,

  mud = 10
}

-- max speed for tracktypes
tracktype_speeds = {
  grade1 =  60,
  grade2 =  40,
  grade3 =  30,
  grade4 =  25,
  grade5 =  20
}

-- max speed for smoothnesses
smoothness_speeds = {
  intermediate    =  80,
  bad             =  40,
  very_bad        =  20,
  horrible        =  10,
  very_horrible   =  5,
  impassable      =  0
}

-- http://wiki.openstreetmap.org/wiki/Speed_limits
maxspeed_table_default = {
  urban = 50,
  rural = 90,
  trunk = 110,
  motorway = 130
}

-- List only exceptions
maxspeed_table = {
  ["ch:rural"] = 80,
  ["ch:trunk"] = 100,
  ["ch:motorway"] = 120,
  ["de:living_street"] = 7,
  ["ru:living_street"] = 20,
  ["ru:urban"] = 60,
  ["ua:urban"] = 60,
  ["at:rural"] = 100,
  ["de:rural"] = 100,
  ["at:trunk"] = 100,
  ["cz:trunk"] = 0,
  ["ro:trunk"] = 100,
  ["cz:motorway"] = 0,
  ["de:motorway"] = 0,
  ["ru:motorway"] = 110,
  ["gb:nsl_single"] = (60*1609)/1000,
  ["gb:nsl_dual"] = (70*1609)/1000,
  ["gb:motorway"] = (70*1609)/1000,
  ["uk:nsl_single"] = (60*1609)/1000,
  ["uk:nsl_dual"] = (70*1609)/1000,
  ["uk:motorway"] = (70*1609)/1000,
  ["nl:rural"] = 80,
  ["nl:trunk"] = 100,
  ["none"] = 140
}

-- set profile properties
properties.u_turn_penalty                  = 20
properties.traffic_signal_penalty          = 2
properties.max_speed_for_map_matching      = 180/3.6 -- 180kmph -> m/s
properties.use_turn_restrictions           = true
properties.continue_straight_at_waypoint   = true
properties.left_hand_driving               = false

local side_road_speed_multiplier = 0.8

local turn_penalty               = 7.5
-- Note: this biases right-side driving.  Should be
-- inverted for left-driving countries.
local turn_bias                  = properties.left_hand_driving and 1/1.075 or 1.075

local obey_oneway                = true
local ignore_areas               = true
local ignore_hov_ways            = true
local ignore_toll_ways           = false

local abs = math.abs
local min = math.min
local max = math.max

local speed_reduction = 0.8

function get_name_suffix_list(vector)
  for index,suffix in ipairs(suffix_list) do
      vector:Add(suffix)
  end
end

function get_restrictions(vector)
  for i,v in ipairs(restrictions) do
    vector:Add(v)
  end
end

local function parse_maxspeed(source)
  if not source then
    return 0
  end
  local n = tonumber(source:match("%d*"))
  if n then
    if string.match(source, "mph") or string.match(source, "mp/h") then
      n = (n*1609)/1000
    end
  else
    -- parse maxspeed like FR:urban
    source = string.lower(source)
    n = maxspeed_table[source]
    if not n then
      local highway_type = string.match(source, "%a%a:(%a+)")
      n = maxspeed_table_default[highway_type]
      if not n then
        n = 0
      end
    end
  end
  return n
end

function node_function (node, result)
  -- parse access and barrier tags
  local access = find_access_tag(node, access_tags_hierarchy)
  if access then
    if access_tag_blacklist[access] then
      result.barrier = true
    end
  else
    local barrier = node:get_value_by_key("barrier")
    if barrier then
      --  make an exception for rising bollard barriers
      local bollard = node:get_value_by_key("bollard")
      local rising_bollard = bollard and "rising" == bollard

      if not barrier_whitelist[barrier] and not rising_bollard then
        result.barrier = true
      end
    end
  end

  -- check if node is a traffic light
  local tag = node:get_value_by_key("highway")
  if "traffic_signals" == tag then
    result.traffic_lights = true
  end
end

-- abort early if this way is obviouslt not routable
function initial_routability_check(way,result,data)
  data.highway = way:get_value_by_key('highway')

  return data.highway ~= nil or
         way:get_value_by_key('route') ~= nil or
         way:get_value_by_key('bridge') ~= nil
end

-- all lanes restricted to hov vehicles?
local function has_all_designated_hov_lanes(lanes)
  if not lanes then
    return false
  end
  -- This gmatch call effectively splits the string on | chars.
  -- we append an extra | to the end so that we can match the final part
  for lane in (lanes .. '|'):gmatch("([^|]*)|") do
    if lane and lane ~= "designated" then
      return false
    end
  end
  return true
end

-- handle high occupancy vehicle tags
function handle_hov(way,result,data)
  -- respect user-preference for HOV
  if not ignore_hov_ways then
    return
  end

  -- check if way is hov only
  local hov = way:get_value_by_key("hov")
  if "designated" == hov then
    return false
  end

  -- check if all lanes are hov only
  local hov_lanes_forward, hov_lanes_backward = Directional.get_values_by_key(way,data,'hov:lanes')
  local inaccessible_forward = has_all_designated_hov_lanes(hov_lanes_forward)
  local inaccessible_backward = has_all_designated_hov_lanes(hov_lanes_backward)

  if inaccessible_forward then
    result.forward_mode = mode.inaccessible
  end
  if inaccessible_backward then
    result.backward_mode = mode.inaccessible
  end
end

-- handle various that can block access
function is_way_blocked(way,result)
  -- we dont route over areas
  local area = way:get_value_by_key("area")
  if ignore_areas and "yes" == area then
    return false
  end
  
  -- respect user-preference for toll=yes ways
  local toll = way:get_value_by_key("toll")
  if ignore_toll_ways and "yes" == toll then
    return false
  end

  -- Reversible oneways change direction with low frequency (think twice a day):
  -- do not route over these at all at the moment because of time dependence.
  -- Note: alternating (high frequency) oneways are handled below with penalty.
  local oneway = way:get_value_by_key("oneway")
  if "reversible" == oneway then
    return false
  end

  local impassable = way:get_value_by_key("impassable")
  if "yes" == impassable then
    return false
  end

  local status = way:get_value_by_key("status")
  if "impassable" == status then
    return false
  end
end

-- set default mode
function set_default_mode(way,result)
  result.forward_mode = mode.driving
  result.backward_mode = mode.driving
end

-- check accessibility by traversing our acces tag hierarchy
function handle_access(way,result,data)
  data.access = find_access_tag(way, access_tags_hierarchy)
  if access_tag_blacklist[data.access] then
    return false
  end
end

-- handling ferries and piers
function handle_ferries(way,result)
  local route = way:get_value_by_key("route")
  if route then
    local route_speed = speed_profile[route]
    if route_speed and route_speed > 0 then
     local duration  = way:get_value_by_key("duration")
     if duration and durationIsValid(duration) then
       result.duration = max( parseDuration(duration), 1 )
     end
     result.forward_mode = mode.ferry
     result.backward_mode = mode.ferry
     result.forward_speed = route_speed
     result.backward_speed = route_speed
    end
  end
end

-- handling movable bridges
function handle_movables(way,result)
  local bridge = way:get_value_by_key("bridge")
  if bridge then
    local bridge_speed = speed_profile[bridge]
    if bridge_speed and bridge_speed > 0 then
      local capacity_car = way:get_value_by_key("capacity:car")
      if capacity_car ~= 0 then
        local duration  = way:get_value_by_key("duration")
        if duration and durationIsValid(duration) then
          result.duration = max( parseDuration(duration), 1 )
        end
        result.forward_speed = bridge_speed
        result.backward_speed = bridge_speed
      end
    end
  end
end

-- handle speed (excluding maxspeed)
function handle_speed(way,result,data)
  if result.forward_speed == -1 then
    local highway_speed = speed_profile[data.highway]
    -- Set the avg speed on the way if it is accessible by road class
    if highway_speed then
      result.forward_speed = highway_speed
      result.backward_speed = highway_speed
    else
      -- Set the avg speed on ways that are marked accessible
      if access_tag_whitelist[data.access] then
        result.forward_speed = speed_profile["default"]
        result.backward_speed = speed_profile["default"]
      end
    end
  end

  if -1 == result.forward_speed and -1 == result.backward_speed then
    return false
  end
  
  if handle_side_roads(way,result) == false then return false end
  if handle_surface(way,result) == false then return false end
  if handle_maxspeed(way,data,result) == false then return false end
  if handle_speed_scaling(way,result) == false then return false end
  if handle_alternating_speed(way,result) == false then return false end
end

-- reduce speed on special side roads
function handle_side_roads(way,result)  
  local sideway = way:get_value_by_key("side_road")
  if "yes" == sideway or
  "rotary" == sideway then
    result.forward_speed = result.forward_speed * side_road_speed_multiplier
    result.backward_speed = result.backward_speed * side_road_speed_multiplier
  end
end

-- reduce speed on bad surfaces
function handle_surface(way,result)
  local surface = way:get_value_by_key("surface")
  local tracktype = way:get_value_by_key("tracktype")
  local smoothness = way:get_value_by_key("smoothness")

  if surface and surface_speeds[surface] then
    result.forward_speed = math.min(surface_speeds[surface], result.forward_speed)
    result.backward_speed = math.min(surface_speeds[surface], result.backward_speed)
  end
  if tracktype and tracktype_speeds[tracktype] then
    result.forward_speed = math.min(tracktype_speeds[tracktype], result.forward_speed)
    result.backward_speed = math.min(tracktype_speeds[tracktype], result.backward_speed)
  end
  if smoothness and smoothness_speeds[smoothness] then
    result.forward_speed = math.min(smoothness_speeds[smoothness], result.forward_speed)
    result.backward_speed = math.min(smoothness_speeds[smoothness], result.backward_speed)
  end
end

-- handles name, including ref and pronunciation
function handle_names(way,result)
  -- parse the remaining tags
  local name = way:get_value_by_key("name")
  local pronunciation = way:get_value_by_key("name:pronunciation")
  local ref = way:get_value_by_key("ref")

  -- Set the name that will be used for instructions
  if name then
    result.name = name
  end

  if ref then
    result.ref = canonicalizeStringList(ref, ";")
  end

  if pronunciation then
    result.pronunciation = pronunciation
  end
end

-- handle turn lanes
function handle_turn_lanes(way,result,data)
  local forward, backward = get_turn_lanes(way,data)

  if forward then
    result.turn_lanes_forward = forward
  end

  if backward then
    result.turn_lanes_backward = backward
  end
end

-- junctions
function handle_roundabouts(way,result)
  local junction = way:get_value_by_key("junction");

  if junction == "roundabout" then
    result.roundabout = true
  end

  -- See Issue 3361: roundabout-shaped not following roundabout rules.
  -- This will get us "At Strausberger Platz do Maneuver X" instead of multiple quick turns.
  -- In a new API version we can think of having a separate type passing it through to the user.
  if junction == "circular" then
    result.circular = true
  end
end

-- Set access restriction flag if access is allowed under certain restrictions only
function handle_restricted(way,result,data)
  if data.access and access_tag_restricted[data.access] then
    result.is_access_restricted = true
  end
end

-- service roads
function handle_service(way,result)
  local service = way:get_value_by_key("service")
  if service then
    -- Set access restriction flag if service is allowed under certain restrictions only
    if service_tag_restricted[service] then
      result.is_access_restricted = true
    end

    -- Set don't allow access to certain service roads
    if service_tag_forbidden[service] then
      result.forward_mode = mode.inaccessible
      result.backward_mode = mode.inaccessible
      return false
    end
  end
end

-- scale speeds to get better average driving times
function handle_speed_scaling(way,result)
  local width = math.huge
  local lanes = math.huge
  if result.forward_speed > 0 or result.backward_speed > 0 then
    local width_string = way:get_value_by_key("width")
    if width_string and tonumber(width_string:match("%d*")) then
      width = tonumber(width_string:match("%d*"))
    end

    local lanes_string = way:get_value_by_key("lanes")
    if lanes_string and tonumber(lanes_string:match("%d*")) then
      lanes = tonumber(lanes_string:match("%d*"))
    end
  end

  local is_bidirectional = result.forward_mode ~= mode.inaccessible and 
                           result.backward_mode ~= mode.inaccessible

  local service = way:get_value_by_key("service")
  if result.forward_speed > 0 then
    local scaled_speed = result.forward_speed * speed_reduction
    local penalized_speed = math.huge
    if service and service_speeds[service] then
      penalized_speed = service_speeds[service]
    elseif width <= 3 or (lanes <= 1 and is_bidirectional) then
      penalized_speed = result.forward_speed / 2
    end
    result.forward_speed = math.min(penalized_speed, scaled_speed)
  end

  if result.backward_speed > 0 then
    local scaled_speed = result.backward_speed * speed_reduction
    local penalized_speed = math.huge
    if service and service_speeds[service]then
      penalized_speed = service_speeds[service]
    elseif width <= 3 or (lanes <= 1 and is_bidirectional) then
      penalized_speed = result.backward_speed / 2
    end
    result.backward_speed = math.min(penalized_speed, scaled_speed)
  end
end

-- handle oneways tags
function handle_oneway(way,result,data)
  local oneway = way:get_value_by_key("oneway")
  data.oneway = oneway
  if obey_oneway then
    if oneway == "-1" then
      data.is_reverse_oneway = true
      result.forward_mode = mode.inaccessible
    elseif oneway == "yes" or
           oneway == "1" or
           oneway == "true" then
      data.is_forward_oneway = true
      result.backward_mode = mode.inaccessible
    else
      local junction = way:get_value_by_key("junction")
      if data.highway == "motorway" or
         junction == "roundabout" or 
         junction == "circular" then
        if oneway ~= "no" then
          -- implied oneway
          data.is_forward_oneway = true
          result.backward_mode = mode.inaccessible
        end
      end
    end
  end
end

-- handle destination tags
function handle_destinations(way,result,data)
  if data.is_forward_oneway or data.is_reverse_oneway then
    local destination = get_destination(way, data.is_forward_oneway)
    result.destinations = canonicalizeStringList(destination, ",")
  end
end

-- maxspeed and advisory maxspeed
function handle_maxspeed(way,data,result)
  local keys = Sequence { 'maxspeed:advisory', 'maxspeed' }
  local forward, backward = Directional.get_values_by_set(way,data,keys)
  forward = parse_maxspeed(forward)
  backward = parse_maxspeed(backward)

  if forward and forward > 0 then
    result.forward_speed = forward
  end

  if backward and backward > 0 then
    result.backward_speed = backward
  end
end

-- Handle high frequency reversible oneways (think traffic signal controlled, changing direction every 15 minutes).
-- Scaling speed to take average waiting time into account plus some more for start / stop.
function handle_alternating_speed(way,result)
  if "alternating" == way:get_value_by_key('oneway') then
    local scaling_factor = 0.4
    if result.forward_speed ~= math.huge then
      result.forward_speed = result.forward_speed * scaling_factor
    end
    if result.backward_speed ~= math.huge then
      result.backward_speed = result.backward_speed * scaling_factor
    end
  end
end


-- determine if this way can be used as a start/end point for routing
function handle_startpoint(way,result)
  -- only allow this road as start point if it not a ferry
  result.is_startpoint = result.forward_mode == mode.driving or 
                              result.backward_mode == mode.driving
end

-- set the road classification based on guidance globals configuration
function handle_classification(way,result,data)
  set_classification(data.highway,result,way)
end

-- main entry point for processsing a way
function way_function(way, result)
  -- intermediate values used during processing
  local data = {}

  -- to optimize processing, we should try to abort as soon as
  -- possible if the way is not routable, to avoid doing
  -- unnecessary work. this implies we should check things that
  -- commonly forbids access early, and handle complicated edge
  -- cases later.
  
  -- perform an quick initial check and abort if way is obviously
  -- not routable, e.g. because it does not have any of the key
  -- tags indicating routability
  if initial_routability_check(way,result,data) == false then return end

  -- set the default mode for this profile. if can be changed later
  -- in case it turns we're e.g. on a ferry
  if set_default_mode(way,result) == false then return end

  -- check various tags that could indicate that the way is not
  -- routable. this includes things like status=impassable,
  -- toll=yes and oneway=reversible
  if is_way_blocked(way,result) == false then return end

  -- determine access status by checking our hierarchy of
  -- access tags, e.g: motorcar, motor_vehicle, vehicle
  if handle_access(way,result,data) == false then return end

  -- check whether forward/backward directons are routable
  if handle_oneway(way,result,data) == false then return end

  -- check whether forward/backward directons are routable
  if handle_destinations(way,result,data) == false then return end

  -- check whether we're using a special transport mode
  if handle_ferries(way,result) == false then return end
  if handle_movables(way,result) == false then return end

  -- handle service road restrictions
  if handle_service(way,result) == false then return end

  -- check high occupancy vehicle restrictions
  if handle_hov(way,result,data) == false then return end

  -- compute speed taking into account way type, maxspeed tags, etc.
  if handle_speed(way,result,data) == false then return end

  -- handle turn lanes and road classification, used for guidance
  if handle_turn_lanes(way,result,data) == false then return end
  if handle_classification(way,result,data) == false then return end

  -- handle various other flags
  if handle_roundabouts(way,result) == false then return end
  if handle_startpoint(way,result) == false then return end
  if handle_restricted(way,result,data) == false then return end

  -- check if this way can be routed through, but not used as
  -- origin or destination
  if handle_restricted(way,result,data) == false then return end

  -- set name, ref and pronunciation
  if handle_names(way,result) == false then return end
end

function turn_function (angle)
  -- Use a sigmoid function to return a penalty that maxes out at turn_penalty
  -- over the space of 0-180 degrees.  Values here were chosen by fitting
  -- the function to some turn penalty samples from real driving.
  -- multiplying by 10 converts to deci-seconds see issue #1318
  if angle>=0 then
    return 10 * turn_penalty / (1 + 2.718 ^ - ((13 / turn_bias) * angle/180 - 6.5*turn_bias))
  else
    return 10 * turn_penalty / (1 + 2.718 ^  - ((13 * turn_bias) * - angle/180 - 6.5/turn_bias))
  end
end
