-- Profile handlers dealing with various aspects of tag parsing
--
-- You can run a selection you find useful in your profile,
-- or do you own processing if/when required.


local get_turn_lanes = require("lib/guidance").get_turn_lanes
local set_classification = require("lib/guidance").set_classification
local get_destination = require("lib/destination").get_destination
local Tags = require('lib/tags')
local Measure = require("lib/measure")

WayHandlers = {}

-- check that way has at least one tag that could imply routability-
-- we store the checked tags in data, to avoid fetching again later
function WayHandlers.tag_prefetch(profile,way,result,data)
  for key,v in pairs(profile.prefetch) do
    data[key] = way:get_value_by_key( key )
  end

  return next(data) ~= nil
end

-- set default mode
function WayHandlers.default_mode(profile,way,result,data)
  result.forward_mode = profile.default_mode
  result.backward_mode = profile.default_mode
end

-- handles name, including ref and pronunciation
function WayHandlers.names(profile,way,result,data)
  -- parse the remaining tags
  local name = way:get_value_by_key("name")
  local pronunciation = way:get_value_by_key("name:pronunciation")
  local ref = way:get_value_by_key("ref")
  local exits = way:get_value_by_key("junction:ref")

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

  if exits then
    result.exits = canonicalizeStringList(exits, ";")
  end
end

-- junctions
function WayHandlers.roundabouts(profile,way,result,data)
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

-- determine if this way can be used as a start/end point for routing
function WayHandlers.startpoint(profile,way,result,data)
  -- if profile specifies set of allowed start modes, then check for that
  -- otherwise require default mode
  if profile.allowed_start_modes then
    result.is_startpoint = profile.allowed_start_modes[result.forward_mode] == true or
                           profile.allowed_start_modes[result.backward_mode] == true
  else
    result.is_startpoint = result.forward_mode == profile.default_mode or
                           result.backward_mode == profile.default_mode
  end
  -- highway=service and access tags check
  local is_service = data.highway == "service"
  if is_service then
    if profile.service_access_tag_blacklist[data.forward_access] then
      result.is_startpoint = false
    end
  end
end

-- handle turn lanes
function WayHandlers.turn_lanes(profile,way,result,data)
  local forward, backward = get_turn_lanes(way,data)

  if forward then
    result.turn_lanes_forward = forward
  end

  if backward then
    result.turn_lanes_backward = backward
  end
end

-- set the road classification based on guidance globals configuration
function WayHandlers.classification(profile,way,result,data)
  set_classification(data.highway,result,way)
end

-- handle destination tags
function WayHandlers.destinations(profile,way,result,data)
  if data.is_forward_oneway or data.is_reverse_oneway then
    local destination = get_destination(way, data.is_forward_oneway)
    result.destinations = canonicalizeStringList(destination, ",")
  end
end

-- handling ferries and piers
function WayHandlers.ferries(profile,way,result,data)
  local route = data.route
  if route then
    local route_speed = profile.route_speeds[route]
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
function WayHandlers.movables(profile,way,result,data)
  local bridge = data.bridge
  if bridge then
    local bridge_speed = profile.bridge_speeds[bridge]
    if bridge_speed and bridge_speed > 0 then
      local capacity_car = way:get_value_by_key("capacity:car")
      if capacity_car ~= 0 then
        result.forward_mode = profile.default_mode
        result.backward_mode = profile.default_mode
        local duration  = way:get_value_by_key("duration")
        if duration and durationIsValid(duration) then
          result.duration = math.max( parseDuration(duration), 1 )
        else
          result.forward_speed = bridge_speed
          result.backward_speed = bridge_speed
        end
      end
    end
  end
end

-- service roads
function WayHandlers.service(profile,way,result,data)
  local service = way:get_value_by_key("service")
  if service then
    -- Set don't allow access to certain service roads
    if profile.service_tag_forbidden[service] then
      result.forward_mode = mode.inaccessible
      result.backward_mode = mode.inaccessible
      return false
    end
  end
end

