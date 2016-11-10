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
function handle_initial_check(data)
  return TagCache.get(data,'highway') ~= nil or
         TagCache.get(data,'route') ~= nil or
         TagCache.get(data,'bridge') ~= nil 
end

-- handle high occupancy vehicle tags
function handle_hov(data)
  -- respect user-preference for HOV-only ways
  if ignore_hov_ways then
    local hov = TagCache.get(data,"hov")
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

    local hov_lanes = TagCache.get(data,"hov:lanes")
    local hov_lanes_forward = TagCache.get(data,"hov:lanes:forward")
    local hov_lanes_backward = TagCache.get(data,"hov:lanes:backward")

    local hov_all_designated = hov_lanes and hov_lanes ~= ""
                               and has_all_designated_hov_lanes(hov_lanes)

    local hov_all_designated_forward = hov_lanes_forward and hov_lanes_forward ~= ""
                                       and has_all_designated_hov_lanes(hov_lanes_forward)

    local hov_all_designated_backward = hov_lanes_backward and hov_lanes_backward ~= ""
                                        and has_all_designated_hov_lanes(hov_lanes_backward)

    -- forward/backward lane depend on a way's direction
    local oneway = TagCache.get(data,"oneway")
    local reverse = oneway and oneway == "-1"

    if hov_all_designated or hov_all_designated_forward then
      if reverse then
        data.result.backward_mode = mode.inaccessible
      else
        data.result.forward_mode = mode.inaccessible
      end
    end

    if hov_all_designated_backward then
      if reverse then
        data.result.forward_mode = mode.inaccessible
      else
        data.result.backward_mode = mode.inaccessible
      end
    end

  end
end

-- handle squares and other areas
function handle_area(data)
  -- we dont route over areas
  local area = TagCache.get(data,"area")
  if ignore_areas and area and "yes" == area then
    return false
  end
end

-- handle toll roads
function handle_toll(data)
  -- respect user-preference for toll=yes ways
  local toll = TagCache.get(data,"toll")
  if ignore_toll_ways and toll and "yes" == toll then
    return false
  end
end

-- handle various that can block access
function handle_blocking(data)
  if handle_area(data) == false then
    return false
  end
  
  if handle_hov(data) == false then
    return false
  end
  
  if handle_toll(data) == false then
    return false
  end

  -- Reversible oneways change direction with low frequency (think twice a day):
  -- do not route over these at all at the moment because of time dependence.
  -- Note: alternating (high frequency) oneways are handled below with penalty.
  local oneway = TagCache.get(data,"oneway")
  if oneway and "reversible" == oneway then
    return false
  end

  local impassable = TagCache.get(data,"impassable")
  if impassable and "yes" == impassable then
    return false
  end

  local status = TagCache.get(data,"status")
  if status and "impassable" == status then
    return false
  end
end

-- set default mode
function handle_default_mode(data)
  data.result.forward_mode = mode.driving
  data.result.backward_mode = mode.driving
end

-- check accessibility by traversing our acces tag hierarchy
function handle_access(data)
  data.access = find_access_tag(data.way, access_tags_hierarchy)
  if access_tag_blacklist[data.access] then
    return false
  end
end

-- handling ferries and piers
function handle_ferries(data)
  local route = TagCache.get(data,"route")
  local route_speed = speed_profile[route]
  if (route_speed and route_speed > 0) then
   TagCache.set(data,"highway",route)
   local duration  = TagCache.get(data,"duration")
   if duration and durationIsValid(duration) then
     data.result.duration = max( parseDuration(duration), 1 )
   end
   data.result.forward_mode = mode.ferry
   data.result.backward_mode = mode.ferry
   data.result.forward_speed = route_speed
   data.result.backward_speed = route_speed
  end
end

