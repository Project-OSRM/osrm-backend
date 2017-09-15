-- Bicycle profile

api_version = 2

Set = require('lib/set')
Sequence = require('lib/sequence')
Handlers = require("lib/way_handlers")
find_access_tag = require("lib/access").find_access_tag
pprint = require('lib/pprint')

Bicycle = {}

function setup()
  local default_speed = 15
  local walking_speed = 6

  local profile = {
    properties = {
      u_turn_penalty                = 20,
      traffic_light_penalty         = 2,
      --weight_name                   = 'cyclability',
      weight_name                   = 'duration',
      process_call_tagless_node     = false,
      max_speed_for_map_matching    = 110/3.6, -- kmph -> m/s
      use_turn_restrictions         = false,
      continue_straight_at_waypoint = false
    },

    default_mode              = mode.cycling,
    default_speed             = default_speed,
    walking_speed             = walking_speed,
    oneway_handling           = true,
    turn_penalty              = 6,
    turn_bias                 = 1.4,
    use_public_transport      = true,

    allowed_start_modes = Set {
      mode.cycling,
      mode.pushing_bike
    },

    implied_oneways = {
      highway = { 'motorway' },
      junction = { 'roundabout', 'circular' }
    },

    -- a list of suffixes to suppress in name change instructions
    suffix_list = {
      'N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW', 'North', 'South', 'West', 'East'
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

    construction_whitelist = Set {
      'no',
      'widening',
      'minor',
    },

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
      'shared',
      'shared_lane'
    },

    -- reduce the driving speed by 30% for unsafe roads
    -- only used for cyclability metric
    unsafe_highway_list = {
      primary = 0.7,
      secondary = 0.75,
      tertiary = 0.8,
      primary_link = 0.7,
      secondary_link = 0.75,
      tertiary_link = 0.8,
    },

    service_penalties = {
      alley             = 0.5,
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
      }
    },

    routes = {
      access_required = true,
      speed = 10,
      keys = {
        railway = {
          mode = mode.train,
          values = Set {
            'train',
            'railway',
            'subway',
            'light_rail',
            'monorail',
            'tram'
          }
        },
        route = {
          values = {
            ferry = {
              speed = 5,
              mode = mode.ferry,
              access_required = false
            }
          }
        }
      }
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
    },

    prefetch = Set {
      'highway',
      'route',
      'railway',
      'bridge',
      'route',
      'man_made',
      'railway',
      'amenity',
      'public_transport',
    }
  }

  -- setup foot profile, used for determining where we can push bikes
  profile.foot_functions = require('foot')
  profile.foot_profile = profile.foot_functions.setup()

  -- adjust foot profile for our needs
  profile.foot_profile.default_mode = mode.pushing_bike
  profile.foot_profile.implied_oneways = profile.implied_oneways

  return profile
end

function process_node(profile, node, result)
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

function Bicycle.cycleway(profile,way,result,data)
  local cycleway = way:get_value_by_key("cycleway")
  local cycleway_left = way:get_value_by_key("cycleway:left")
  local cycleway_right = way:get_value_by_key("cycleway:right")

  data.has_cycleway_left = false
  data.has_cycleway_right = false

  if cycleway_left and profile.cycleway_tags[cycleway_left] then
    data.has_cycleway_left = true
  end
  if cycleway_right and profile.cycleway_tags[cycleway_right] then
    data.has_cycleway_right = true
  end
  if cycleway and string.find(cycleway, "opposite") == 1 then
    if data.is_reverse_oneway then
      data.has_cycleway_right = true
    else
      data.has_cycleway_left = true
    end
  elseif cycleway and profile.cycleway_tags[cycleway] then
    data.has_cycleway_left = not (data.backward_denied or data.is_forward_oneway)
    data.has_cycleway_right = not (data.forward_access or data.is_backward_oneway)
  end

  if data.has_cycleway_left then
    result.backward_mode = profile.default_mode
    result.backward_speed = profile.speeds['highway']['cycleway']
  end

  if data.has_cycleway_right then
    result.forward_mode = profile.default_mode
    result.forward_speed = profile.speeds['highway']['cycleway']
  end
end