-- all lanes restricted to hov vehicles?
function WayHandlers.has_all_designated_hov_lanes(lanes)
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
function WayHandlers.hov(profile,way,result,data)
  -- respect user-preference for HOV
  if not profile.avoid.hov_lanes then
    return
  end

  local hov = way:get_value_by_key("hov")
    if "designated" == hov then
      result.forward_restricted = true
      result.backward_restricted = true
  end

  data.hov_lanes_forward, data.hov_lanes_backward = Tags.get_forward_backward_by_key(way,data,'hov:lanes')
  local all_hov_forward = WayHandlers.has_all_designated_hov_lanes(data.hov_lanes_forward)
  local all_hov_backward = WayHandlers.has_all_designated_hov_lanes(data.hov_lanes_backward)

  -- in this case we will use turn penalties instead of filtering out
  if profile.properties.weight_name == 'routability' then
    if (all_hov_forward) then
      result.forward_restricted = true
    end
    if (all_hov_backward) then
      result.backward_restricted = true
    end
    return
  end

  -- filter out ways where all lanes are hov only
  if all_hov_forward then
    result.forward_mode = mode.inaccessible
  end
  if all_hov_backward then
    result.backward_mode = mode.inaccessible
  end
end


-- set highway and access classification by user preference
function WayHandlers.way_classification_for_turn(profile,way,result,data)
  local highway = way:get_value_by_key("highway")
  local access = way:get_value_by_key("access")

  if highway and profile.highway_turn_classification[highway] then
    assert(profile.highway_turn_classification[highway] < 16, "highway_turn_classification must be smaller than 16")
    result.highway_turn_classification = profile.highway_turn_classification[highway]
  end
  if access and profile.access_turn_classification[access] then
    assert(profile.access_turn_classification[access] < 16, "access_turn_classification must be smaller than 16")
    result.access_turn_classification = profile.access_turn_classification[access]
  end
end


-- check accessibility by traversing our access tag hierarchy
function WayHandlers.access(profile,way,result,data)
  data.forward_access, data.backward_access =
    Tags.get_forward_backward_by_set(way,data,profile.access_tags_hierarchy)

  -- only allow a subset of roads to be treated as restricted
  if profile.restricted_highway_whitelist[data.highway] then
      if profile.restricted_access_tag_list[data.forward_access] then
          result.forward_restricted = true
      end

      if profile.restricted_access_tag_list[data.backward_access] then
          result.backward_restricted = true
      end
  end

  -- blacklist access tags that aren't marked as restricted
  if profile.access_tag_blacklist[data.forward_access] and not result.forward_restricted then
    result.forward_mode = mode.inaccessible
  end

  if profile.access_tag_blacklist[data.backward_access] and not result.backward_restricted then
    result.backward_mode = mode.inaccessible
  end

  if result.forward_mode == mode.inaccessible and result.backward_mode == mode.inaccessible then
    return false
  end
end

-- handle speed (excluding maxspeed)
function WayHandlers.speed(profile,way,result,data)
  if result.forward_speed ~= -1 then
    return        -- abort if already set, eg. by a route
  end

  local key,value,speed = Tags.get_constant_by_key_value(way,profile.speeds)

  if speed then
    -- set speed by way type
    result.forward_speed = speed
    result.backward_speed = speed
  else
    -- Set the avg speed on ways that are marked accessible
    if profile.access_tag_whitelist[data.forward_access] then
      result.forward_speed = profile.default_speed
    elseif data.forward_access and not profile.access_tag_blacklist[data.forward_access] then
      result.forward_speed = profile.default_speed -- fallback to the avg speed if access tag is not blacklisted
    elseif not data.forward_access and data.backward_access then
       result.forward_mode = mode.inaccessible
    end

    if profile.access_tag_whitelist[data.backward_access] then
      result.backward_speed = profile.default_speed
    elseif data.backward_access and not profile.access_tag_blacklist[data.backward_access] then
      result.backward_speed = profile.default_speed -- fallback to the avg speed if access tag is not blacklisted
    elseif not data.backward_access and data.forward_access then
       result.backward_mode = mode.inaccessible
    end
  end

  if result.forward_speed == -1 and result.backward_speed == -1 and result.duration <= 0 then
    return false
  end
end

