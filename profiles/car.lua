-- Car profile

api_version = 4

Set = require('lib/set')
Sequence = require('lib/sequence')
Handlers = require("lib/way_handlers")
Relations = require("lib/relations")
Obstacles = require("lib/obstacles")
find_access_tag = require("lib/access").find_access_tag
resolve_access = require("lib/access").resolve_access
limit = require("lib/maxspeed").limit
Utils = require("lib/utils")
Measure = require("lib/measure")
Co2 = require("lib/co2")

local access_tags = {'motorcar', 'motor_vehicle', 'vehicle', 'access'}

function setup()
  return {
    properties = {
      max_speed_for_map_matching      = 180/3.6, -- 180kmph -> m/s
      -- For routing based on duration, but weighted for preferring certain roads
      weight_name                     = 'routability',
      -- For shortest duration without penalties for accessibility
      -- weight_name                     = 'duration',
      -- For shortest distance without penalties for accessibility
      -- weight_name                     = 'distance',
      -- For lowest CO2 emissions (grams, see the co2 table below) without penalties for accessibility
      -- weight_name                     = 'co2',
      -- For lowest CO2 emissions, weighted like 'routability' for preferring certain roads
      -- weight_name                     = 'routability-co2',
      process_call_tagless_node      = false,
      u_turn_penalty                 = 20,
      continue_straight_at_waypoint  = true,
      use_turn_restrictions          = true,
      -- left_hand_driving is deliberately unset. Setting it either way forces
      -- that side on every way the OSM tags and location data leave open;
      -- leaving it unset lets the driving side index answer per way.
    },

    default_mode              = mode.driving,
    default_speed             = 10,
    oneway_handling           = true,
    side_road_multiplier      = 0.8,
    turn_penalty              = 7.5,
    speed_reduction           = 0.8,
    turn_bias                 = 1.075,
    -- Seconds added to a turn that crosses oncoming traffic: a left turn where
    -- traffic drives on the right, a right turn where it drives on the left.
    -- Set to 0 to cost turns purely by turn_bias.
    oncoming_turn_penalty     = 2.0,
    cardinal_directions       = false,
    -- Additional seconds for u-turns at tagged turning facilities.
    turning_facility_penalty  = 5,

    -- Penalty multiplier for roads with no lane markings (lane_markings=no)
    -- Applied to bidirectional roads to prefer roads with clear lane markings
    lane_markings_penalty     = 0.75,

    -- Penalty multiplier for the disadvantaged direction on ways tagged 'priority=forward'/'priority=backward'.
    -- Applies to the per-direction rate (speed). A value < 1 reduces the disadvantaged
    -- direction's rate which increases its routing weight (weight ≈ duration / rate).
    -- This is applied to any way with a 'priority' tag; add an explicit width/lanes
    -- guard if the penalty should only target narrow or single-lane roads.
    priority_penalty          = 0.7,

    -- Size of the vehicle, to be limited by physical restriction of the way
    vehicle_height = 2.0, -- in meters, 2.0m is the height slightly above biggest SUVs
    vehicle_width = 1.9, -- in meters, ways with narrow tag are considered narrower than 2.2m

    -- Size of the vehicle, to be limited mostly by legal restriction of the way
    vehicle_length = 4.8, -- in meters, 4.8m is the length of large or family car
    vehicle_weight = 2000, -- in kilograms

    -- CO2 emission model, used only by the 'co2' and 'routability-co2' weights.
    -- The weight unit is the gram of CO2.
    co2 = {
      -- Emission factor (g CO2/km) by travel speed (km/h), linearly interpolated
      -- and clamped outside the range.
      --
      -- Default: COPERT hot emissions of a medium petrol passenger car, Euro 6 d/e.
      -- Source: EMEP/EEA air pollutant emission inventory guidebook, chapter
      -- 1.A.3.b.i-iv Road transport, Appendix 4 "Emission Factors" (July 2026
      -- file, COPERT 5.10.1), sheet HOT_EMISSIONS_PARAMETERS, row
      -- Passenger Cars / Petrol / Medium / Euro 6 d/e / pollutant EC (energy
      -- consumption). The GDI, PFI and GDI+GPF technologies share the same row.
      --
      -- 1. Energy consumption in MJ/km, guidebook equation (25), valid 5-130 km/h:
      --      EC(V) = (A*V^2 + B*V + C + D/V) / (E*V^2 + F*V + G) * (1 - RF)
      --      A = 1.31723006778538e-4    B = 5.4859592952839e-3   C = 2.6191950512566
      --      D = 1.72774612390592e-9    E = -8.50415012699799e-5 F = 2.3584384067406e-2
      --      G = 0.344297577833879      RF = 0.23196729231897276
      -- 2. Conversion to CO2 with the guidebook values for petrol:
      --      g CO2/km = EC / 43.774 MJ/kg (Table 3-28) * 3.169 kg CO2/kg fuel (Table 3-29) * 1000
      -- 3. Sampled at the speeds below and rounded to 0.1 g/km. The linear
      --    interpolation stays within 1.4 % of the formula between 5 and 130 km/h.
      --
      -- These are hot emissions on a flat road with an average load: no cold
      -- start, no slope, no traffic. To model another vehicle, apply steps 1-3 to
      -- its row of the same sheet (e.g. Light Commercial Vehicles / Diesel /
      -- N1-III for a large diesel van, with 42.695 MJ/kg for diesel).
      curve = {
        {   5, 320.2 },
        {  10, 261.4 },
        {  15, 223.7 },
        {  20, 197.8 },
        {  30, 165.5 },
        {  40, 147.2 },
        {  50, 136.7 },
        {  60, 131.0 },
        {  70, 128.5 },
        {  80, 128.6 },
        {  90, 130.7 },
        { 100, 134.6 },
        { 110, 140.2 },
        { 120, 147.6 },
        { 130, 156.6 },
      },
      -- Emissions per second spent waiting at turns, traffic signals, stops and
      -- barriers, and on ways with a fixed duration. COPERT has no idle factor
      -- (its curves start at 5 km/h). 0.5 g/s is a petrol engine idling at about
      -- 0.8 l/h: 0.8 l/h * 0.75 kg/l * 3.169 kg CO2/kg / 3600 = 0.53 g/s. This is
      -- consistent with the curve above at 5 km/h: 320.2 g/km * 5 / 3600 = 0.44 g/s.
      idle_g_per_s = 0.5,
      -- Emissions allocated to the vehicle per km on board a ferry (g CO2/km), for
      -- a car with its driver only.
      -- Source: UK DESNZ greenhouse gas conversion factors 2026 (flat file),
      -- "Business travel- sea / Ferry", CO2 per passenger.km:
      --   foot passenger 0.01848 kg, car passenger 0.12774 kg.
      -- DESNZ allocates the CO2 of mixed passenger and freight (RoPax) ferries by
      -- transported mass, with 1.25 t per car and 0.1 t per passenger and luggage
      -- (methodology report 2026, section 7.5-7.10, Table 31). Its car passenger
      -- factor is per person, the car being shared between its occupants.
      -- The factor per vehicle is derived from the same mass allocation:
      -- 1. CO2 per tonne.km = 0.01848 kg / 0.1 t = 0.1848 kg (foot passenger)
      -- 2. car + driver = (1.25 t + 0.1 t) * 0.1848 kg = 0.249 kg per km -> 250 g/km
      -- Check: the car passenger factor corresponds to 2.1 occupants per car,
      -- (0.1 + 1.25 / 2.1) * 0.1848 = 0.128 kg.
      -- Limits: DESNZ data comes from UK RoPax routes (2007 study). Fast ferries
      -- and passenger-only ferries emit significantly more. Shuttle trains
      -- (route=shuttle_train) also use this factor, although they are electric.
      ferry_g_per_km = 250,
    },

    -- Optional: upper limit for all speeds (e.g., 87 for trucks)
    -- When set, no derived speed will exceed this value
    -- When nil (default), no additional capping is applied
    vehicle_max_speed = nil, -- in km/h

    -- a list of suffixes to suppress in name change instructions. The suffixes also include common substrings of each other
    suffix_list = {
      'N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW', 'North', 'South', 'West', 'East', 'Nor', 'Sou', 'We', 'Ea'
    },

    barrier_whitelist = Set {
      'cattle_grid',
      'border_control',
      'toll_booth',
      'sally_port',
      'no',
      'entrance',
      'height_restrictor',
      'arch'
    },

    access_tag_whitelist = Set {
      'yes',
      'motorcar',
      'motor_vehicle',
      'vehicle',
      'permissive',
      'designated',
      'hov'
    },

    access_tag_blacklist = Set {
      'no',
      'agricultural',
      'forestry',
      'emergency',
      'psv',
      'taxi', -- sub class of psv
      'share_taxi', -- sub class of psv
      'minibus', -- sub class of psv
      'bus', -- sub class of psv
      'foot',
      'emergency_vehicle',
      'restricted',
      'military',
      'official',
      'customers',
      'private',
      'delivery',
      'destination',
      'permit',
      'residents'
    },

    -- tags disallow access to in combination with highway=service
    service_access_tag_blacklist = Set {
        'private'
    },

    restricted_access_tag_list = Set {
      'private',
      'delivery',
      'destination',
      'customers',
      'permit',
      'residents',
      'unknown',
    },

    access_tags_hierarchy = Sequence {table.unpack(access_tags)},

    service_tag_forbidden = Set {
      'emergency_access'
    },

    restrictions = Sequence {
      'motorcar',
      'motor_vehicle',
      'vehicle'
    },

    classes = Sequence {
        'toll', 'motorway', 'ferry', 'restricted', 'tunnel'
    },

    -- classes to support for exclude flags
    excludable = Sequence {
        Set {'toll'},
        Set {'motorway'},
        Set {'ferry'}
    },

    avoid = Set {
      'area',
      -- 'toll',    -- uncomment this to avoid tolls
      'reversible',
      'impassable',
      'hov_lanes',
      'steps',
      'construction',
      'proposed'
    },

    speeds = Sequence {
      highway = {
        motorway        = 90,
        motorway_link   = 45,
        trunk           = 85,
        trunk_link      = 40,
        primary         = 65,
        primary_link    = 30,
        secondary       = 55,
        secondary_link  = 25,
        tertiary        = 40,
        tertiary_link   = 20,
        unclassified    = 25,
        residential     = 25,
        living_street   = 10,
        service         = 15,
        -- winter highway types (OSM highway=winter_road / highway=ice_road)
        winter_road     = 20,
        ice_road        = 15
      }
    },

    service_penalties = {
      alley             = 0.5,
      parking           = 0.5,
      parking_aisle     = 0.5,
      driveway          = 0.5,
      ["drive-through"] = 0.5,
      ["drive-thru"] = 0.5
    },

    barrier_penalties = {
      gate      = 60,
      lift_gate = 60,
    },

    restricted_highway_whitelist = Set {
      'motorway',
      'motorway_link',
      'trunk',
      'trunk_link',
      'primary',
      'primary_link',
      'secondary',
      'secondary_link',
      'tertiary',
      'tertiary_link',
      'residential',
      'living_street',
      'unclassified',
      'service',
      'winter_road',
      'ice_road'
    },

    construction_whitelist = Set {
      'no',
      'widening',
      'minor',
    },

    route_speeds = {
      ferry = 5,
      shuttle_train = 10
    },

    bridge_speeds = {
      movable = 5
    },

    -- surface/trackype/smoothness
    -- values were estimated from looking at the photos at the relevant wiki pages

    -- max speed for surfaces
    surface_speeds = {
      asphalt = nil,    -- nil mean no limit. removing the line has the same effect
      concrete = nil,
      ["concrete:plates"] = nil,
      ["concrete:lanes"] = nil,
      paved = nil,

      cement = 80,
      compacted = 80,
      fine_gravel = 80,

      paving_stones = 60,
      metal = 60,
      bricks = 60,

      grass = 40,
      wood = 40,
      sett = 40,
      grass_paver = 40,
      gravel = 40,
      unpaved = 40,
      ground = 40,
      dirt = 40,
      pebblestone = 40,
      tartan = 40,

      cobblestone = 30,
      clay = 30,

      earth = 20,
      stone = 20,
      rocky = 20,
      sand = 20,

      laterite = 15,

      mud = 10,

      -- winter surfaces (OSM surface=ice / surface=snow)
      ice  = 20,
      snow = 30
    },

    -- max speed for tracktypes
    tracktype_speeds = {
      grade1 =  60,
      grade2 =  40,
      grade3 =  30,
      grade4 =  25,
      grade5 =  20
    },

    -- max speed for smoothnesses
    smoothness_speeds = {
      intermediate    =  80,
      bad             =  40,
      very_bad        =  20,
      horrible        =  10,
      very_horrible   =  5,
      impassable      =  0
    },

    -- http://wiki.openstreetmap.org/wiki/Speed_limits
    maxspeed_table_default = {
      urban = 50,
      rural = 90,
      trunk = 110,
      motorway = 130
    },

    -- List only exceptions
    maxspeed_table = {
      ["at:rural"] = 100,
      ["at:trunk"] = 100,
      ["ar:urban"] = 40,
      ["ar:rural"] = 110,      
      ["be:motorway"] = 120,
      ["be-bru:rural"] = 70,
      ["be-bru:urban"] = 30,
      ["be-vlg:rural"] = 70,
      ["bg:motorway"] = 140,
      ["by:urban"] = 60,
      ["by:motorway"] = 110,
      ["ca-on:rural"] = 80,
      ["ch:rural"] = 80,
      ["ch:trunk"] = 100,
      ["ch:motorway"] = 120,
      ["de:living_street"] = 7,
      ["de:rural"] = 100,
      ["de:motorway"] = 0,
      ["dk:rural"] = 80,
      ["es:trunk"] = 90,
      ["fr:rural"] = 80,
      ["gb:nsl_single"] = (60*1609)/1000,
      ["gb:nsl_dual"] = (70*1609)/1000,
      ["gb:motorway"] = (70*1609)/1000,
      ["lv:living_street"] = 20,
      ["nl:rural"] = 80,
      ["nl:trunk"] = 100,
      ['no:rural'] = 80,
      ['no:motorway'] = 110,
      ['ph:urban'] = 40,
      ['ph:rural'] = 80,
      ['ph:motorway'] = 100,
      ['pl:rural'] = 100,
      ['pl:expressway'] = 120,
      ['pl:motorway'] = 140,
      ["ro:trunk"] = 100,
      ["ru:living_street"] = 20,
      ["ru:urban"] = 60,
      ["ru:motorway"] = 110,
      ["uk:nsl_single"] = (60*1609)/1000,
      ["uk:nsl_dual"] = (70*1609)/1000,
      ["uk:motorway"] = (70*1609)/1000,
      ['za:urban'] = 60,
      ['za:rural'] = 100,
      ["none"] = 140
    },

    relation_types = Sequence {
      "route"
    },

    -- classify highway tags when necessary for turn weights
    highway_turn_classification = {
    },

    -- classify access tags when necessary for turn weights
    access_turn_classification = {
    }
  }
