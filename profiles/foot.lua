
-- Foot profile
local http = require("socket.http") -- LuaSocket for HTTP requests
local json = require("cjson") 

api_version = 2

Set = require('lib/set')
Sequence = require('lib/sequence')
Handlers = require("lib/way_handlers")
find_access_tag = require("lib/access").find_access_tag
stations_data = nil


function fetch_pollution_data()
  local url = "http://128.199.51.173:8000/routes/api/pollution/NO2"

  local response, status = http.request(url)

  if status == 200 and response then
    print("Raw response:", response)
    local success, data = pcall(json.decode, response) -- Manejar errores de JSON
    if success and data and data.stations then
      print("Pollution data fetched successfully.")
      print("Number of stations:", #data.stations)
      return data
    else
      print("Failed to decode JSON or missing 'stations' key.")
    end
  else
    print("HTTP request failed. Status:", status)
  end

  -- Fallback a datos vacíos si hay un error
  return { stations = {} }
end



function setup()
  local walking_speed = 5
  stations_data = fetch_pollution_data()

  -- Check if data was successfully retrieved
  if not stations_data or not stations_data.stations then
    print("Warning: Pollution data could not be loaded. Defaulting to no pollution.")
    stations_data = { stations = {} } -- Fallback to empty station data
  else
    print("Pollution data loaded successfully.")
  end
  return {
    properties = {
      weight_name                   = 'duration',
      max_speed_for_map_matching    = 40/3.6, -- kmph -> m/s
      call_tagless_node_function    = false,
      traffic_light_penalty         = 2,
      u_turn_penalty                = 2,
      continue_straight_at_waypoint = false,
      use_turn_restrictions         = false,
    },

    default_mode            = mode.walking,
    default_speed           = walking_speed,
    oneway_handling         = 'specific',     -- respect 'oneway:foot' but not 'oneway'

    barrier_blacklist = Set {
      'yes',
      'wall',
      'fence'
    },

    access_tag_whitelist = Set {
      'yes',
      'foot',
      'permissive',
      'designated'
    },

    access_tag_blacklist = Set {
      'no',
      'agricultural',
      'forestry',
      'private',
      'delivery',
    },

    restricted_access_tag_list = Set { },

    restricted_highway_whitelist = Set { },

    construction_whitelist = Set {},

    access_tags_hierarchy = Sequence {
      'foot',
      'access'
    },

    -- tags disallow access to in combination with highway=service
    service_access_tag_blacklist = Set { },

    restrictions = Sequence {
      'foot'
    },

    -- list of suffixes to suppress in name change instructions
    suffix_list = Set {
      'N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW', 'North', 'South', 'West', 'East'
    },

    avoid = Set {
      'impassable',
      'proposed'
    },

    speeds = Sequence {
      highway = {
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
        platform        = walking_speed,
        footway         = walking_speed,
        pier            = walking_speed,
      },

      railway = {
        platform        = walking_speed
      },

      amenity = {
        parking         = walking_speed,
        parking_entrance= walking_speed
      },

      man_made = {
        pier            = walking_speed
      },

      leisure = {
        track           = walking_speed
      }
    },

    route_speeds = {
      ferry = 5
    },

    bridge_speeds = {
    },

    surface_speeds = {
      fine_gravel =   walking_speed*0.75,
      gravel =        walking_speed*0.75,
      pebblestone =   walking_speed*0.75,
      mud =           walking_speed*0.5,
      sand =          walking_speed*0.5
    },

    tracktype_speeds = {
    },

    smoothness_speeds = {
    }
  }
end

function calculate_pollution(lat, lon)
  -- Calcular contaminación
  local pollution_value = 0
  local total_weight = 0
  local weight = 0
  local p = 1.8
  local max_distance = 3
  if stations_data and stations_data.stations then
    for _, station in ipairs(stations_data.stations) do
      local station_lat = tonumber(station.lat)
      local station_lon = tonumber(station.lon)
      local latest_reading = tonumber(station.pollution)

      if station_lat and station_lon and latest_reading then
        -- Fórmula de distancia usando Haversine
        local R = 6371 -- Radio de la Tierra en km
        local dlat = math.rad(station_lat - lat)
        local dlon = math.rad(station_lon - lon)
        local a = math.sin(dlat / 2)^2 +
                  math.cos(math.rad(lat)) * math.cos(math.rad(station_lat)) * math.sin(dlon / 2)^2
        local c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
        local distance = R * c

      -- Ponderación ajustada
        weight = 1 / ((distance + 1)) 
        weight = tonumber(string.format("%.6f", weight))
        pollution_value = pollution_value + (latest_reading * weight)
        --total_weight = total_weight + weight

      end
    end
    --print(pollution_value)
    return pollution_value
  else
    print("No station data available.")
    return 0
  end
end

function process_node(profile, node, result)
  -- Parse access and barrier tags
  local access = find_access_tag(node, profile.access_tags_hierarchy)
  if access then
    if profile.access_tag_blacklist[access] then
      result.barrier = true
    end
  else
    local barrier = node:get_value_by_key("barrier")
    if barrier then
      -- Make an exception for rising bollard barriers
      local bollard = node:get_value_by_key("bollard")
      local rising_bollard = bollard and "rising" == bollard

      if profile.barrier_blacklist[barrier] and not rising_bollard then
        result.barrier = true
      end
    end
  end
end


-- main entry point for processsing a way
function process_way(profile, way, result)
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
    highway = way:get_value_by_key("highway"),
  }

    -- Verificar si el objeto `way` está definido
  if not way then
      print("Error: way is nil.")
      return
  end


  -- perform an quick initial check and abort if the way is
  -- obviously not routable. here we require at least one
  -- of the prefetched tags to be present, ie. the data table
  -- cannot be empty
  if next(data) == nil then     -- is the data table empty?
    return
  end

  local handlers = Sequence {
    -- set the default mode for this profile. if can be changed later
    -- in case it turns we're e.g. on a ferry
    WayHandlers.default_mode,

    -- check various tags that could indicate that the way is not
    -- routable. this includes things like status=impassable,
    -- toll=yes and oneway=reversible
    WayHandlers.blocked_ways,

    -- determine access status by checking our hierarchy of
    -- access tags, e.g: motorcar, motor_vehicle, vehicle
    WayHandlers.access,

    -- check whether forward/backward directons are routable
    WayHandlers.oneway,

    -- check whether forward/backward directons are routable
    WayHandlers.destinations,

    -- check whether we're using a special transport mode
    WayHandlers.ferries,
    WayHandlers.movables,

    -- compute speed taking into account way type, maxspeed tags, etc.
    WayHandlers.speed,
    WayHandlers.surface,

    -- handle turn lanes and road classification, used for guidance
    WayHandlers.classification,

    -- handle various other flags
    WayHandlers.roundabouts,
    WayHandlers.startpoint,

    -- set name, ref and pronunciation
    WayHandlers.names,

    -- set weight properties of the way
    WayHandlers.weights,
  }

  WayHandlers.run(profile, way, result, data, handlers)