-- add class information
function WayHandlers.classes(profile,way,result,data)
    if not profile.classes then
        return
    end

    local allowed_classes = Set {}
    for k, v in pairs(profile.classes) do
        allowed_classes[v] = true
    end

    local forward_toll, backward_toll = Tags.get_forward_backward_by_key(way, data, "toll")
    local forward_route, backward_route = Tags.get_forward_backward_by_key(way, data, "route")
    local tunnel = way:get_value_by_key("tunnel")

    if allowed_classes["tunnel"] and tunnel and tunnel ~= "no" then
      result.forward_classes["tunnel"] = true
      result.backward_classes["tunnel"] = true
    end

    if allowed_classes["toll"] and forward_toll == "yes" then
        result.forward_classes["toll"] = true
    end
    if allowed_classes["toll"] and backward_toll == "yes" then
        result.backward_classes["toll"] = true
    end

    if allowed_classes["ferry"] and forward_route == "ferry" then
        result.forward_classes["ferry"] = true
    end
    if allowed_classes["ferry"] and backward_route == "ferry" then
        result.backward_classes["ferry"] = true
    end

    if allowed_classes["restricted"] and result.forward_restricted then
        result.forward_classes["restricted"] = true
    end
    if allowed_classes["restricted"] and result.backward_restricted then
        result.backward_classes["restricted"] = true
    end

    if allowed_classes["motorway"] and (data.highway == "motorway" or data.highway == "motorway_link") then
        result.forward_classes["motorway"] = true
        result.backward_classes["motorway"] = true
    end
end

-- reduce speed on bad surfaces
function WayHandlers.surface(profile,way,result,data)
  local surface = way:get_value_by_key("surface")
  local tracktype = way:get_value_by_key("tracktype")
  local smoothness = way:get_value_by_key("smoothness")

  if surface and profile.surface_speeds[surface] then
    result.forward_speed = math.min(profile.surface_speeds[surface], result.forward_speed)
    result.backward_speed = math.min(profile.surface_speeds[surface], result.backward_speed)
  end
  if tracktype and profile.tracktype_speeds[tracktype] then
    result.forward_speed = math.min(profile.tracktype_speeds[tracktype], result.forward_speed)
    result.backward_speed = math.min(profile.tracktype_speeds[tracktype], result.backward_speed)
  end
  if smoothness and profile.smoothness_speeds[smoothness] then
    result.forward_speed = math.min(profile.smoothness_speeds[smoothness], result.forward_speed)
    result.backward_speed = math.min(profile.smoothness_speeds[smoothness], result.backward_speed)
  end
end

-- scale speeds to get better average driving times
function WayHandlers.penalties(profile,way,result,data)
  -- heavily penalize a way tagged with all HOV lanes
  -- in order to only route over them if there is no other option
  local service_penalty = 1.0
  local service = way:get_value_by_key("service")
  if service and profile.service_penalties[service] then
    service_penalty = profile.service_penalties[service]
  end

  local width_penalty = 1.0
  local width = math.huge
  local lanes = math.huge
  local width_string = way:get_value_by_key("width")
  if width_string and tonumber(width_string:match("%d*")) then
    width = tonumber(width_string:match("%d*"))
  end

  local lanes_string = way:get_value_by_key("lanes")
  if lanes_string and tonumber(lanes_string:match("%d*")) then
    lanes = tonumber(lanes_string:match("%d*"))
  end

  local is_bidirectional = result.forward_mode ~= mode.inaccessible and
                           result.backward_mode ~= mode.inaccessible

  if width <= 3 or (lanes <= 1 and is_bidirectional) then
    width_penalty = 0.5
  end

  -- Handle high frequency reversible oneways (think traffic signal controlled, changing direction every 15 minutes).
  -- Scaling speed to take average waiting time into account plus some more for start / stop.
  local alternating_penalty = 1.0
  if data.oneway == "alternating" then
    alternating_penalty = 0.4
  end

  local sideroad_penalty = 1.0
  data.sideroad = way:get_value_by_key("side_road")
  if "yes" == data.sideroad or "rotary" == data.sideroad then
    sideroad_penalty = profile.side_road_multiplier
  end

  local forward_penalty = math.min(service_penalty, width_penalty, alternating_penalty, sideroad_penalty)
  local backward_penalty = math.min(service_penalty, width_penalty, alternating_penalty, sideroad_penalty)

  if profile.properties.weight_name == 'routability' then
    if result.forward_speed > 0 then
      result.forward_rate = (result.forward_speed * forward_penalty) / 3.6
    end
    if result.backward_speed > 0 then
      result.backward_rate = (result.backward_speed * backward_penalty) / 3.6
    end
    if result.duration > 0 then
      result.weight = result.duration / forward_penalty
    end
  end
end

