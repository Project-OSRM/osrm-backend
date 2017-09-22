-- Bicycle profile

api_version = 2

Set = require('lib/set')
Sequence = require('lib/sequence')
Handlers = require("lib/way_handlers")
find_access_tag = require("lib/access").find_access_tag
limit = require("lib/maxspeed").limit

function setup()
  local default_speed = 15
  local walking_speed = 6

  return {
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
      footway = walking_speed,
      pedestrian = walking_speed,
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
end

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

function handle_bicycle_tags(profile,way,result,data)
    -- initial routability check, filters out buildings, boundaries, etc
  local route = way:get_value_by_key("route")
  local man_made = way:get_value_by_key("man_made")
  local railway = way:get_value_by_key("railway")
  local amenity = way:get_value_by_key("amenity")
  local public_transport = way:get_value_by_key("public_transport")
  local bridge = way:get_value_by_key("bridge")

  if (not data.highway or data.highway == '') and
  (not route or route == '') and
  (not profile.use_public_transport or not railway or railway=='') and
  (not amenity or amenity=='') and
  (not man_made or man_made=='') and
  (not public_transport or public_transport=='') and
  (not bridge or bridge=='')
  then
    return false
  end

  -- access
  local access = find_access_tag(way, profile.access_tags_hierarchy)
  if access and profile.access_tag_blacklist[access] then
    return false
  end

  -- other tags
  local junction = way:get_value_by_key("junction")
  local maxspeed = parse_maxspeed(way:get_value_by_key ( "maxspeed") )
  local maxspeed_forward = parse_maxspeed(way:get_value_by_key( "maxspeed:forward"))
  local maxspeed_backward = parse_maxspeed(way:get_value_by_key( "maxspeed:backward"))
  local barrier = way:get_value_by_key("barrier")
  local oneway = way:get_value_by_key("oneway")
  local oneway_bicycle = way:get_value_by_key("oneway:bicycle")
  local cycleway = way:get_value_by_key("cycleway")
  local cycleway_left = way:get_value_by_key("cycleway:left")
  local cycleway_right = way:get_value_by_key("cycleway:right")
  local duration = way:get_value_by_key("duration")
  local service = way:get_value_by_key("service")
  local foot = way:get_value_by_key("foot")
  local foot_forward = way:get_value_by_key("foot:forward")
  local foot_backward = way:get_value_by_key("foot:backward")
  local bicycle = way:get_value_by_key("bicycle")


  local way_type_allows_pushing = false

  -- speed
  local bridge_speed = profile.bridge_speeds[bridge]
  if (bridge_speed and bridge_speed > 0) then
    data.highway = bridge
    if duration and durationIsValid(duration) then
      result.duration = math.max( parseDuration(duration), 1 )
    end
    result.forward_speed = bridge_speed
    result.backward_speed = bridge_speed
    way_type_allows_pushing = true
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
    way_type_allows_pushing = true
  -- public_transport platforms (new tagging platform)
  elseif public_transport and profile.platform_speeds[public_transport] then
    result.forward_speed = profile.platform_speeds[public_transport]
    result.backward_speed = profile.platform_speeds[public_transport]
    way_type_allows_pushing = true
  -- railways
  elseif profile.use_public_transport and railway and profile.railway_speeds[railway] and profile.access_tag_whitelist[access] then
    result.forward_mode = mode.train
    result.backward_mode = mode.train
    result.forward_speed = profile.railway_speeds[railway]
    result.backward_speed = profile.railway_speeds[railway]
  elseif amenity and profile.amenity_speeds[amenity] then
    -- parking areas
    result.forward_speed = profile.amenity_speeds[amenity]
    result.backward_speed = profile.amenity_speeds[amenity]
    way_type_allows_pushing = true
  elseif profile.bicycle_speeds[data.highway] then
    -- regular ways
    result.forward_speed = profile.bicycle_speeds[data.highway]
    result.backward_speed = profile.bicycle_speeds[data.highway]
    way_type_allows_pushing = true
  elseif access and profile.access_tag_whitelist[access]  then
    -- unknown way, but valid access tag
    result.forward_speed = profile.default_speed
    result.backward_speed = profile.default_speed
    way_type_allows_pushing = true
  end

  -- oneway
  local implied_oneway = junction == "roundabout" or junction == "circular" or data.highway == "motorway"
  local reverse = false

  if oneway_bicycle == "yes" or oneway_bicycle == "1" or oneway_bicycle == "true" then
    result.backward_mode = mode.inaccessible
  elseif oneway_bicycle == "no" or oneway_bicycle == "0" or oneway_bicycle == "false" then
   -- prevent other cases
  elseif oneway_bicycle == "-1" then
    result.forward_mode = mode.inaccessible
    reverse = true
  elseif oneway == "yes" or oneway == "1" or oneway == "true" then
    result.backward_mode = mode.inaccessible
  elseif oneway == "no" or oneway == "0" or oneway == "false" then
    -- prevent other cases
  elseif oneway == "-1" then
    result.forward_mode = mode.inaccessible
    reverse = true
  elseif implied_oneway then
    result.backward_mode = mode.inaccessible
  end

  -- cycleway
  local has_cycleway_left = false
  local has_cycleway_right = false

  if cycleway_left and profile.cycleway_tags[cycleway_left] then
    has_cycleway_left = true
  end
  if cycleway_right and profile.cycleway_tags[cycleway_right] then
    has_cycleway_right = true
  end
  if cycleway and string.find(cycleway, "opposite") == 1 then
    if reverse then
      has_cycleway_right = true
  else
      has_cycleway_left = true
    end
  elseif cycleway and profile.cycleway_tags[cycleway] then
    -- "cycleway" tag without left/right should not affect a direction
    -- already forbidden by oneway tags
    has_cycleway_left = result.backward_mode ~= mode.inaccessible
    has_cycleway_right = result.forward_mode ~= mode.inaccessible
  end


  if has_cycleway_left then
    result.backward_mode = mode.cycling
    result.backward_speed = profile.bicycle_speeds["cycleway"]
  end

  if has_cycleway_right then
    result.forward_mode = mode.cycling
    result.forward_speed = profile.bicycle_speeds["cycleway"]
  end

  -- pushing bikes - if no other mode found
  if result.forward_mode == mode.inaccessible or result.backward_mode == mode.inaccessible or
    result.forward_speed == -1 or result.backward_speed == -1 then
    if foot ~= 'no' then
      local push_forward_speed = nil
      local push_backward_speed = nil

      if profile.pedestrian_speeds[data.highway] then
        push_forward_speed = profile.pedestrian_speeds[data.highway]
        push_backward_speed = profile.pedestrian_speeds[data.highway]
      elseif man_made and profile.man_made_speeds[man_made] then
        push_forward_speed = profile.man_made_speeds[man_made]
        push_backward_speed = profile.man_made_speeds[man_made]
      else
        if foot == 'yes' then
          push_forward_speed = profile.walking_speed
          if not implied_oneway then
            push_backward_speed = profile.walking_speed
          end
        elseif foot_forward == 'yes' then
          push_forward_speed = profile.walking_speed
        elseif foot_backward == 'yes' then
          push_backward_speed = profile.walking_speed
        elseif way_type_allows_pushing then
          push_forward_speed = profile.walking_speed
          if not implied_oneway then
            push_backward_speed = profile.walking_speed
          end
        end
      end

      if push_forward_speed and (result.forward_mode == mode.inaccessible or result.forward_speed == -1) then
        result.forward_mode = mode.pushing_bike
        result.forward_speed = push_forward_speed
      end
      if push_backward_speed and (result.backward_mode == mode.inaccessible or result.backward_speed == -1)then
        result.backward_mode = mode.pushing_bike
        result.backward_speed = push_backward_speed
      end

    end

  end


  -- dismount
  if bicycle == "dismount" then
    result.forward_mode = mode.pushing_bike
    result.backward_mode = mode.pushing_bike
    result.forward_speed = profile.walking_speed
    result.backward_speed = profile.walking_speed
  end



  -- maxspeed
  limit( result, maxspeed, maxspeed_forward, maxspeed_backward )


  -- not routable if no speed assigned
  -- this avoid assertions in debug builds
  if result.forward_speed <= 0 and result.duration <= 0 then
    result.forward_mode = mode.inaccessible
  end
  if result.backward_speed <= 0 and result.duration <= 0 then
    result.backward_mode = mode.inaccessible
  end


  -- convert duration into cyclability
  if profile.properties.weight_name == 'cyclability' then
      local safety_penalty = profile.unsafe_highway_list[data.highway] or 1.
      local is_unsafe = safety_penalty < 1
      local forward_is_unsafe = is_unsafe and not has_cycleway_right
      local backward_is_unsafe = is_unsafe and not has_cycleway_left
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

      if result.forward_speed > 0 then
        -- convert from km/h to m/s
        result.forward_rate = result.forward_speed / 3.6 * forward_penalty
      end
      if result.backward_speed > 0 then
        -- convert from km/h to m/s
        result.backward_rate = result.backward_speed / 3.6 * backward_penalty
      end
      if result.duration > 0 then
        result.weight = result.duration / forward_penalty
      end
  end
end
function process_way(profile, way, result)
  -- the initial filtering of ways based on presence of tags
  -- affects processing times significantly, because all ways
  -- have to be checked.
  -- to increase performance, prefetching and initial tag check
  -- is done directly instead of via a handler.

  -- in general we should try to abort as soon as
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
    WayHandlers.default_mode,

    -- check various tags that could indicate that the way is not
    -- routable. this includes things like status=impassable,
    -- toll=yes and oneway=reversible
    WayHandlers.blocked_ways,

    -- our main handler
    handle_bicycle_tags,

    -- compute speed taking into account way type, maxspeed tags, etc.
    WayHandlers.surface,

    -- handle turn lanes and road classification, used for guidance
    WayHandlers.classification,

    -- handle allowed start/end modes
    WayHandlers.startpoint,

    -- handle roundabouts
    WayHandlers.roundabouts,

    -- set name, ref and pronunciation
    WayHandlers.names,

    -- set weight properties of the way
    WayHandlers.weights
  }

  WayHandlers.run(profile, way, result, data, handlers)
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
