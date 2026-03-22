-- Per-country bicycle highway access tables
-- Based on: https://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access_restrictions

local country_bicycle_data = {}

local no_speed = -1
local default_speed = 15
local walking_speed = 4  -- matches bicycle.lua walking_speed

-- Shared base bicycle highway speeds (Worldwide defaults)
local bicycle_speeds = {
  motorway       = no_speed,
  motorway_link  = no_speed,
  trunk          = default_speed,
  trunk_link     = default_speed,
  primary        = default_speed,
  primary_link   = default_speed,
  secondary      = default_speed,
  secondary_link = default_speed,
  tertiary       = default_speed,
  tertiary_link  = default_speed,
  residential    = default_speed,
  unclassified   = default_speed,
  living_street  = default_speed,
  road           = default_speed,
  service        = default_speed,
  track          = 12,
  path           = 13,
  cycleway       = default_speed,
  footway        = walking_speed,
  pedestrian     = no_speed,
  steps          = walking_speed,
  pier           = walking_speed,
  bridleway      = no_speed,
}

-- Helper: copy base table, overriding specified keys
local function make_highway(overrides)
  local t = {}
  for k, v in pairs(bicycle_speeds) do t[k] = v end
  for k, v in pairs(overrides) do t[k] = v end
  return t
end

-- Countries where trunk/trunk_link are blocked for cyclists
local trunk_blocked = make_highway({ trunk = no_speed, trunk_link = no_speed })

-- Countries where bridleway is allowed
local bridleway_allowed = make_highway({ bridleway = default_speed })

-- Countries where bridleway is allowed but trunk is also blocked
local trunk_blocked_bridleway_allowed = make_highway({
  trunk = no_speed, trunk_link = no_speed, bridleway = default_speed
})

-- Countries where footway and pedestrian are allowed
local footway_pedestrian_allowed = make_highway({
  footway = default_speed, pedestrian = default_speed
})

-- GBR: footway/pedestrian allowed, trunk not blocked
local gbr_highway = make_highway({ footway = default_speed, pedestrian = default_speed })

-- AUS: footway/pedestrian allowed, trunk not blocked
local aus_highway = make_highway({ footway = default_speed, pedestrian = default_speed })

-- NLD: pedestrian allowed
local nld_highway = make_highway({ pedestrian = default_speed })

-- NOR: pedestrian allowed
local nor_highway = make_highway({ pedestrian = default_speed })

-- IRL: pedestrian allowed, bridleway allowed
local irl_highway = make_highway({ pedestrian = default_speed, bridleway = default_speed })

local countries = {
  Worldwide = { bicycle = { highway = bicycle_speeds } },

  -- Countries with trunk blocked
  AUT   = { bicycle = { highway = trunk_blocked } },
  BEL   = { bicycle = { highway = trunk_blocked } },
  CHE   = { bicycle = { highway = trunk_blocked } },
  DNK   = { bicycle = { highway = trunk_blocked } },
  FIN   = { bicycle = { highway = trunk_blocked } },
  FRA   = { bicycle = { highway = trunk_blocked } },
  HUN   = { bicycle = { highway = trunk_blocked } },
  SVK   = { bicycle = { highway = trunk_blocked } },

  -- Countries with bridleway allowed (trunk still accessible)
  GRC   = { bicycle = { highway = bridleway_allowed } },

  -- IRL: bridleway + pedestrian allowed
  IRL   = { bicycle = { highway = irl_highway } },

  -- GBR, AUS: footway + pedestrian allowed
  GBR   = { bicycle = { highway = gbr_highway } },
  AUS   = { bicycle = { highway = aus_highway } },

  -- NLD, NOR: pedestrian allowed
  NLD   = { bicycle = { highway = nld_highway } },
  NOR   = { bicycle = { highway = nor_highway } },

  -- Countries that follow Worldwide defaults
  BLR   = { bicycle = { highway = bicycle_speeds } },
  BRA   = { bicycle = { highway = bicycle_speeds } },
  CHN   = { bicycle = { highway = bicycle_speeds } },
  DEU   = { bicycle = { highway = bicycle_speeds } },
  ESP   = { bicycle = { highway = bicycle_speeds } },
  HKG   = { bicycle = { highway = bicycle_speeds } },
  ISL   = { bicycle = { highway = bicycle_speeds } },
  ITA   = { bicycle = { highway = bicycle_speeds } },
  OMN   = { bicycle = { highway = bicycle_speeds } },
  PHL   = { bicycle = { highway = bicycle_speeds } },
  POL   = { bicycle = { highway = bicycle_speeds } },
  ROU   = { bicycle = { highway = bicycle_speeds } },
  RUS   = { bicycle = { highway = bicycle_speeds } },
  SWE   = { bicycle = { highway = bicycle_speeds } },
  THA   = { bicycle = { highway = bicycle_speeds } },
  TUR   = { bicycle = { highway = bicycle_speeds } },
  UKR   = { bicycle = { highway = bicycle_speeds } },
  USA   = { bicycle = { highway = bicycle_speeds } },
  tyrol = { bicycle = { highway = trunk_blocked } },
}

function country_bicycle_data.getAccessProfile(country, profile)
  if countries[country] and countries[country][profile] then
    return countries[country][profile]
  end
  return countries['Worldwide'][profile]
end

return country_bicycle_data
