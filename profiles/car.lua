-- Car profile
local find_access_tag = require("lib/access").find_access_tag
local get_destination = require("lib/destination").get_destination
local set_classification = require("lib/guidance").set_classification
local get_turn_lanes = require("lib/guidance").get_turn_lanes

require("lib/tag_cache")

-- Begin of globals
barrier_whitelist = { ["cattle_grid"] = true, ["border_control"] = true, ["checkpoint"] = true, ["toll_booth"] = true, ["sally_port"] = true, ["gate"] = true, ["lift_gate"] = true, ["no"] = true, ["entrance"] = true }
access_tag_whitelist = { ["yes"] = true, ["motorcar"] = true, ["motor_vehicle"] = true, ["vehicle"] = true, ["permissive"] = true, ["designated"] = true, ["destination"] = true }
access_tag_blacklist = { ["no"] = true, ["private"] = true, ["agricultural"] = true, ["forestry"] = true, ["emergency"] = true, ["psv"] = true, ["delivery"] = true }
access_tag_restricted = { ["destination"] = true, ["delivery"] = true }
access_tags_hierarchy = { "motorcar", "motor_vehicle", "vehicle", "access" }
service_tag_restricted = { ["parking_aisle"] = true, ["parking"] = true }
service_tag_forbidden = { ["emergency_access"] = true }
restrictions = { "motorcar", "motor_vehicle", "vehicle" }

-- A list of suffixes to suppress in name change instructions
suffix_list = { "N", "NE", "E", "SE", "S", "SW", "W", "NW", "North", "South", "West", "East" }

speed_profile = {
  ["motorway"] = 90,
  ["motorway_link"] = 45,
  ["trunk"] = 85,
  ["trunk_link"] = 40,
  ["primary"] = 65,
  ["primary_link"] = 30,
  ["secondary"] = 55,
  ["secondary_link"] = 25,
  ["tertiary"] = 40,
  ["tertiary_link"] = 20,
  ["unclassified"] = 25,
  ["residential"] = 25,
  ["living_street"] = 10,
  ["service"] = 15,
--  ["track"] = 5,
  ["ferry"] = 5,
  ["movable"] = 5,
  ["shuttle_train"] = 10,
  ["default"] = 10
}

-- service speeds
service_speeds = {
  ["alley"] = 5,
  ["parking"] = 5,
  ["parking_aisle"] = 5,
  ["driveway"] = 5,
  ["drive-through"] = 5
}

-- surface/trackype/smoothness
-- values were estimated from looking at the photos at the relevant wiki pages

-- max speed for surfaces
surface_speeds = {
  ["asphalt"] = nil,    -- nil mean no limit. removing the line has the same effect
  ["concrete"] = nil,
  ["concrete:plates"] = nil,
  ["concrete:lanes"] = nil,
  ["paved"] = nil,

  ["cement"] = 80,
  ["compacted"] = 80,
  ["fine_gravel"] = 80,

  ["paving_stones"] = 60,
  ["metal"] = 60,
  ["bricks"] = 60,

  ["grass"] = 40,
  ["wood"] = 40,
  ["sett"] = 40,
  ["grass_paver"] = 40,
  ["gravel"] = 40,
  ["unpaved"] = 40,
  ["ground"] = 40,
  ["dirt"] = 40,
  ["pebblestone"] = 40,
  ["tartan"] = 40,

  ["cobblestone"] = 30,
  ["clay"] = 30,

  ["earth"] = 20,
  ["stone"] = 20,
  ["rocky"] = 20,
  ["sand"] = 20,

  ["mud"] = 10
}

-- max speed for tracktypes
tracktype_speeds = {
  ["grade1"] =  60,
  ["grade2"] =  40,
  ["grade3"] =  30,
  ["grade4"] =  25,
  ["grade5"] =  20
}