-- dismount and push bicycle
function Bicycle.dismount(profile,way,result,data)
  data.bicycle = way:get_value_by_key("bicycle")

  if data.bicycle == "dismount" then
    result.forward_mode = mode.pushing_bike
    result.backward_mode = mode.pushing_bike
    result.forward_speed = profile.walking_speed
    result.backward_speed = profile.walking_speed
  end
end

function Bicycle.weight(profile,way,result,data)
  -- convert duration into cyclability
  if profile.properties.weight_name == 'cyclability' then
    local safety_penalty = profile.unsafe_highway_list[data.highway] or 1.
    local is_unsafe = safety_penalty < 1
    local forward_is_unsafe = is_unsafe and not data.has_cycleway_right
    local backward_is_unsafe = is_unsafe and not data.has_cycleway_left
    local is_undesireable = data.highway == "service" and profile.service_penalties[service]
    local forward_penalty = 1.
    local backward_penalty = 1.

    if forward_is_unsafe then
      forward_penalty = math.min(forward_penalty, safety_penalty)
    end
    if backward_is_unsafe then
       backward_penalty = math.min(backward_penalty, safety_penalty)
    end

    if is_undesireable then
       forward_penalty = math.min(forward_penalty, profile.service_penalties[service])
       backward_penalty = math.min(backward_penalty, profile.service_penalties[service])
    end

    if result.duration > 0 then
      result.weight = result.weight * forward_penalty
    else
      if result.forward_speed > 0 then
        -- convert from km/h to m/s
        result.forward_rate = result.forward_rate * forward_penalty
      end
      if result.backward_speed > 0 then
        -- convert from km/h to m/s
        result.backward_rate = result.backward_rate * backward_penalty
      end
    end
  end
end

function process_way(profile, way, result)
  local data = {}
  local handlers

  handlers = Sequence {
    WayHandlers.prefetch,
    WayHandlers.blocked_ways,
    WayHandlers.access
  }
  if WayHandlers.run(profile,way,result,data,handlers) == false then
    -- if blocked or access denied, we can abort now.
    -- there's no need to check for ferries, trains or pushing of bikes
    return false
  end

  -- run these, but don't about if a handler returns false,
  -- in that case we then check if pushing is possible
  handlers = Sequence {
    WayHandlers.routes,
    WayHandlers.movables,
    WayHandlers.speed,
    WayHandlers.maxspeed,
    WayHandlers.oneway,
    Bicycle.cycleway,
    Bicycle.dismount,
    WayHandlers.surface
  }
  WayHandlers.run( profile,way,result,data,handlers, { no_abort = true } )

  -- try pushing if at least one directions is inaccessible on bike
  if not WayHandlers.is_done(result,data) then
    handlers = Sequence {
      WayHandlers.blocked_ways,
      WayHandlers.access,
      WayHandlers.speed,
      WayHandlers.maxspeed,
      WayHandlers.oneway
    }

    -- store foot result in separate table, then merge
    local foot_result = WayHandlers.new_result()
    if WayHandlers.run(profile.foot_profile,way,foot_result,data,handlers) ~= false then
      WayHandlers.merge_results(foot_result,result)
    end
  end

  -- handle remaining stuff
  handlers = Sequence {
    WayHandlers.roundabouts,
    WayHandlers.penalties,
    Bicycle.weight,
    WayHandlers.classification,
    WayHandlers.startpoint,
    WayHandlers.names,
    WayHandlers.cleanup
  }
  WayHandlers.run(profile,way,result,data,handlers)
end

function process_turn(profile, turn)
  -- compute turn penalty as angle^2, with a left/right bias
  local normalized_angle = turn.angle / 90.0
  if normalized_angle >= 0.0 then
    turn.duration = normalized_angle * normalized_angle * profile.turn_penalty / profile.turn_bias
  else
    turn.duration = normalized_angle * normalized_angle * profile.turn_penalty * profile.turn_bias
  end

  if turn.direction_modifier == direction_modifier.uturn then
    turn.duration = turn.duration + profile.properties.u_turn_penalty
  end

  if turn.has_traffic_light then
     turn.duration = turn.duration + profile.properties.traffic_light_penalty
  end
  if profile.properties.weight_name == 'cyclability' then
    turn.weight = turn.duration
  end
end

return {
  setup = setup,
  process_way = process_way,
  process_node = process_node,
  process_turn = process_turn
}
