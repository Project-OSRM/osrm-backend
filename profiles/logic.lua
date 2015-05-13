require("lib/access")
require("lib/parser")

-- modes
local mode_normal = 1
local mode_ferry = 2

-- directions
local direction_forward = "forward"
local direction_backward = "backward"

-- used by extractor/restriction_parser.cpp
function get_exceptions (vector)
  for i,v in ipairs(restriction_exception_tags) do
    vector:Add(v)
  end
end

-- used by contractor/edge_based_graph_factory.cpp & contractor/processing_chain.cpp
-- function turn_function (angle)
--   -- print ("called at angle " .. angle )
--   local index = math.abs(math.floor(angle/10+0.5))+1 -- +1 'coz LUA starts as idx 1
--   local penalty = turn_cost_table[index]
--   -- print ("index: " .. index .. ", bias: " .. penalty )
--   return penalty
-- end

-- used by extractor/extractor.cpp
function node_function (node, result)
  -- parse access and barrier tags

  local barrier = node:get_value_by_key("barrier")
  if barrier then
    if barrier_whitelist[barrier] then
    else
      result.barrier = true
    end
  end

  if result.barrier ~= true and access_mode == access_mode_explicit_forbiddance then
    local access = find_access_tag(node, access_tags_hierarchy)
    if access and access_tag_blacklist[access] then
      result.barrier = true
    end
  end

  -- check if node is a traffic light
  local tag = node:get_value_by_key("highway")
  if tag and "traffic_signals" == tag then
    --result.duration = traffic_signal_penalty
    result.traffic_lights = true;
  end
end

-- used by extractor/extractor.cpp
function way_function (way, result)
  local highway = way:get_value_by_key("highway")
  local route = way:get_value_by_key("route")

  if not ((highway and highway ~= "") or (route and route ~= "")) then
    return
  end

  -- we don't route over areas
  local area = way:get_value_by_key("area")
  if ignore_areas and area and "yes" == area then
    return
  end

  -- check if oneway tag is unsupported
  local oneway = way:get_value_by_key("oneway")
  if oneway and "reversible" == oneway then
    return
  end

  -- Check if we are allowed to access the way
  local access = find_access_tag(way, access_tags_hierarchy)
  if access_mode == access_mode_explicit_allowance then
    if not access then
	  return
	end
    if access_tag_blacklist[access] then
	  return
	end
  elseif access_mode == access_mode_explicit_forbiddance then
    if access and access_tag_blacklist[access] then
      return
    end
  end

  if weight_check == true then
    -- respect vehicle weight
    local maxweight = parse_weight(way:get_value_by_key("maxweight"))
    if maxweight and 0 < maxweight then
      if vehicle_weight >= maxweight then
        return
      end
    end
  end

  if dimension_check == true then
    -- respect vehicle dimensions
    local maxheight = parse_size(way:get_value_by_key("maxheight"))
    if maxheight and 0 < maxheight then
      if vehicle_height >= maxheight then
	    return
      end
    end
    local maxwidth = parse_size(way:get_value_by_key("maxwidth"))
    if maxwidth and 0 < maxwidth then
      if vehicle_width >= maxwidth then
	    return
      end
    end
    local maxlength = parse_size(way:get_value_by_key("maxlength"))
    if maxlength and 0 < maxlength then
      if vehicle_length >= maxlength then
	    return
      end
    end
  end

  -- handling ferries and piers
  local route_speed = speed_profile[route]
  if(route_speed and route_speed > 0) then
    highway = route;
    local duration  = way:get_value_by_key("duration")
    if duration and durationIsValid(duration) then
      result.duration = math.max( parseDuration(duration), 1 );
    end
    result.forward_mode = mode_ferry
    result.backward_mode = mode_ferry
    result.forward_speed = route_speed
    result.backward_speed = route_speed
  end

  -- leave early of this way is not accessible
  if "" == highway then
    return
  end

  if result.forward_speed == -1 then
    result.forward_speed = get_speed_on_way(way, highway, direction_forward)
	result.backward_speed = get_speed_on_way(way, highway, direction_backward)
  end

  if -1 == result.forward_speed and -1 == result.backward_speed then
    return
  end

  -- limit speed to vehicle capability
  result.forward_speed = speed_limiter(highway, result.forward_speed)
  result.backward_speed = speed_limiter(highway, result.backward_speed)

  -- reduce speed on bad surfaces
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

  -- parse the remaining tags
  local name = way:get_value_by_key("name")
  local ref = way:get_value_by_key("ref")
  local junction = way:get_value_by_key("junction")
  -- local barrier = way:get_value_by_key("barrier", "")
  -- local cycleway = way:get_value_by_key("cycleway", "")
  local service = way:get_value_by_key("service")

  -- Set the name that will be used for instructions
  if ref and "" ~= ref then
    result.name = ref
  elseif name and "" ~= name then
    result.name = name