-- handling movable bridges
function handle_movables(data)
  local bridge = TagCache.get(data,"bridge")
  local bridge_speed = speed_profile[bridge]
  local capacity_car = TagCache.get(data,"capacity:car")
  if (bridge_speed and bridge_speed > 0) and (capacity_car ~= 0) then
    TagCache.set(data,"highway",bridge)
    local duration  = TagCache.get(data,"duration")
    if duration and durationIsValid(duration) then
      data.result.duration = max( parseDuration(duration), 1 )
    end
    data.result.forward_speed = bridge_speed
    data.result.backward_speed = bridge_speed
  end
end

-- handle speed (excluding maxspeed)
function handle_speed(data)
  if data.result.forward_speed == -1 then
    local highway_speed = speed_profile[TagCache.get(data,"highway")]
    local max_speed = parse_maxspeed( TagCache.get(data,"maxspeed") )
    -- Set the avg speed on the way if it is accessible by road class
    if highway_speed then
      if max_speed and max_speed > highway_speed then
        data.result.forward_speed = max_speed
        data.result.backward_speed = max_speed
        -- max_speed = math.huge
      else
        data.result.forward_speed = highway_speed
        data.result.backward_speed = highway_speed
      end
    else
      -- Set the avg speed on ways that are marked accessible
      if access_tag_whitelist[data.access] then
        data.result.forward_speed = speed_profile["default"]
        data.result.backward_speed = speed_profile["default"]
      end
    end
    if 0 == max_speed then
      max_speed = math.huge
    end
    data.result.forward_speed = min(data.result.forward_speed, max_speed)
    data.result.backward_speed = min(data.result.backward_speed, max_speed)
  end

  if -1 == data.result.forward_speed and -1 == data.result.backward_speed then
    return false
  end
  
  if handle_side_roads(data) == false then return false end
  if handle_surface(data) == false then return false end
  if handle_maxspeed(data) == false then return false end
  if handle_speed_scaling(data) == false then return false end
  if handle_alternating_speed(data) == false then return false end
end

-- reduce speed on special side roads
function handle_side_roads(data)  
  local sideway = TagCache.get(data,"side_road")
  if "yes" == sideway or
  "rotary" == sideway then
    data.result.forward_speed = data.result.forward_speed * side_road_speed_multiplier
    data.result.backward_speed = data.result.backward_speed * side_road_speed_multiplier
  end
end

-- reduce speed on bad surfaces
function handle_surface(data)
  local surface = TagCache.get(data,"surface")
  local tracktype = TagCache.get(data,"tracktype")
  local smoothness = TagCache.get(data,"smoothness")

  if surface and surface_speeds[surface] then
    data.result.forward_speed = math.min(surface_speeds[surface], data.result.forward_speed)
    data.result.backward_speed = math.min(surface_speeds[surface], data.result.backward_speed)
  end
  if tracktype and tracktype_speeds[tracktype] then
    data.result.forward_speed = math.min(tracktype_speeds[tracktype], data.result.forward_speed)
    data.result.backward_speed = math.min(tracktype_speeds[tracktype], data.result.backward_speed)
  end
  if smoothness and smoothness_speeds[smoothness] then
    data.result.forward_speed = math.min(smoothness_speeds[smoothness], data.result.forward_speed)
    data.result.backward_speed = math.min(smoothness_speeds[smoothness], data.result.backward_speed)
  end
end

-- handles name, including ref and pronunciation
function handle_names(data)
  -- parse the remaining tags
  local name = TagCache.get(data,"name")
  local pronunciation = TagCache.get(data,"name:pronunciation")
  local ref = TagCache.get(data,"ref")

  -- Set the name that will be used for instructions
  if name then
    data.result.name = name
  end

  if ref then
    data.result.ref = canonicalizeStringList(ref, ";")
  end

  if pronunciation then
    data.result.pronunciation = pronunciation
  end
end