end

function process_node(profile, node, result, relations)
  -- parse access and barrier tags
  local access = resolve_access(find_access_tag(node, profile.access_tags_hierarchy), profile)
  if access then
    if profile.access_tag_blacklist[access] and not profile.restricted_access_tag_list[access] then
      obstacle_map:add(node, Obstacle.new(obstacle_type.barrier))
    end
  else
    local barrier = node:get_value_by_key("barrier")
    if barrier then
      --  check height restriction barriers
      local restricted_by_height = false
      if barrier == 'height_restrictor' then
         local maxheight = Measure.get_max_height(node:get_value_by_key("maxheight"), node)
         restricted_by_height = maxheight and maxheight < profile.vehicle_height
      end

      --  make an exception for rising bollard barriers
      local bollard = node:get_value_by_key("bollard")
      local rising_bollard = bollard and "rising" == bollard

      -- make an exception for lowered/flat barrier=kerb
      -- and incorrect tagging of highway crossing kerb as highway barrier
      local kerb = node:get_value_by_key("kerb")
      local highway = node:get_value_by_key("highway")
      local flat_kerb = kerb and ("lowered" == kerb or "flush" == kerb)
      local highway_crossing_kerb = barrier == "kerb" and highway and highway == "crossing"

      -- make an exception for fence with sensory=audible/audio (virtual livestock fences)
      local sensory = node:get_value_by_key("sensory")
      local audible_fence = barrier == "fence" and sensory and (sensory == "audible" or sensory == "audio")

      -- check if barrier has a configurable penalty (e.g., gates)
      local barrier_penalty = profile.barrier_penalties[barrier]

      if not profile.barrier_whitelist[barrier]
                and not rising_bollard
                and not flat_kerb
                and not highway_crossing_kerb
                and not audible_fence
                and not barrier_penalty
                or restricted_by_height then
        obstacle_map:add(node, Obstacle.new(obstacle_type.barrier))
      end

      -- apply configurable penalty to gates/lift_gates
      if barrier_penalty then
        obstacle_map:add(node, Obstacle.new(obstacle_type.gate,
                                            obstacle_direction.both, barrier_penalty, 0))
      end
    end
  end

  Obstacles.process_node(profile, node)