--  else
      --    result.name = highway  -- if no name exists, use way type
  end

  if junction and "roundabout" == junction then
    result.roundabout = true;
  end

  -- Set access restriction flag if access is allowed under certain restrictions only
  if access ~= "" and access_tag_restricted[access] then
    result.is_access_restricted = true
  end

  -- Set access restriction flag if service is allowed under certain restrictions only
  if service and service ~= "" and service_tag_restricted[service] then
    result.is_access_restricted = true
  end

  -- Set direction according to tags on way
  if obey_oneway then
    if oneway == "-1" then
      result.forward_mode = 0
    elseif oneway == "yes" or
    oneway == "1" or
    oneway == "true" or
    junction == "roundabout" or
    (highway == "motorway_link" and oneway ~="no") or
    (highway == "motorway" and oneway ~= "no") then
      result.backward_mode = 0
    end
  end

  -- Override general direction settings of there is a specific one for our mode of travel
  if ignore_in_grid[highway] then
    result.ignore_in_grid = true
  end

  -- scale speeds to get better avg driving times
  if result.forward_speed > 0 then
    result.forward_speed = result.forward_speed * speed_reduction + speed_reduction_add;
  end
  if result.backward_speed > 0 then
    result.backward_speed = result.backward_speed * speed_reduction + speed_reduction_add;
  end
end

-- These are wrappers to parse vectors of nodes and ways and thus to speed up any tracing JIT
--function node_vector_function (vector)
  --for v in vector.nodes do
    --node_function(v)
  --end
--end

function get_speed_on_way (way, highway, direction)
  local speeds = {}
  speeds["_country"] = nil
  speeds["*"] = nil
  for i,e in ipairs(vehicle_hierarchy) do
	speeds[e] = nil
  end

  -- http://wiki.openstreetmap.org/wiki/Key:zone:maxspeed
  speeds = get_speed_on_way_by_maxspeed(way, highway, speeds, direction, "zone:maxspeed")
  -- http://wiki.openstreetmap.org/wiki/Proposed_features/trafficzone
  speeds = get_speed_on_way_by_maxspeed(way, highway, speeds, direction, "zone:traffic")
  -- http://wiki.openstreetmap.org/wiki/Speed_limits
  -- http://wiki.openstreetmap.org/wiki/Key:maxspeed
  speeds = get_speed_on_way_by_maxspeed(way, highway, speeds, direction, "maxspeed")

  --local maxspeed_lanes = way:get_value_by_key("maxspeed:lanes")

  -- calculate result
  local speed = nil;

  for i,e in ipairs(vehicle_hierarchy) do
    if not speed then
	speed = speeds[e]
    end
  end
  if not speed then
	speed = speeds["*"]
  end

  -- default (try country)
  if not speed then
    if not speeds["_country"] then
	  speeds["_country"] = get_country(way)
    end
    if speeds["_country"] then
      speed = speed_profile[speeds["_country"] .. ":" .. highway]
    end
  end
  -- default
  if not speed then
	speed = speed_profile[highway]
  end

  if not speed then
	speed = -1
  end

  return speed
end

function get_speed_on_way_by_maxspeed (way, highway, speeds, direction, key)
  local speed = parse_speed(highway, way:get_value_by_key(key), speeds)
  if speed then
    speeds["*"] = speed
  end
  speed = parse_speed(highway, way:get_value_by_key(key .. ":" .. direction), speeds)
  if speed then
    speeds["*"] = speed
  end

  for i,e in ipairs(vehicle_hierarchy) do
	speed = parse_speed(highway, way:get_value_by_key(key .. ":" .. e), speeds)
	if speed then
	  speeds[e] = speed
	end
	speed = parse_speed(highway, way:get_value_by_key(key .. ":" .. e .. ":" .. direction), speeds)
	if speed then
      speeds[e] = speed
	end
  end
  return speeds
end

function speed_limiter (highway, speed)
  if not highway then
    return speed
  end
  local maxspeed = vehicle_highway_max_speeds[highway]
  if maxspeed and maxspeed > 0 then
    return math.min(speed, maxspeed)
  end
  maxspeed = vehicle_highway_max_speeds["*"]
  if maxspeed and maxspeed > 0 then
    return math.min(speed, maxspeed)
  end
end

function get_country (way)
  local checker = {
	["source:maxspeed"] = "(%a%a):%a+",
	["tmc"] = "(%a%a):%a+",
	["is_in@de"] = "Germany",
	["source:lit@de"] = "www%.autobahn%-bilder%.de"
  }
  for rule,pattern in pairs(checker) do
    local key = string.match(rule, "([^@]+)@?.*")
	local country_code = string.match(rule, "[^@]+@(.+)")
	local element = way:get_value_by_key(key)
	if element then
	  if country_code then
	    local found = string.find(element, pattern)
		if found then
		  return string.lower(country_code)
		end
	  else
	    local country = string.match(element, pattern)
		if country then
		  return string.lower(country)
		end
	  end
	end
  end
end