-- handle turn lanes
function handle_turn_lanes(data)
  local turn_lanes = ""
  local turn_lanes_forward = ""
  local turn_lanes_backward = ""

  turn_lanes, turn_lanes_forward, turn_lanes_backward = get_turn_lanes(data.way)
  if turn_lanes and turn_lanes ~= "" then
    data.result.turn_lanes_forward = turn_lanes;
    data.result.turn_lanes_backward = turn_lanes;
  else
    if turn_lanes_forward and turn_lanes_forward ~= ""  then
      data.result.turn_lanes_forward = turn_lanes_forward;
    end

    if turn_lanes_backward and turn_lanes_backward ~= "" then
      data.result.turn_lanes_backward = turn_lanes_backward;
    end
  end
end

-- junctions
function handle_junctions(data)
  if TagCache.get(data,"junction") == "roundabout" then
    data.result.roundabout = true
  end
end

-- Set access restriction flag if access is allowed under certain restrictions only
function handle_restricted(data)
  if data.access ~= "" and access_tag_restricted[data.access] then
    data.result.is_access_restricted = true
  end
end

-- service roads
function handle_service(data)
  local service = TagCache.get(data,"service")
  if service then
    -- Set access restriction flag if service is allowed under certain restrictions only
    if service_tag_restricted[service] then
      data.result.is_access_restricted = true
    end

    -- Set don't allow access to certain service roads
    if service_tag_forbidden[service] then
      data.result.forward_mode = mode.inaccessible
      data.result.backward_mode = mode.inaccessible
      return false
    end
  end
end

-- scale speeds to get better average driving times
function handle_speed_scaling(data)
  local width = math.huge
  local lanes = math.huge
  if data.result.forward_speed > 0 or data.result.backward_speed > 0 then
    local width_string = TagCache.get(data,"width")
    if width_string and tonumber(width_string:match("%d*")) then
      width = tonumber(width_string:match("%d*"))
    end

    local lanes_string = TagCache.get(data,"lanes")
    if lanes_string and tonumber(lanes_string:match("%d*")) then
      lanes = tonumber(lanes_string:match("%d*"))
    end
  end

  local is_bidirectional = data.result.forward_mode ~= mode.inaccessible and 
                           data.result.backward_mode ~= mode.inaccessible

  local service = TagCache.get(data,"service")
  if data.result.forward_speed > 0 then
    local scaled_speed = data.result.forward_speed * speed_reduction
    local penalized_speed = math.huge
    if service and service ~= "" and service_speeds[service] then
      penalized_speed = service_speeds[service]
    elseif width <= 3 or (lanes <= 1 and is_bidirectional) then
      penalized_speed = data.result.forward_speed / 2
    end
    data.result.forward_speed = math.min(penalized_speed, scaled_speed)
  end

  if data.result.backward_speed > 0 then
    local scaled_speed = data.result.backward_speed * speed_reduction
    local penalized_speed = math.huge
    if service and service ~= "" and service_speeds[service]then
      penalized_speed = service_speeds[service]
    elseif width <= 3 or (lanes <= 1 and is_bidirectional) then
      penalized_speed = data.result.backward_speed / 2
    end
    data.result.backward_speed = math.min(penalized_speed, scaled_speed)
  end
end

-- oneways
function handle_oneway(data)
  local oneway = TagCache.get(data,"oneway")
  if obey_oneway then
    if oneway == "-1" then
      data.result.forward_mode = mode.inaccessible

      local is_forward = false
      local destination = get_destination(data.way, is_forward)
      data.result.destinations = canonicalizeStringList(destination, ",")
    elseif oneway == "yes" or
           oneway == "1" or
           oneway == "true" or
           TagCache.get(data,"junction") == "roundabout" or
           (TagCache.get(data,"highway") == "motorway" and oneway ~= "no") then

      data.result.backward_mode = mode.inaccessible

      local is_forward = true
      local destination = get_destination(data.way, is_forward)
      data.result.destinations = canonicalizeStringList(destination, ",")
    end
  end
end