-- max speed for smoothnesses
smoothness_speeds = {
  ["intermediate"]    =  80,
  ["bad"]             =  40,
  ["very_bad"]        =  20,
  ["horrible"]        =  10,
  ["very_horrible"]   =  5,
  ["impassable"]      =  0
}

-- http://wiki.openstreetmap.org/wiki/Speed_limits
maxspeed_table_default = {
  ["urban"] = 50,
  ["rural"] = 90,
  ["trunk"] = 110,
  ["motorway"] = 130
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
  if access and access ~= "" then
    if access_tag_blacklist[access] then
      result.barrier = true
    end
  else
    local barrier = node:get_value_by_key("barrier")
    if barrier and "" ~= barrier then
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
  if tag and "traffic_signals" == tag then
    result.traffic_lights = true
  end
end

-- abort early if this way is obviouslt not routable
function handle_initial_check(way,result,cache)
  return TagCache.get(way,cache,'highway') ~= nil or
         TagCache.get(way,cache,'route') ~= nil or
         TagCache.get(way,cache,'bridge') ~= nil
end

-- handle high occupancy vehicle tags
function handle_hov(way,result,cache)
  -- respect user-preference for HOV-only ways
  if ignore_hov_ways then
    local hov = TagCache.get(way,cache,"hov")
    if hov and "designated" == hov then
      return false
    end

    -- also respect user-preference for HOV-only ways when all lanes are HOV-designated
    local function has_all_designated_hov_lanes(lanes)
      local all = true
      -- This gmatch call effectively splits the string on | chars.
      -- we append an extra | to the end so that we can match the final part
      for lane in (lanes .. '|'):gmatch("([^|]*)|") do
        if lane and lane ~= "designated" then
          all = false
          break
        end
      end
      return all
    end

    local hov_lanes = TagCache.get(way,cache,"hov:lanes")
    local hov_lanes_forward = TagCache.get(way,cache,"hov:lanes:forward")
    local hov_lanes_backward = TagCache.get(way,cache,"hov:lanes:backward")

    local hov_all_designated = hov_lanes and hov_lanes ~= ""
                               and has_all_designated_hov_lanes(hov_lanes)

    local hov_all_designated_forward = hov_lanes_forward and hov_lanes_forward ~= ""
                                       and has_all_designated_hov_lanes(hov_lanes_forward)

    local hov_all_designated_backward = hov_lanes_backward and hov_lanes_backward ~= ""
                                        and has_all_designated_hov_lanes(hov_lanes_backward)

    -- forward/backward lane depend on a way's direction
    local oneway = TagCache.get(way,cache,"oneway")
    local reverse = oneway and oneway == "-1"

    if hov_all_designated or hov_all_designated_forward then
      if reverse then
        result.backward_mode = mode.inaccessible
      else
        result.forward_mode = mode.inaccessible
      end
    end

    if hov_all_designated_backward then
      if reverse then
        result.forward_mode = mode.inaccessible
      else
        result.backward_mode = mode.inaccessible
      end
    end

  end
end

-- handle squares and other areas
function handle_area(way,result,cache)
  -- we dont route over areas
  local area = TagCache.get(way,cache,"area")
  if ignore_areas and area and "yes" == area then
    return false
  end
end

-- handle toll roads
function handle_toll(way,result,cache)
  -- respect user-preference for toll=yes ways
  local toll = TagCache.get(way,cache,"toll")
  if ignore_toll_ways and toll and "yes" == toll then
    return false
  end
end

-- handle various that can block access
function handle_blocking(way,result,cache)
  if handle_area(way,result,cache) == false then
    return false
  end
  
  if handle_hov(way,result,cache) == false then
    return false
  end
  
  if handle_toll(way,result,cache) == false then
    return false
  end

  -- Reversible oneways change direction with low frequency (think twice a day):
  -- do not route over these at all at the moment because of time dependence.
  -- Note: alternating (high frequency) oneways are handled below with penalty.
  local oneway = TagCache.get(way,cache,"oneway")
  if oneway and "reversible" == oneway then
    return false
  end

  local impassable = TagCache.get(way,cache,"impassable")
  if impassable and "yes" == impassable then
    return false
  end

  local status = TagCache.get(way,cache,"status")
  if status and "impassable" == status then
    return false
  end
end

-- set default mode
function handle_default_mode(way,result,cache)
  result.forward_mode = mode.driving
  result.backward_mode = mode.driving
end

-- check accessibility by traversing our acces tag hierarchy
function handle_access(way,result,cache)
  access = find_access_tag(way, access_tags_hierarchy)
  if access_tag_blacklist[access] then
    return false
  end
end

-- handling ferries and piers
function handle_ferries(way,result,cache)
  local route = TagCache.get(way,cache,"route")
  local route_speed = speed_profile[route]
  if (route_speed and route_speed > 0) then
   TagCache.set(cache,"highway",route)
   local duration  = TagCache.get(way,cache,"duration")
   if duration and durationIsValid(duration) then
     result.duration = max( parseDuration(duration), 1 )
   end
   result.forward_mode = mode.ferry
   result.backward_mode = mode.ferry
   result.forward_speed = route_speed
   result.backward_speed = route_speed
  end
end

-- handling movable bridges
function handle_movables(way,result,cache)
  local bridge = TagCache.get(way,cache,"bridge")
  local bridge_speed = speed_profile[bridge]
  local capacity_car = TagCache.get(way,cache,"capacity:car")
  if (bridge_speed and bridge_speed > 0) and (capacity_car ~= 0) then
    TagCache.set(cache,"highway",bridge)
    local duration  = TagCache.get(way,cache,"duration")
    if duration and durationIsValid(duration) then
      result.duration = max( parseDuration(duration), 1 )
    end
    result.forward_speed = bridge_speed
    result.backward_speed = bridge_speed
  end
end

-- handle speed (excluding maxspeed)
function handle_speed(way,result,cache)
  if result.forward_speed == -1 then
    local highway_speed = speed_profile[TagCache.get(way,cache,"highway")]
    local max_speed = parse_maxspeed( TagCache.get(way,cache,"maxspeed") )
    -- Set the avg speed on the way if it is accessible by road class
    if highway_speed then
      if max_speed and max_speed > highway_speed then
        result.forward_speed = max_speed
        result.backward_speed = max_speed
        -- max_speed = math.huge
      else
        result.forward_speed = highway_speed
        result.backward_speed = highway_speed
      end
    else
      -- Set the avg speed on ways that are marked accessible
      if access_tag_whitelist[access] then
        result.forward_speed = speed_profile["default"]
        result.backward_speed = speed_profile["default"]
      end
    end
    if 0 == max_speed then
      max_speed = math.huge
    end
    result.forward_speed = min(result.forward_speed, max_speed)
    result.backward_speed = min(result.backward_speed, max_speed)
  end

  if -1 == result.forward_speed and -1 == result.backward_speed then
    return false
  end
  
  if handle_side_roads(way,result,cache) == false then return false end
  if handle_surface(way,result,cache) == false then return false end
  if handle_maxspeed(way,result,cache) == false then return false end
  if handle_speed_scaling(way,result,cache) == false then return false end
  if handle_alternating_speed(way,result,cache) == false then return false end
end

-- reduce speed on special side roads
function handle_side_roads(way,result,cache)  
  local sideway = TagCache.get(way,cache,"side_road")
  if "yes" == sideway or
  "rotary" == sideway then
    result.forward_speed = result.forward_speed * side_road_speed_multiplier
    result.backward_speed = result.backward_speed * side_road_speed_multiplier
  end
end

-- reduce speed on bad surfaces
function handle_surface(way,result,cache)
  local surface = TagCache.get(way,cache,"surface")
  local tracktype = TagCache.get(way,cache,"tracktype")
  local smoothness = TagCache.get(way,cache,"smoothness")

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
function handle_names(way,result,cache)
  -- parse the remaining tags
  local name = TagCache.get(way,cache,"name")
  local pronunciation = TagCache.get(way,cache,"name:pronunciation")
  local ref = TagCache.get(way,cache,"ref")

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
function handle_turn_lanes(way,result,cache)
  local turn_lanes = ""
  local turn_lanes_forward = ""
  local turn_lanes_backward = ""

  turn_lanes, turn_lanes_forward, turn_lanes_backward = get_turn_lanes(way)
  if turn_lanes and turn_lanes ~= "" then
    result.turn_lanes_forward = turn_lanes;
    result.turn_lanes_backward = turn_lanes;
  else
    if turn_lanes_forward and turn_lanes_forward ~= ""  then
      result.turn_lanes_forward = turn_lanes_forward;
    end

    if turn_lanes_backward and turn_lanes_backward ~= "" then
      result.turn_lanes_backward = turn_lanes_backward;
    end
  end
end

-- junctions
function handle_junctions(way,result,cache)
  if TagCache.get(way,cache,"junction") == "roundabout" then
    result.roundabout = true
  end
end

-- Set access restriction flag if access is allowed under certain restrictions only
function handle_restricted(way,result,cache)
  if access ~= "" and access_tag_restricted[access] then
    result.is_access_restricted = true
  end
end

-- service roads
function handle_service(way,result,cache)
  local service = TagCache.get(way,cache,"service")
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
function handle_speed_scaling(way,result,cache)
  local width = math.huge
  local lanes = math.huge
  if result.forward_speed > 0 or result.backward_speed > 0 then
    local width_string = TagCache.get(way,cache,"width")
    if width_string and tonumber(width_string:match("%d*")) then
      width = tonumber(width_string:match("%d*"))
    end

    local lanes_string = TagCache.get(way,cache,"lanes")
    if lanes_string and tonumber(lanes_string:match("%d*")) then
      lanes = tonumber(lanes_string:match("%d*"))
    end
  end

  local is_bidirectional = result.forward_mode ~= mode.inaccessible and 
                           result.backward_mode ~= mode.inaccessible

  local service = TagCache.get(way,cache,"service")
  if result.forward_speed > 0 then
    local scaled_speed = result.forward_speed * speed_reduction
    local penalized_speed = math.huge
    if service and service ~= "" and service_speeds[service] then
      penalized_speed = service_speeds[service]
    elseif width <= 3 or (lanes <= 1 and is_bidirectional) then
      penalized_speed = result.forward_speed / 2
    end
    result.forward_speed = math.min(penalized_speed, scaled_speed)
  end

  if result.backward_speed > 0 then
    local scaled_speed = result.backward_speed * speed_reduction
    local penalized_speed = math.huge
    if service and service ~= "" and service_speeds[service]then
      penalized_speed = service_speeds[service]
    elseif width <= 3 or (lanes <= 1 and is_bidirectional) then
      penalized_speed = result.backward_speed / 2
    end
    result.backward_speed = math.min(penalized_speed, scaled_speed)
  end
end

-- oneways
function handle_oneway(way,result,cache)
  local oneway = TagCache.get(way,cache,"oneway")
  if obey_oneway then
    if oneway == "-1" then
      result.forward_mode = mode.inaccessible

      local is_forward = false
      local destination = get_destination(way, is_forward)
      result.destinations = canonicalizeStringList(destination, ",")
    elseif oneway == "yes" or
           oneway == "1" or
           oneway == "true" or
           TagCache.get(way,cache,"junction") == "roundabout" or
           (TagCache.get(way,cache,"highway") == "motorway" and oneway ~= "no") then

      result.backward_mode = mode.inaccessible

      local is_forward = true
      local destination = get_destination(way, is_forward)
      result.destinations = canonicalizeStringList(destination, ",")
    end
  end
end

-- maxspeed and advisory maxspeed
function handle_maxspeed(way,result,cache)
  -- Override speed settings if explicit forward/backward maxspeeds are given
  local maxspeed_forward = parse_maxspeed(TagCache.get(way,cache,"maxspeed:forward"))
  local maxspeed_backward = parse_maxspeed(TagCache.get(way,cache,"maxspeed:backward"))
  if maxspeed_forward and maxspeed_forward > 0 then
    if mode.inaccessible ~= result.forward_mode and 
       mode.inaccessible ~= result.backward_mode then
       result.backward_speed = result.forward_speed
    end
    result.forward_speed = maxspeed_forward
  end
  if maxspeed_backward and maxspeed_backward > 0 then
    result.backward_speed = maxspeed_backward
  end

  -- Override speed settings if advisory forward/backward maxspeeds are given
  local advisory_speed = parse_maxspeed(TagCache.get(way,cache,"maxspeed:advisory"))
  local advisory_forward = parse_maxspeed(TagCache.get(way,cache,"maxspeed:advisory:forward"))
  local advisory_backward = parse_maxspeed(TagCache.get(way,cache,"maxspeed:advisory:backward"))
  -- apply bi-directional advisory speed first
  if advisory_speed and advisory_speed > 0 then
    if mode.inaccessible ~= result.forward_mode then
      result.forward_speed = advisory_speed
    end
    if mode.inaccessible ~= result.backward_mode then
      result.backward_speed = advisory_speed
    end
  end
  if advisory_forward and advisory_forward > 0 then
    if mode.inaccessible ~= result.forward_mode and mode.inaccessible ~= result.backward_mode then
      result.backward_speed = result.forward_speed
    end
    result.forward_speed = advisory_forward
  end
  if advisory_backward and advisory_backward > 0 then
    result.backward_speed = advisory_backward
  end
end

-- Handle high frequency reversible oneways (think traffic signal controlled, changing direction every 15 minutes).
-- Scaling speed to take average waiting time into account plus some more for start / stop.
function handle_alternating_speed(way,result,cache)
  if "alternating" == TagCache.get(way,cache,'oneway') then
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
function handle_startpoint(way,result,cache)
  -- only allow this road as start point if it not a ferry
  result.is_startpoint = result.forward_mode == mode.driving or 
                              result.backward_mode == mode.driving
end

-- leave early if this way is not accessible
function handle_way_type(way,result,cache)
  if "" == TagCache.get(way,cache,"highway") then return false end
end

-- set the road classification based on guidance globals configuration
function handle_classification(way,result,cache)
  set_classification(TagCache.get(way,cache,"highway"),result,way)
end

-- main entry point for processsing a way
function way_function(way, result)
  
  -- define a table that we can pass around to helper functions
  -- so they have access to the input/output objects.
  
  -- we also use a table as a local cache of tags, to avoid calling
  -- into C++ more than once for each tag, without having to
  -- pass lists of already fetches tags around.

  cache = {}
  
  -- perform each procesing step sequentially.
  -- most steps can abort processing, meaning the way
  -- is not routable  
  
  if handle_initial_check(way,result,cache) == false then return end
  if handle_default_mode(way,result,cache) == false then return end--
  if handle_blocking(way,result,cache) == false then return end
  if handle_access(way,result,cache) == false then return end
  if handle_ferries(way,result,cache) == false then return end--
  if handle_movables(way,result,cache) == false then return end--
  if handle_service(way,result,cache) == false then return end
  if handle_oneway(way,result,cache) == false then return end--
  if handle_speed(way,result,cache) == false then return end
  if handle_turn_lanes(way,result,cache) == false then return end--
  if handle_junctions(way,result,cache) == false then return end--
  if handle_startpoint(way,result,cache) == false then return end--
  if handle_restricted(way,result,cache) == false then return end--
  if handle_classification(way,result,cache) == false then return end--
  if handle_names(way,result,cache) == false then return end--
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