end

local function get_cached_tag(self, key, fallback)
  local value = self.tags[key]
  if value == nil then value = fallback end
  if value ~= '' then return value end
end

local cached_way_metatable = {
  __index = function(self, key)
    local member = self.native[key]
    if type(member) == 'function' then
      return function(_, ...)
        return member(self.native, ...)
      end
    end
    return member
  end
}

local function process_way_with_tags(profile, way, result, relations, tag_source)
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
    highway = tag_source:get_value_by_key('highway'),
    bridge = tag_source:get_value_by_key('bridge'),
    route = tag_source:get_value_by_key('route')
  }

  -- perform an quick initial check and abort if the way is
  -- obviously not routable.
  -- highway or route tags must be in data table, bridge is optional
  if (not data.highway or data.highway == '') and
  (not data.route or data.route == '')
  then
    return
  end

  handlers = Sequence {
    -- set the default mode for this profile. if can be changed later
    -- in case it turns we're e.g. on a ferry
    WayHandlers.default_mode,

    -- check various tags that could indicate that the way is not
    -- routable. this includes things like status=impassable,
    -- toll=yes and oneway=reversible
    WayHandlers.blocked_ways,
    WayHandlers.avoid_ways,
    WayHandlers.handle_height,
    WayHandlers.handle_width,
    WayHandlers.handle_length,
    WayHandlers.handle_weight,

    -- determine access status by checking our hierarchy of
    -- access tags, e.g: motorcar, motor_vehicle, vehicle
    WayHandlers.access,

    -- check whether forward/backward directions are routable
    WayHandlers.oneway,

    -- check a road's destination
    WayHandlers.destinations,

    -- check whether we're using a special transport mode
    WayHandlers.ferries,
    WayHandlers.movables,

    -- handle service road restrictions
    WayHandlers.service,

    -- handle hov
    WayHandlers.hov,

    -- compute speed taking into account way type, maxspeed tags, etc.
    WayHandlers.speed,
    WayHandlers.maxspeed,
    WayHandlers.surface,

    -- apply vehicle-specific maximum speed cap before calculating rates
    WayHandlers.vehicle_speed_cap,

    WayHandlers.penalties,

    -- compute class labels
    WayHandlers.classes,

    -- handle turn lanes and road classification, used for guidance
    WayHandlers.turn_lanes,
    WayHandlers.classification,

    -- handle various other flags
    WayHandlers.roundabouts,
    WayHandlers.startpoint,
    WayHandlers.driving_side,

    -- set name, ref and pronunciation
    WayHandlers.names,

    -- set weight properties of the way
    WayHandlers.weights,

    -- set classification of ways relevant for turns
    WayHandlers.way_classification_for_turn
  }

  WayHandlers.run(profile, tag_source, result, data, handlers, relations)

  if profile.cardinal_directions then
      Relations.process_way_refs(way, relations, result)
  end