-- maxspeed and advisory maxspeed
function handle_maxspeed(data)
  -- Override speed settings if explicit forward/backward maxspeeds are given
  local maxspeed_forward = parse_maxspeed(TagCache.get(data,"maxspeed:forward"))
  local maxspeed_backward = parse_maxspeed(TagCache.get(data,"maxspeed:backward"))
  if maxspeed_forward and maxspeed_forward > 0 then
    if mode.inaccessible ~= data.result.forward_mode and 
       mode.inaccessible ~= data.result.backward_mode then
       data.result.backward_speed = data.result.forward_speed
    end
    data.result.forward_speed = maxspeed_forward
  end
  if maxspeed_backward and maxspeed_backward > 0 then
    data.result.backward_speed = maxspeed_backward
  end

  -- Override speed settings if advisory forward/backward maxspeeds are given
  local advisory_speed = parse_maxspeed(TagCache.get(data,"maxspeed:advisory"))
  local advisory_forward = parse_maxspeed(TagCache.get(data,"maxspeed:advisory:forward"))
  local advisory_backward = parse_maxspeed(TagCache.get(data,"maxspeed:advisory:backward"))
  -- apply bi-directional advisory speed first
  if advisory_speed and advisory_speed > 0 then
    if mode.inaccessible ~= data.result.forward_mode then
      data.result.forward_speed = advisory_speed
    end
    if mode.inaccessible ~= data.result.backward_mode then
      data.result.backward_speed = advisory_speed
    end
  end
  if advisory_forward and advisory_forward > 0 then
    if mode.inaccessible ~= data.result.forward_mode and mode.inaccessible ~= data.result.backward_mode then
      data.result.backward_speed = data.result.forward_speed
    end
    data.result.forward_speed = advisory_forward
  end
  if advisory_backward and advisory_backward > 0 then
    data.result.backward_speed = advisory_backward
  end
end

-- Handle high frequency reversible oneways (think traffic signal controlled, changing direction every 15 minutes).
-- Scaling speed to take average waiting time into account plus some more for start / stop.
function handle_alternating_speed(data)
  if "alternating" == TagCache.get(data,'oneway') then
    local scaling_factor = 0.4
    if data.result.forward_speed ~= math.huge then
      data.result.forward_speed = data.result.forward_speed * scaling_factor
    end
    if data.result.backward_speed ~= math.huge then
      data.result.backward_speed = data.result.backward_speed * scaling_factor
    end
  end
end

-- determine if this way can be used as a start/end point for routing
function handle_startpoint(data)
  -- only allow this road as start point if it not a ferry
  data.result.is_startpoint = data.result.forward_mode == mode.driving or 
                              data.result.backward_mode == mode.driving
end

-- leave early if this way is not accessible
function handle_way_type(data)
  if "" == TagCache.get(data,"highway") then return false end
end

-- set the road classification based on guidance globals configuration
function handle_classification(data)
  set_classification(TagCache.get(data,"highway"),data.result,data.way)
end

-- main entry point for processsing a way
function way_function(way, result)
  
  -- define a table that we can pass around to helper functions
  -- so they have access to the input/output objects.
  
  -- we also use a table as a local cache of tags, to avoid calling
  -- into C++ more than once for each tag, without having to
  -- pass lists of already fetches tags around.

  local data = {
    way = way,
    result = result,
    cache = {},
  }
  
  -- perform each procesing step sequentially.
  -- most steps can abort processing, meaning the way
  -- is not routable  
  
  if handle_initial_check(data) == false then return end
  if handle_default_mode(data) == false then return end--
  if handle_blocking(data) == false then return end
  if handle_access(data) == false then return end
  if handle_ferries(data) == false then return end--
  if handle_movables(data) == false then return end--
  if handle_service(data) == false then return end
  if handle_oneway(data) == false then return end--
  if handle_speed(data) == false then return end
  if handle_turn_lanes(data) == false then return end--
  if handle_junctions(data) == false then return end--
  if handle_startpoint(data) == false then return end--
  if handle_restricted(data) == false then return end--
  if handle_classification(data) == false then return end--
  if handle_names(data) == false then return end--
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
