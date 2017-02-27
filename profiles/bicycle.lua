api_version = 1

-- Bicycle profile
local find_access_tag = require("lib/access").find_access_tag
local Set = require('lib/set')
local Sequence = require('lib/sequence')
local Handlers = require("lib/handlers")
local next = next       -- bind to local for speed
local limit = require("lib/maxspeed").limit
local pprint = require('lib/pprint')

-- load foot profile and change default mode to pushing bike
local foot_profile = require('foot')
foot_profile.default_mode = mode.pushing_bike
foot_profile.routes.train.require_whitelisting = true

-- these need to be global because they are accesed externaly
properties.max_speed_for_map_matching    = 110/3.6 -- kmph -> m/s
properties.use_turn_restrictions         = false
properties.continue_straight_at_waypoint = false
properties.weight_name                   = 'duration'
--properties.weight_name                   = 'cyclability'


local default_speed = 15
local walking_speed = 6

local profile = {
  default_mode              = mode.cycling,
  default_speed             = 15,
  speed_reduction           = 1,
  oneway_handling           = true,
  traffic_light_penalty     = 2,
  u_turn_penalty            = 20,
  turn_penalty              = 6,
  turn_bias                 = 1.4,

  -- reduce the driving speed by 30% for unsafe roads
  --safety_penalty            = 0.7,
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

  restrictions = Sequence {
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
    alley = 0.5,
  },

  speeds = Sequence {
    highway = {
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
    amenity = {
      parking = 10,
      parking_entrance = 10
    },
  },
  
  platform_speeds = {
    platform = walking_speed
  },

  routes = {
    ferry = {
      keys = { 'route' },
      speeds = {
        ferry = 5
      }
    },
    
    train = {
      require_whitelisting = true,
      keys = { 'route', 'railway' },
      speeds = {
        train = 10,
        railway = 10,
        subway = 10,
        light_rail = 10,
        monorail = 10,
        tram = 10
      }
    },
    
    default = {
      keys = { 'bridge' },
      speeds = {
        movable = 5
      }
    }
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
    'building',
    'impassable',
    'construction'
  },
  
  maxspeed_increase = false,
  maxspeed_table_default = {
  },

  maxspeed_table = {
  }
}

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
    building = way:get_value_by_key('building'),
    bridge = way:get_value_by_key('bridge'),
    route = way:get_value_by_key('route'),
    leisure = way:get_value_by_key('leisure'),
    man_made = way:get_value_by_key('man_made'),
    railway = way:get_value_by_key('railway'),
    platform = way:get_value_by_key('platform'),
    amenity = way:get_value_by_key('amenity'),
    public_transport = way:get_value_by_key('public_transport'),
    bicycle = way:get_value_by_key("bicycle")
  }

  local bike_result = {}
  bicycle_way_function(data,way,bike_result)
    
  if not Handlers.both_directions_handled(data,bike_result,profile) then
    
    -- one or both directions are not routable by bike.
    -- can we push our bike instead?
    -- bicycle=no forbids both riding and pushing bikes. 
    -- otherwise we use the footprofile to check if we can walk.
    -- if this is the case we assume we can push the bike.
    
    local foot_result = {}
    
    -- prepare the result table, ie. set speeds=-1, mode=inaccessible, etc.
    Handlers.handle_init(way,foot_result,data,foot_profile)
    
    -- call the foot profile
    foot_profile.way_function(way,foot_result)
    
    -- merge with bike result
    Handlers.merge(foot_result,bike_result)
  end
  
  -- output
  Handlers.output(bike_result,result)
end

function handle_cycleways(way,result,data,profile)
  local cycleway = way:get_value_by_key("cycleway")
  local cycleway_left = way:get_value_by_key("cycleway:left")
  local cycleway_right = way:get_value_by_key("cycleway:right")
  
  local common = cycleway and profile.cycleway_tags[cycleway]
  local left = cycleway_left and profile.cycleway_tags[cycleway_left]
  local right = cycleway_right and profile.cycleway_tags[cycleway_right]
  local speed = profile.speeds.highway.cycleway
    
  if cycleway == 'opposite' or cycleway == 'opposite_track' or cycleway == 'opposite_lane' then
    if data.is_reverse_oneway then
      result.forward_mode = mode.cycling
      result.forward_speed = speed
    else
      result.backward_mode = mode.cycling
      result.backward_speed = speed
    end
  elseif common then
    if data.is_forward_oneway then
      result.forward_mode = mode.cycling
      result.forward_speed = speed
    elseif data.is_reverse_oneway then
      result.backward_mode = mode.cycling
      result.backward_speed = speed
    else
      result.backward_mode = mode.cycling
      result.backward_speed = speed
      result.forward_mode = mode.cycling
      result.forward_speed = speed    
    end
  elseif left and right then
    result.backward_mode = mode.cycling
    result.backward_speed = speed
    result.forward_mode = mode.cycling
    result.forward_speed = speed    
  elseif left then
    result.backward_mode = mode.cycling
    result.backward_speed = speed
  elseif right then
    result.forward_mode = mode.cycling
    result.forward_speed = speed    
  end
end

function handle_cyclability(way,result,data,profile)
  -- convert duration into cyclability
  if properties.weight_name == 'cyclability' then
    data.service = way:get_value_by_key("service")
    local is_unsafe = profile.safety_penalty < 1 and profile.unsafe_highway_list[data.highway]
    local is_undesireable = data.highway == "service" and profile.service_penalties[data.service]
    local penalty = 1.0
    if is_unsafe then
      penalty = math.min(penalty, profile.safety_penalty)
    end
    if is_undesireable then
      penalty = math.min(penalty, profile.service_penalties[data.service])
      result.forward_rate = 1
      result.backward_rate = 1
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
end

-- initial routability check
-- quickly filter out buildings, boundaries, etc. to
-- increase processing speed
function handle_routability_check(way,result,data,profile)
   return data.highway or
          data.route or
          data.railway or
          data.amenity or
          data.man_made and
          data.public_transport or
          data.bridge
end

function bicycle_way_function (data,way,result)
  local handlers = Sequence {
    Handlers.handle_init,
    handle_routability_check,
    Handlers.handle_default_mode,
    Handlers.handle_blocked_ways,
    Handlers.handle_access,
    Handlers.handle_dismount,
    Handlers.handle_oneway,
    Handlers.handle_roundabouts,
    handle_cycleways,
    Handlers.handle_routes,
    Handlers.handle_speed,
    Handlers.handle_surface,
    Handlers.handle_maxspeed,
    Handlers.handle_destinations,
    --Handlers.handle_weights,
    handle_cyclability,
    Handlers.handle_classification,
    Handlers.handle_startpoint,
    Handlers.handle_names
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
      -- penalize turns from non-local access only segments onto local access only tags
      if not turn.source_restricted and turn.target_restricted then
          turn.weight = turn.weight + 3000
      end
  end
end

profile.way_function = way_function
return profile