end

function process_turn (profile, turn)
  turn.duration = 0.

  if turn.direction_modifier == direction_modifier.u_turn then
     turn.duration = turn.duration + profile.properties.u_turn_penalty
  end

  if turn.has_traffic_light then
     turn.duration = profile.properties.traffic_light_penalty
  end
  if profile.properties.weight_name == 'routability' then
      -- penalize turns from non-local access only segments onto local access only tags
      if not turn.source_restricted and turn.target_restricted then
          turn.weight = turn.weight + 3000
      end
  end
end

function process_segment(profile, segment)
  -- Extract coordinates of the start and end points
  local source_lat, source_lon = segment.source.lat, segment.source.lon
  local target_lat, target_lon = segment.target.lat, segment.target.lon

  -- Calculate pollution impact at source and target
  local pollution_source = calculate_pollution(source_lat, source_lon)
  local pollution_target = calculate_pollution(target_lat, target_lon)
  

  -- Average pollution for the segment
  local avg_pollution = (pollution_source + pollution_target) / 2
  --print(avg_pollution)

  -- Adjust weight and duration based on pollution level
  segment.weight = segment.weight + (avg_pollution^1.4)
end


return {
  setup = setup,
  process_way =  process_way,
  process_node = process_node,
  process_turn = process_turn,
  process_segment = process_segment
}

