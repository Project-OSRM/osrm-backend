api_version = 1

-- Bicycle profile
local find_access_tag = require("lib/access").find_access_tag
local Set = require('lib/set')
local Tags = require('lib/tags')
local Sequence = require('lib/sequence')
local Handlers = require("lib/handlers")
local next = next       -- bind to local for speed
local limit = require("lib/maxspeed").limit

-- these need to be global because they are accesed externaly
properties.max_speed_for_map_matching    = 110/3.6 -- kmph -> m/s
properties.use_turn_restrictions         = false
properties.continue_straight_at_waypoint = false
properties.weight_name                   = 'duration'
--properties.weight_name                   = 'cyclability'

-- Set to true if you need to call the node_function for every node.
-- Generally can be left as false to avoid unnecessary Lua calls
-- (which slow down pre-processing).
properties.call_tagless_node_function      = false


local default_speed = 15
local walking_speed = 6

local profile = {
  default_mode              = mode.cycling,
  default_speed             = 15,
  oneway_handling           = true,
  traffic_light_penalty     = 2,
  u_turn_penalty            = 20,
  turn_penalty              = 6,
  turn_bias                 = 1.4,

  -- reduce the driving speed by 30% for unsafe roads
  -- local safety_penalty            = 0.7,
  safety_penalty            = 1.0,
  use_public_transport      = true,

  allowed_start_modes = Set {
    mode.cycling,
    mode.pushing_bike
  },

  barrier_whitelist = Set {
    'sump_buster',
    'bus_trap',
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
  },

  access_tag_whitelist = Set {
  	'yes',
  	'permissive',
   	'designated'
  },

  access_tag_blacklist = Set {
  	'no',
   	'private',
   	'agricultural',
   	'forestry',
   	'delivery'
  },

  restricted_access_tag_list = Set { },

  restricted_highway_whitelist = Set { },

  access_tags_hierarchy = Sequence {
  	'bicycle',
  	'vehicle',
  	'access'
  },

  access_tags_push_hierarchy = Sequence {
    'foot',
  	'access'
  },

  restrictions = Set {
  	'bicycle'
  },

  cycleway_tags = Set {
  	'track',
  	'lane',
  	'opposite',
  	'opposite_lane',
  	'opposite_track',
  	'share_busway',
  	'sharrow',
  	'shared'
  },

  unsafe_highway_list = Set {
  	'primary',
   	'secondary',
   	'tertiary',
   	'primary_link',
   	'secondary_link',
   	'tertiary_link'
  },

  service_penalties = {
    alley             = 0.5,
  },

  bicycle_speeds = {
    cycleway = default_speed,
    primary = default_speed,
    primary_link = default_speed,
    secondary = default_speed,
    secondary_link = default_speed,
    tertiary = default_speed,
    tertiary_link = default_speed,
    residential = default_speed,
    unclassified = default_speed,
    living_street = default_speed,
    road = default_speed,
    service = default_speed,
    track = 12,
    path = 12
  },

  pedestrian_speeds = {
    primary         = walking_speed,
    primary_link    = walking_speed,
    secondary       = walking_speed,
    secondary_link  = walking_speed,
    tertiary        = walking_speed,
    tertiary_link   = walking_speed,
    unclassified    = walking_speed,
    residential     = walking_speed,
    road            = walking_speed,
    living_street   = walking_speed,
    service         = walking_speed,
    track           = walking_speed,
    path            = walking_speed,
    steps           = walking_speed,
    pedestrian      = walking_speed,
    footway         = walking_speed,
    pier            = walking_speed,
    steps = 2
  },

  railway_speeds = {
    train = 10,
    railway = 10,
    subway = 10,
    light_rail = 10,
    monorail = 10,
    tram = 10
  },

  platform_speeds = {
    platform = walking_speed
  },

  amenity_speeds = {
    parking = 10,
    parking_entrance = 10
  },

  man_made_speeds = {
    pier = walking_speed
  },

  route_speeds = {
    ferry = 5
  },

  bridge_speeds = {
    movable = 5
  },

  surface_speeds = {
    asphalt = default_speed,
    ["cobblestone:flattened"] = 10,
    paving_stones = 10,
    compacted = 10,
    cobblestone = 6,
    unpaved = 6,
    fine_gravel = 6,
    gravel = 6,
    pebblestone = 6,
    ground = 6,
    dirt = 6,
    earth = 6,
    grass = 6,
    mud = 3,
    sand = 3,
    sett = 10
  },

  tracktype_speeds = {
  },

  smoothness_speeds = {
  },

  avoid = Set {
    'impassable',
    'construction'
  }
}