-- maxspeed and advisory maxspeed
function WayHandlers.maxspeed(profile,way,result,data)
  local keys = Sequence {  'maxspeed:advisory', 'maxspeed', 'source:maxspeed', 'maxspeed:type' }
  local forward, backward = Tags.get_forward_backward_by_set(way,data,keys)
  forward = WayHandlers.parse_maxspeed(forward,profile)
  backward = WayHandlers.parse_maxspeed(backward,profile)

  if forward and forward > 0 then
    result.forward_speed = forward * profile.speed_reduction
  end

  if backward and backward > 0 then
    result.backward_speed = backward * profile.speed_reduction
  end
end

function WayHandlers.parse_maxspeed(source,profile)
  if not source then
    return 0
  end

  local n = Measure.get_max_speed(source)
  if not n then
    -- parse maxspeed like FR:urban
    source = string.lower(source)
    n = profile.maxspeed_table[source]
    if not n then
      local highway_type = string.match(source, "%a%a:(%a+)")
      n = profile.maxspeed_table_default[highway_type]
      if not n then
        n = 0
      end
    end
  end
  return n
end

-- handle maxheight tags
function WayHandlers.handle_height(profile,way,result,data)
  local keys = Sequence { 'maxheight:physical', 'maxheight' }
  local forward, backward = Tags.get_forward_backward_by_set(way,data,keys)
  forward = Measure.get_max_height(forward,way)
  backward = Measure.get_max_height(backward,way)

  if forward and forward < profile.vehicle_height then
    result.forward_mode = mode.inaccessible
  end

  if backward and backward < profile.vehicle_height then
    result.backward_mode = mode.inaccessible
  end
end

-- handle maxwidth tags
function WayHandlers.handle_width(profile,way,result,data)
  local keys = Sequence { 'maxwidth:physical', 'maxwidth', 'width', 'est_width' }
  local forward, backward = Tags.get_forward_backward_by_set(way,data,keys)
  local narrow = way:get_value_by_key('narrow')

  if ((forward and forward == 'narrow') or (narrow and narrow == 'yes')) and profile.vehicle_width > 2.2 then
    result.forward_mode = mode.inaccessible
  elseif forward then
    forward = Measure.get_max_width(forward)
    if forward and forward <= profile.vehicle_width then
      result.forward_mode = mode.inaccessible
    end
  end

  if ((backward and backward == 'narrow') or (narrow and narrow == 'yes')) and profile.vehicle_width > 2.2 then
    result.backward_mode = mode.inaccessible
  elseif backward then
    backward = Measure.get_max_width(backward)
    if backward and backward <= profile.vehicle_width then
      result.backward_mode = mode.inaccessible
    end
  end
end

-- handle maxweight tags
function WayHandlers.handle_weight(profile,way,result,data)
  local keys = Sequence { 'maxweight' }
  local forward, backward = Tags.get_forward_backward_by_set(way,data,keys)
  forward = Measure.get_max_weight(forward)
  backward = Measure.get_max_weight(backward)

  if forward and forward < profile.vehicle_weight then
    result.forward_mode = mode.inaccessible
  end

  if backward and backward < profile.vehicle_weight then
    result.backward_mode = mode.inaccessible
  end
end

-- handle maxlength tags
function WayHandlers.handle_length(profile,way,result,data)
  local keys = Sequence { 'maxlength' }
  local forward, backward = Tags.get_forward_backward_by_set(way,data,keys)
  forward = Measure.get_max_length(forward)
  backward = Measure.get_max_length(backward)

  if forward and forward < profile.vehicle_length then
    result.forward_mode = mode.inaccessible
  end

  if backward and backward < profile.vehicle_length then
    result.backward_mode = mode.inaccessible
  end
end

-- handle oneways tags
function WayHandlers.oneway(profile,way,result,data)
  if not profile.oneway_handling then
    return
  end

  local oneway
  if profile.oneway_handling == true then
    oneway = Tags.get_value_by_prefixed_sequence(way,profile.restrictions,'oneway') or way:get_value_by_key("oneway")
  elseif profile.oneway_handling == 'specific' then
    oneway = Tags.get_value_by_prefixed_sequence(way,profile.restrictions,'oneway')
  elseif profile.oneway_handling == 'conditional' then
    -- Following code assumes that `oneway` and `oneway:conditional` tags have opposite values and takes weakest (always `no`).
    -- So if we will have:
    -- oneway=yes, oneway:conditional=no @ (condition1)
    -- oneway=no, oneway:conditional=yes @ (condition2)
    -- condition1 will be always true and condition2 will be always false.
    if way:get_value_by_key("oneway:conditional") then
        oneway = "no"
    else
        oneway = Tags.get_value_by_prefixed_sequence(way,profile.restrictions,'oneway') or way:get_value_by_key("oneway")
    end
  end

  data.oneway = oneway

  if oneway == "-1" then
    data.is_reverse_oneway = true
    result.forward_mode = mode.inaccessible
  elseif oneway == "yes" or
         oneway == "1" or
         oneway == "true" then
    data.is_forward_oneway = true
    result.backward_mode = mode.inaccessible
  elseif profile.oneway_handling == true then
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