end

function process_way(profile, way, result, relations)
  process_way_with_tags(profile, way, result, relations, way)
end

local function process_way_cached(profile, way, result, relations)
  local tag_source = setmetatable({
    native = way,
    tags = way:snapshot_tags(),
    get_value_by_key = get_cached_tag
  }, cached_way_metatable)
  process_way_with_tags(profile, way, result, relations, tag_source)
end

function process_turn(profile, turn)
  -- Use a sigmoid function to return a penalty that maxes out at turn_penalty
  -- over the space of 0-180 degrees.  Values here were chosen by fitting
  -- the function to some turn penalty samples from real driving.
  if obstacle_map:any(turn.via) then
    for _, obs in pairs(obstacle_map:get(turn.from, turn.via)) do
      -- disregard a minor stop if entering by the major road
      -- rationale: if a stop sign is tagged at the center of the intersection with stop=minor
      -- it should only penalize the minor roads entering the intersection
      if obs.type == obstacle_type.stop_minor and not Obstacles.entering_by_minor_road(turn) then
          goto skip
      end
      -- heuristic to infer the direction of a stop without an explicit direction tag
      -- rationale: a stop sign should not be placed farther than 20m from the intersection
      if turn.number_of_roads == 2
          and obs.type == obstacle_type.stop
          and obs.direction == obstacle_direction.none
          and turn.source_road.distance < 20
          and turn.target_road.distance > 20 then
              goto skip
      end
      turn.duration = turn.duration + obs.duration
      ::skip::
    end
  end

  local is_left_hand_driving = turn.is_left_hand_driving
  local number_of_roads = turn.number_of_roads
  local is_u_turn = turn.is_u_turn
  local duration = turn.duration
  local turn_penalty = profile.turn_penalty
  local turn_bias = is_left_hand_driving and 1. / profile.turn_bias or profile.turn_bias

  if number_of_roads > 2 or turn.source_mode ~= turn.target_mode or is_u_turn then
    local angle = turn.angle
    if angle >= 0 then
      duration = duration + turn_penalty / (1 + math.exp( -((13 / turn_bias) *  angle/180 - 6.5*turn_bias)))
    else
      duration = duration + turn_penalty / (1 + math.exp( -((13 * turn_bias) * -angle/180 - 6.5/turn_bias)))
    end

    -- A turn across oncoming traffic waits for a gap, a turn away from it does
    -- not. turn_bias already leans that way, but it spends itself on the shape
    -- of the sigmoid, so its effect peaks near 90 degrees and fades to almost
    -- nothing at the sharp end, where both branches saturate at turn_penalty.
    -- This term does not fade there.
    --
    -- It only applies where there is oncoming traffic to cross. That means a
    -- real junction, since number_of_roads is also 2 for a mode change or a
    -- compressed obstacle node and charging those would tax every ferry
    -- boarding, and it means neither leg is a roundabout, where traffic runs
    -- one way and going around is a series of turns against nothing. Within a
    -- junction it tapers with the same sigmoid shape as the turn penalty, so a
    -- near-straight manoeuvre pays almost nothing and there is no cliff at the
    -- point where a bend becomes a turn. U-turns are excluded because
    -- u_turn_penalty already pays for crossing traffic through 180 degrees.
    local crosses_oncoming
    if is_left_hand_driving then
      crosses_oncoming = angle > 0
    else
      crosses_oncoming = angle < 0
    end

    if crosses_oncoming and not is_u_turn and number_of_roads > 2
       and not turn.source_is_roundabout and not turn.target_is_roundabout then
      local sharpness = 1 / (1 + math.exp( -(13 * math.abs(angle)/180 - 3.25)))
      duration = duration + profile.oncoming_turn_penalty * sharpness
    end

    if is_u_turn then
      if turn.has_turning_facility then
        duration = duration + profile.turning_facility_penalty
      else
        duration = duration + profile.properties.u_turn_penalty
      end
    end
  end

  turn.duration = duration
  local weight_name = profile.properties.weight_name
  -- for distance based routing we don't want to have penalties based on turn angle
  if weight_name == 'distance' then
     turn.weight = 0
  elseif weight_name == 'co2' or weight_name == 'routability-co2' then
     -- time lost at the turn is converted into idling emissions (grams of CO2)
     turn.weight = duration * profile.co2.idle_g_per_s
  else
     turn.weight = duration
  end

  if weight_name == 'routability' or weight_name == 'routability-co2' then
      -- penalize turns from non-local access only segments onto local access only tags
      if not turn.source_restricted and turn.target_restricted then
          turn.weight = constants.max_turn_weight
      end
  end
end

return {
  setup = setup,
  process_way = process_way,
  process_node = process_node,
  process_turn = process_turn,
  tag_filters = {
    [process_node] = {any = {'barrier', 'highway', table.unpack(access_tags)}},
    [process_way] = {any = {'highway', 'route'}, callback = process_way_cached}
  }
}
