-- Per-country foot highway access tables
-- Based on: https://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access_restrictions

local country_foot_data = {}

local no_speed = -1
local walking_speed = 5  -- matches foot.lua walking_speed

-- Shared base foot highway speeds (Worldwide defaults)
local foot_speeds = {
  motorway       = no_speed,
  motorway_link  = no_speed,
  trunk          = walking_speed,
  trunk_link     = walking_speed,
  primary        = walking_speed,
  primary_link   = walking_speed,
  secondary      = walking_speed,
  secondary_link = walking_speed,
  tertiary       = walking_speed,
  tertiary_link  = walking_speed,
  unclassified   = walking_speed,
  residential    = walking_speed,
  road           = walking_speed,
  living_street  = walking_speed,
  service        = walking_speed,
  track          = walking_speed,
  path           = walking_speed,
  steps          = walking_speed,
  pedestrian     = walking_speed,
  platform       = walking_speed,
  footway        = walking_speed,
  pier           = walking_speed,
  cycleway       = no_speed,
  bridleway      = no_speed,
}

-- Helper: copy base table, overriding specified keys
local function make_highway(overrides)
  local t = {}
  for k, v in pairs(foot_speeds) do t[k] = v end
  for k, v in pairs(overrides) do t[k] = v end
  return t
end

-- Countries where trunk/trunk_link are blocked for pedestrians
local trunk_blocked = make_highway({ trunk = no_speed, trunk_link = no_speed })

-- CHE: trunk blocked, cycleway allowed
local che_highway = make_highway({ trunk = no_speed, trunk_link = no_speed, cycleway = walking_speed })

-- FIN: trunk blocked, cycleway allowed
local fin_highway = make_highway({ trunk = no_speed, trunk_link = no_speed, cycleway = walking_speed })

-- BEL: trunk blocked, bridleway allowed
local bel_highway = make_highway({ trunk = no_speed, trunk_link = no_speed, bridleway = walking_speed })

-- IRL: trunk accessible, bridleway allowed
local irl_highway = make_highway({ bridleway = walking_speed })

local countries = {
  Worldwide = { foot = { highway = foot_speeds } },

  -- Countries with trunk blocked (no special additions)
  AUT   = { foot = { highway = trunk_blocked } },
  DNK   = { foot = { highway = trunk_blocked } },
  FRA   = { foot = { highway = trunk_blocked } },
  HUN   = { foot = { highway = trunk_blocked } },
  SVK   = { foot = { highway = trunk_blocked } },

  -- CHE: trunk blocked + cycleway allowed
  CHE   = { foot = { highway = che_highway } },

  -- FIN: trunk blocked + cycleway allowed
  FIN   = { foot = { highway = fin_highway } },

  -- BEL: trunk blocked + bridleway allowed
  BEL   = { foot = { highway = bel_highway } },

  -- IRL: bridleway allowed
  IRL   = { foot = { highway = irl_highway } },

  -- Countries following Worldwide defaults
  AUS   = { foot = { highway = foot_speeds } },
  BLR   = { foot = { highway = foot_speeds } },
  BRA   = { foot = { highway = foot_speeds } },
  CHN   = { foot = { highway = foot_speeds } },
  DEU   = { foot = { highway = foot_speeds } },
  ESP   = { foot = { highway = foot_speeds } },
  GBR   = { foot = { highway = foot_speeds } },
  GRC   = { foot = { highway = foot_speeds } },
  HKG   = { foot = { highway = foot_speeds } },
  ISL   = { foot = { highway = foot_speeds } },
  ITA   = { foot = { highway = foot_speeds } },
  NLD   = { foot = { highway = foot_speeds } },
  NOR   = { foot = { highway = foot_speeds } },
  OMN   = { foot = { highway = foot_speeds } },
  PHL   = { foot = { highway = foot_speeds } },
  POL   = { foot = { highway = foot_speeds } },
  ROU   = { foot = { highway = foot_speeds } },
  RUS   = { foot = { highway = foot_speeds } },
  SWE   = { foot = { highway = foot_speeds } },
  THA   = { foot = { highway = foot_speeds } },
  TUR   = { foot = { highway = foot_speeds } },
  UKR   = { foot = { highway = foot_speeds } },
  USA   = { foot = { highway = foot_speeds } },
  tyrol = { foot = { highway = trunk_blocked } },
}

function country_foot_data.getAccessProfile(country, profile)
  if countries[country] and countries[country][profile] then
    return countries[country][profile]
  end
  return countries['Worldwide'][profile]
end

return country_foot_data