function WayHandlers.weights(profile,way,result,data)
  if profile.properties.weight_name == 'distance' then
    result.weight = -1
     -- set weight rates to 1 for the distance weight, edge weights are distance / rate
    if (result.forward_mode ~= mode.inaccessible and result.forward_speed > 0) then
       result.forward_rate = 1
    end
    if (result.backward_mode ~= mode.inaccessible and result.backward_speed > 0) then
       result.backward_rate = 1
    end
  end
end


-- handle general avoid rules

function WayHandlers.avoid_ways(profile,way,result,data)
  if profile.avoid[data.highway] then
    return false
  end
end

-- handle various that can block access
function WayHandlers.blocked_ways(profile,way,result,data)

  -- areas
  if profile.avoid.area and way:get_value_by_key("area") == "yes" then
    return false
  end

  -- toll roads
  if profile.avoid.toll and way:get_value_by_key("toll") == "yes" then
    return false
  end

  -- don't route over steps
  if profile.avoid.steps and data.highway == "steps" then
    return false
  end

  -- construction
  -- TODO if highway is valid then we shouldn't check railway, and vica versa
  if profile.avoid.construction and (data.highway == 'construction' or way:get_value_by_key('railway') == 'construction') then
    return false
  end

  -- In addition to the highway=construction tag above handle the construction=* tag
  -- http://wiki.openstreetmap.org/wiki/Key:construction
  -- https://taginfo.openstreetmap.org/keys/construction#values
  if profile.avoid.construction then
    local construction = way:get_value_by_key('construction')

    -- Of course there are negative tags to handle, too
    if construction and not profile.construction_whitelist[construction] then
      return false
    end
  end

  -- Not only are there multiple construction tags there is also a proposed=* tag.
  -- http://wiki.openstreetmap.org/wiki/Key:proposed
  -- https://taginfo.openstreetmap.org/keys/proposed#values
  if profile.avoid.proposed and way:get_value_by_key('proposed') then
    return false
  end

  -- Reversible oneways change direction with low frequency (think twice a day):
  -- do not route over these at all at the moment because of time dependence.
  -- Note: alternating (high frequency) oneways are handled below with penalty.
  if profile.avoid.reversible and way:get_value_by_key("oneway") == "reversible" then
    return false
  end

  -- impassables
  if profile.avoid.impassable  then
    if way:get_value_by_key("impassable") == "yes" then
      return false
    end

    if way:get_value_by_key("status") == "impassable" then
      return false
    end
  end
end

function WayHandlers.driving_side(profile, way, result, data)
   local driving_side = way:get_value_by_key('driving_side')
   if driving_side == nil then
      driving_side = way:get_location_tag('driving_side')
   end

   if driving_side == 'left' then
      result.is_left_hand_driving = true
   elseif driving_side == 'right' then
      result.is_left_hand_driving = false
   else
      result.is_left_hand_driving = profile.properties.left_hand_driving
   end
end


-- Call a sequence of handlers, aborting in case a handler returns false. Example:
--
-- handlers = Sequence {
--   WayHandlers.tag_prefetch,
--   WayHandlers.default_mode,
--   WayHandlers.blocked_ways,
--   WayHandlers.access,
--   WayHandlers.speed,
--   WayHandlers.names
-- }
--
-- WayHandlers.run(handlers,way,result,data,profile)
--
-- Each method in the list will be called on the WayHandlers object.
-- All handlers must accept the parameteres (profile, way, result, data, relations) and return false
-- if the handler chain should be aborted.
-- To ensure the correct order of method calls, use a Sequence of handler names.

function WayHandlers.run(profile, way, result, data, handlers, relations)
  for i,handler in ipairs(handlers) do
    if handler(profile, way, result, data, relations) == false then
      return false
    end
  end
end

return WayHandlers