local function parse_maxspeed(source)
    if not source then
        return 0
    end
    local n = tonumber(source:match("%d*"))
    if not n then
        n = 0
    end
    if string.match(source, "mph") or string.match(source, "mp/h") then
        n = (n*1609)/1000
    end
    return n
end

function get_restrictions(vector)
  for i,v in ipairs(profile.restrictions) do
    vector:Add(v)
  end
end

function node_function (node, result)
  -- parse access and barrier tags
  local highway = node:get_value_by_key("highway")
  local is_crossing = highway and highway == "crossing"

  local access = find_access_tag(node, profile.access_tags_hierarchy)
  if access and access ~= "" then
    -- access restrictions on crossing nodes are not relevant for
    -- the traffic on the road
    if profile.access_tag_blacklist[access] and not is_crossing then
      result.barrier = true
    end
  else
    local barrier = node:get_value_by_key("barrier")
    if barrier and "" ~= barrier then
      if not profile.barrier_whitelist[barrier] then
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

function way_function (way, result)
  -- the intial filtering of ways based on presence of tags
  -- affects processing times significantly, because all ways
  -- have to be checked.
  -- to increase performance, prefetching and intial tag check
  -- is done in directly instead of via a handler.

  -- in general we should  try to abort as soon as
  -- possible if the way is not routable, to avoid doing
  -- unnecessary work. this implies we should check things that
  -- commonly forbids access early, and handle edge cases later.

  -- data table for storing intermediate values during processing

  local data = {
    -- prefetch tags
    highway = way:get_value_by_key('highway'),
  }

  local handlers = Sequence {
    -- set the default mode for this profile. if can be changed later
    -- in case it turns we're e.g. on a ferry
    'handle_default_mode',

    -- check various tags that could indicate that the way is not
    -- routable. this includes things like status=impassable,
    -- toll=yes and oneway=reversible
    'handle_blocked_ways',
  }

  if Handlers.run(handlers,way,result,data,profile) == false then
    return
  end

  -- initial routability check, filters out buildings, boundaries, etc
  local route = way:get_value_by_key("route")
  local man_made = way:get_value_by_key("man_made")
  local railway = way:get_value_by_key("railway")
  local amenity = way:get_value_by_key("amenity")
  local public_transport = way:get_value_by_key("public_transport")
  local bridge = way:get_value_by_key("bridge")
  local junction = way:get_value_by_key("junction")

  if (not data.highway or data.highway == '') and
  (not route or route == '') and
  (not profile.use_public_transport or not railway or railway=='') and
  (not amenity or amenity=='') and
  (not man_made or man_made=='') and
  (not public_transport or public_transport=='') and
  (not bridge or bridge=='')
  then
    return
  end

  -- access
  local forward_access, backward_access =
    Tags.get_forward_backward_by_set(way,data,profile.access_tags_hierarchy)
  if forward_access and profile.access_tag_blacklist[forward_access] then
      result.forward_mode = mode.pushing_bike
  else
      result.forward_mode = mode.cycling
  end
  if backward_access and profile.access_tag_blacklist[backward_access] then
      result.backward_mode = mode.pushing_bike
 else
      result.backward_mode = mode.cycling
  end

  local in_roundabout = junction == "roundabout" or junction == "circular"
  local forward_push_access, backward_push_access =
    Tags.get_forward_backward_by_set(way,data,profile.access_tags_push_hierarchy)
  if result.forward_mode == mode.pushing_bike then
    if in_roundabout or (forward_push_access and profile.access_tag_blacklist[forward_push_access]) then
        result.forward_mode = mode.inaccessible
    end
  end
  if result.backward_mode == mode.pushing_bike then
    if in_roundabout or (backward_push_access and profile.access_tag_blacklist[backward_push_access]) then
        result.backward_mode = mode.inaccessible
    end
  end

  -- we don't want to allow walking in traffic circles, exit early
  if result.forward_mode == mode.inaccessible and result.backward_mode == mode.inaccessible then
    return
  end

  -- other tags
  local maxspeed = parse_maxspeed(way:get_value_by_key ( "maxspeed") )
  local maxspeed_forward = parse_maxspeed(way:get_value_by_key( "maxspeed:forward"))
  local maxspeed_backward = parse_maxspeed(way:get_value_by_key( "maxspeed:backward"))
  local barrier = way:get_value_by_key("barrier")
  local oneway = way:get_value_by_key("oneway")
  local onewayClass = way:get_value_by_key("oneway:bicycle")
  local cycleway = way:get_value_by_key("cycleway")
  local cycleway_left = way:get_value_by_key("cycleway:left")
  local cycleway_right = way:get_value_by_key("cycleway:right")
  local duration = way:get_value_by_key("duration")
  local service = way:get_value_by_key("service")

  -- speed
  local bridge_speed = profile.bridge_speeds[bridge]
  if (bridge_speed and bridge_speed > 0) then
    data.highway = bridge
    if duration and durationIsValid(duration) then
      result.duration = math.max( parseDuration(duration), 1 )
    end
    result.forward_speed = bridge_speed
    result.backward_speed = bridge_speed
  elseif profile.route_speeds[route] then
    -- ferries (doesn't cover routes tagged using relations)
    result.forward_mode = mode.ferry
    result.backward_mode = mode.ferry
    if duration and durationIsValid(duration) then
      result.duration = math.max( 1, parseDuration(duration) )
    else
       result.forward_speed = profile.route_speeds[route]
       result.backward_speed = profile.route_speeds[route]
    end
  -- railway platforms (old tagging scheme)
  elseif railway and profile.platform_speeds[railway] then
    result.forward_speed = profile.platform_speeds[railway]
    result.backward_speed = profile.platform_speeds[railway]
    result.forward_mode = mode.pushing_bike
    result.backward_mode = mode.pushing_bike
  -- public_transport platforms (new tagging platform)
  elseif public_transport and profile.platform_speeds[public_transport] then
    result.forward_speed = profile.platform_speeds[public_transport]
    result.backward_speed = profile.platform_speeds[public_transport]
    result.forward_mode = mode.pushing_bike
    result.backward_mode = mode.pushing_bike
  -- railways
  elseif profile.use_public_transport and railway and profile.railway_speeds[railway] then
    if forward_access and profile.access_tag_whitelist[forward_access] then
      result.forward_mode = mode.forward_mode ~= mode.inaccessible and mode.train or mode.inaccessible
      result.forward_speed = profile.railway_speeds[railway]
    end
    if backward_access and profile.access_tag_whitelist[backward_access] then
      result.backward_mode = mode.backward_mode ~= mode.inaccessible and mode.train or mode.inaccessible
      result.backward_speed = profile.railway_speeds[railway]
    end
  elseif amenity and profile.amenity_speeds[amenity] then
    -- parking areas
    result.forward_speed = profile.amenity_speeds[amenity]
    result.backward_speed = profile.amenity_speeds[amenity]
  elseif profile.bicycle_speeds[data.highway] then
    -- regular ways
    result.forward_speed = profile.bicycle_speeds[data.highway]
    result.backward_speed = profile.bicycle_speeds[data.highway]
  else
    -- unknown highway type, check if its on the whitelist, otherwise throw it out
    if forward_access and profile.access_tag_whitelist[forward_access]  then
      result.forward_mode = mode.cycling
      result.forward_speed = default_speed
    else
      if profile.pedestrian_speeds[data.highway] or profile.access_tag_whitelist[forward_push_access] then
        result.forward_mode = mode.pushing_bike
        result.forward_speed = walking_speed
      else
        result.forward_mode = mode.inaccessible
      end
    end
    if backward_access and profile.access_tag_whitelist[backward_access]  then
        result.backward_mode = mode.cycling
        result.backward_speed = default_speed
    else
      if profile.pedestrian_speeds[data.highway] or profile.access_tag_whitelist[backward_push_access] then
        result.backward_mode = mode.pushing_bike
      else
        result.backward_mode = mode.inaccessible
      end
    end
  end

  -- cycleways
  local has_cycleway_forward = false
  local has_cycleway_backward = false
  local has_cycleway_left= false
  local has_cycleway_right = false
  if cycleway and profile.cycleway_tags[cycleway] then
    has_cycleway_left = true
    has_cycleway_right = true
  elseif cycleway_left and profile.cycleway_tags[cycleway_left] then
    has_cycleway_left = true
  elseif cycleway_right and profile.cycleway_tags[cycleway_right] then
    has_cycleway_right = true
  end

  -- direction
  local impliedOneway = false
  if junction == "roundabout" or junction == "circular" or data.highway == "motorway" then
    impliedOneway = true
  end

  if onewayClass == "yes" or onewayClass == "1" or onewayClass == "true" then
    result.backward_mode = mode.pushing_bike
    result.forward_mode = mode.cycling
    -- in this case we don't care on which side the cycleway is
    has_cycleway_forward = false
    has_cycleway_backward = false
  elseif onewayClass == "no" or onewayClass == "0" or onewayClass == "false" or
    -- opposite works like oneway:bicycle=no
    (cycleway and cycleway == "opposite") or
    (cycleway_left and cycleway_left == "opposite") or
    (cycleway_right and cycleway_right == "opposite") then
    -- FIXME assumes right-handed traffic
    if has_cycleway_left then
        has_cycleway_backward = true
    end
    if has_cycleway_right then
        has_cycleway_forward = true
    end
  elseif onewayClass == "-1" or
      (cycleway and (cycleway == "opposite_lane" or cycleway == "opposite_track")) or
      (cycleway_left and (cycleway_left == "opposite_lane" or cycleway_left == "opposite_track")) or
      (cycleway_right and (cycleway_right == "opposite_lane" or cycleway_right == "opposite_track")) then
    result.forward_mode = mode.pushing_bike
    result.backward_mode = mode.cycling
    has_cycleway_forward = false
  elseif oneway == "no" or oneway == "0" or oneway == "false" then
    -- FIXME assumes right-handed traffic
    if has_cycleway_left then
        has_cycleway_backward = true
    end
    if has_cycleway_right then
        has_cycleway_forward = true
    end
  elseif oneway == "-1" then
    result.forward_mode = mode.pushing_bike
    has_cycleway_forward = false
  elseif oneway == "yes" or oneway == "1" or oneway == "true" or impliedOneway then
    result.backward_mode = mode.pushing_bike
    has_cycleway_backward = false
  end

  if has_cycleway_forward then
    result.forward_mode = mode.cycling
    result.forward_speed = profile.bicycle_speeds["cycleway"]
  end
  if has_cycleway_backward then
    result.backward_mode = mode.cycling
    result.backward_speed = profile.bicycle_speeds["cycleway"]
  end

  -- apply dismount override or check again of we need to adjust accessibility
  if forward_access == "dismount" then
    result.forward_mode = mode.pushing_bike
    result.forward_speed = walking_speed
  elseif result.forward_mode == mode.pushing_bike then
      if in_roundabout or (forward_push_access and profile.access_tag_blacklist[forward_push_access]) then
          result.forward_mode = mode.inaccessible
      end
  end
  if backward_access == "dismount" then
    result.backward_mode = mode.pushing_bike
    result.backward_speed = walking_speed
  elseif result.backward_mode == mode.pushing_bike then
      if in_roundabout or (backward_push_access and profile.access_tag_blacklist[backward_push_access]) then
          result.backward_mode = mode.inaccessible
      end
  end

  -- biking not allowed, maybe we can push our bike?
  -- essentially requires pedestrian profiling, for example foot=no mean we can't push a bike
  if result.forward_mode == mode.pushing_bike or result.backward_mode == mode.pushing_bike then
    local forward_push_speed = -1
    local backward_push_speed = -1
    if railway and profile.platform_speeds[railway] then
      forward_push_speed = profile.platform_speeds[railway]
      backward_push_speed = profile.platform_speeds[railway]
    elseif public_transport and profile.platform_speeds[public_transport] then
      forward_push_speed = profile.platform_speeds[public_transport]
      backward_push_speed = profile.platform_speeds[public_transport]
    elseif profile.pedestrian_speeds[data.highway] then
      -- pedestrian-only ways and areas
      forward_push_speed = profile.pedestrian_speeds[data.highway]
      backward_push_speed = profile.pedestrian_speeds[data.highway]
    elseif man_made and profile.man_made_speeds[man_made] then
      -- man made structures
      forward_push_speed = profile.man_made_speeds[man_made]
      backward_push_speed = profile.man_made_speeds[man_made]
    else
        if profile.access_tag_whitelist[forward_push_access] then
            forward_push_speed = walking_speed
        elseif result.forward_mode == mode.pushing_bike then
            result.forward_mode = mode.inaccessible
        end
        if profile.access_tag_whitelist[backward_push_access] then
            backward_push_speed = walking_speed
        elseif result.backward_mode == mode.pushing_bike then
            result.backward_mode = mode.inaccessible
        end
    end
    if result.forward_mode == mode.pushing_bike then
        result.forward_speed = forward_push_speed
    end
    if result.backward_mode == mode.pushing_bike then
        result.backward_speed = backward_push_speed
    end
  end

  -- maxspeed
  limit( result, maxspeed, maxspeed_forward, maxspeed_backward )

  -- convert duration into cyclability
  if properties.weight_name == 'cyclability' then
      local is_unsafe = profile.safety_penalty < 1 and profile.unsafe_highway_list[data.highway]
      local is_undesireable = data.highway == "service" and profile.service_penalties[service]
      local penalty = 1.0
      if is_unsafe then
        penalty = math.min(penalty, profile.safety_penalty)
      end
      if is_undesireable then
        penalty = math.min(penalty, profile.service_penalties[service])
      end

      if result.forward_speed > 0 then
        -- convert from km/h to m/s
        result.forward_rate = result.forward_speed / 3.6 * penalty
      end
      if result.backward_speed > 0 then
        -- convert from km/h to m/s
        result.backward_rate = result.backward_speed / 3.6 * penalty
      end
      if result.duration > 0 then
        result.weight = result.duration / penalty
      end
  end

  local handlers = Sequence {
    -- compute speed taking into account way type, maxspeed tags, etc.
    'handle_surface',

    -- handle turn lanes and road classification, used for guidance
    'handle_classification',

    -- handle various other flags
    'handle_roundabouts',
    --'handle_startpoint',

    -- set name, ref and pronunciation
    'handle_names'
  }

  Handlers.run(handlers,way,result,data,profile)

end

function turn_function(turn)
  -- compute turn penalty as angle^2, with a left/right bias
  local normalized_angle = turn.angle / 90.0
  if normalized_angle >= 0.0 then
    turn.duration = normalized_angle * normalized_angle * profile.turn_penalty / profile.turn_bias
  else
    turn.duration = normalized_angle * normalized_angle * profile.turn_penalty * profile.turn_bias
  end

  if turn.direction_modifier == direction_modifier.uturn then
    turn.duration = turn.duration + profile.u_turn_penalty
  end

  if turn.has_traffic_light then
     turn.duration = turn.duration + profile.traffic_light_penalty
  end
  if properties.weight_name == 'cyclability' then
      turn.weight = turn.duration
      -- penalize turns from non-local access only segments onto local access only tags
      if not turn.source_restricted and turn.target_restricted then
          turn.weight = turn.weight + 3000
      end
  end
end
