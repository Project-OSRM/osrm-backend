api_version = 1

-- Foot profile
local find_access_tag = require("lib/access").find_access_tag
local get_destination = require("lib/destination").get_destination
local set_classification = require("lib/guidance").set_classification
local Set = require('lib/set')
local Sequence = require('lib/sequence')
local Directional = require('lib/directional')

barrier_whitelist = Set {
  'cycle_barrier',
  'bollard',
  'entrance',
  'cattle_grid',
  'border_control',
  'toll_booth',
  'sally_port',
  'gate',
  'no',
  'block'
}
access_tag_whitelist = Set {
  'yes',
  'foot',
  'permissive',
  'designated'
}
access_tag_blacklist = Set {
  'no',
  'private',
  'agricultural',
  'forestry',
  'delivery'
}

access_tags_hierarchy = Sequence {
  'foot',
  'access'
}

restrictions = Sequence { 'foot' }

-- A list of suffixes to suppress in name change instructions
-- Note: a Set does not work here because it's read from C++
suffix_list = {
  'N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW', 'North', 'South', 'West', 'East'
}

walking_speed = 5

speed_profile = {
  primary = walking_speed,
  primary_link = walking_speed,
  secondary = walking_speed,
  secondary_link = walking_speed,
  tertiary = walking_speed,
  tertiary_link = walking_speed,
  unclassified = walking_speed,
  residential = walking_speed,
  road = walking_speed,
  living_street = walking_speed,
  service = walking_speed,
  track = walking_speed,
  path = walking_speed,
  steps = walking_speed,
  pedestrian = walking_speed,
  footway = walking_speed,
  pier = walking_speed,
  default = walking_speed
}

route_speeds = {
  ferry = 5
}

platform_speeds = {
  platform = walking_speed
}

amenity_speeds = {
  parking = walking_speed,
  parking_entrance = walking_speed
}

man_made_speeds = {
  pier = walking_speed
}

surface_speeds = {
  fine_gravel =   walking_speed*0.75,
  gravel =        walking_speed*0.75,
  pebblestone =   walking_speed*0.75,
  mud =           walking_speed*0.5,
  sand =          walking_speed*0.5
}

tracktype_speeds = {}

smoothness_speeds = {}

leisure_speeds = {
  track = walking_speed
}

properties.max_speed_for_map_matching    = 40/3.6 -- kmph -> m/s
properties.use_turn_restrictions         = false
properties.continue_straight_at_waypoint = false
properties.weight_name                   = 'duration'

local traffic_light_penalty              = 2
local u_turn_penalty                     = 2



-- setting oneway_handling to 'specific' means that the plain 'oneway' tag is ignored,
-- but oneway:foot (or or more specific modes) is respected.

local oneway_handling   = 'specific'

local ignore_areas      = false


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
  data.leisure = way:get_value_by_key("leisure")
  data.route = way:get_value_by_key("route")
  data.bridge = way:get_value_by_key("bridge")
  data.man_made = way:get_value_by_key("man_made")
  data.railway = way:get_value_by_key("railway")
  data.platform = way:get_value_by_key("platform")
  data.amenity = way:get_value_by_key("amenity")
  data.public_transport = way:get_value_by_key("public_transport")

  return data.highway ~= nil or
         data.leisure ~= nil or
         data.route ~= nil or
         data.bridge ~= nil or
         data.railway ~= nil or
         data.platform ~= nil or
         data.amenity ~= nil or
         data.man_made ~= nil
end

-- handle various that can block access
function is_way_blocked(way,result)
  -- we dont route over areas
  local area = way:get_value_by_key("area")
  if ignore_areas and "yes" == area then
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
  result.forward_mode = mode.walking
  result.backward_mode = mode.walking
end

-- check accessibility by traversing our acces tag hierarchy
function handle_access(way,result,data)
  data.forward_access, data.backward_access =
    Directional.get_values_by_set(way,data,access_tags_hierarchy)

  if access_tag_blacklist[data.forward_access] then
    result.forward_mode = mode.inaccessible
  end

  if access_tag_blacklist[data.backward_access] then
    result.backward_mode = mode.inaccessible
  end

  if result.forward_mode == mode.inaccessible and result.backward_mode == mode.inaccessible then
    return false
  end
end

-- handling ferries and piers
function handle_ferries(way,result)
  local route = way:get_value_by_key("route")
  if route then
    local route_speed = route_speeds[route]
    if route_speed and route_speed > 0 then
     local duration  = way:get_value_by_key("duration")
     if duration and durationIsValid(duration) then
       result.duration = math.max( parseDuration(duration), 1 )
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
    local speed = speed_profile[data.highway] or
                  platform_speeds[data.railway] or      -- old tagging scheme
                  platform_speeds[data.platform] or
                  amenity_speeds[data.amenity] or
                  man_made_speeds[data.man_made] or
                  leisure_speeds[data.leisure]

    if speed then
      -- set speed by way type
      result.forward_speed = highway_speed
      result.backward_speed = highway_speed
      result.forward_speed = speed
      result.backward_speed = speed
    else
      -- Set the avg speed on ways that are marked accessible
      if access_tag_whitelist[data.forward_access] then
        result.forward_speed = speed_profile["default"]
      end

      if access_tag_whitelist[data.backward_access] then
        result.backward_speed = speed_profile["default"]
      end
    end
  end

  if -1 == result.forward_speed and -1 == result.backward_speed then
    return false
  end

  if handle_surface(way,result) == false then return false end
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

-- handle oneways tags
function handle_oneway(way,result,data)
  local oneway
  if oneway_handling == true then
    oneway = Directional.get_value_by_prefixed_sequence(way,restrictions,'oneway') or way:get_value_by_key('oneway')
  elseif oneway_handling == 'specific' then
    oneway = Directional.get_value_by_prefixed_sequence(way,restrictions,'oneway')
  end

  data.oneway = oneway

  if oneway then
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

-- determine if this way can be used as a start/end point for routing
function handle_startpoint(way,result)
  -- only allow this road as start point if it not a ferry
  result.is_startpoint = result.forward_mode == mode.walking or
                              result.backward_mode == mode.walking
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

  -- compute speed taking into account way type, maxspeed tags, etc.
  if handle_speed(way,result,data) == false then return end

  -- handle turn lanes and road classification, used for guidance
  if handle_classification(way,result,data) == false then return end

  -- handle various other flags
  if handle_roundabouts(way,result) == false then return end
  if handle_startpoint(way,result) == false then return end

  -- set name, ref and pronunciation
  if handle_names(way,result) == false then return end
end

function turn_function (turn)
  turn.duration = 0.

  if turn.direction_modifier == direction_modifier.u_turn then
     turn.duration = turn.duration + u_turn_penalty
  end

  if turn.has_traffic_light then
     turn.duration = traffic_light_penalty
  end
end
