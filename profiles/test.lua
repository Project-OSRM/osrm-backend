-- Enable calling our lua profile code directly from the lua command line,
-- which makes it easier to debug.
-- We simulate the normal C++ environment by defining the required globals and functions.

-- Usage:
-- > cd profiles
-- > lua5.1 debug.lua


-- for more convenient printing of tables
local pprint = require('lib/pprint')

-- globals that are normally set from C++

-- profiles code modifies this table
properties = {}

-- should match values defined in include/extractor/road_classification.hpp
road_priority_class = {
  motorway = 0,
  trunk = 2,
  primary = 4,
  secondary = 6,
  tertiary = 8,
  main_residential = 10,
  side_residential = 11,
  link_road = 14,
  bike_path = 16,
  foot_path = 18,
  connectivity = 31,
}

-- should match values defined in include/extractor/travel_mode.hpp
mode = {
  inaccessible = 0,
  driving = 1,
  cycling = 2,
  walking = 3,
  ferry = 4,
  train = 5,
  pushing_bike = 6,
}

-- input tags, normally extracted from OSM data
local way = {
  highway = 'primary',
  name = 'Main Street',
  --width = '3',
  --maxspeed = '30',
  --['maxspeed:advisory'] = '25',
  --oneway = '-1',
  --service = 'alley',
  --['oneway:bicycle'] = 'yes',
  --junction = 'roundabout',
  --['name:pronunciation'] = 'fuerloong',
  --route = 'ferry',
  --duration = '00:01:00',
  --hov = 'designated',
  --access = 'no'
}
-- tag function normally provided via C++
function way:get_value_by_key(k)
  return self[k]
end

-- Mock C++ helper functions which are called from LUA.
-- FIXME
-- Debugging LUA code that uses these will not work correctly
-- unless we reimplement themethods in LUA.

function durationIsValid(str)
  return true
end

function parseDuration(str)
  return 1
end

function canonicalizeStringList(str)
  return str
end
 
-- start state of result table, normally set form C++
local result = {
  road_classification = {},
  forward_speed = -1,
  backward_speed = -1,
}

-- the profile we want to debug
require("car")

-- call the way function
for i=0,10000,1
do
  way_function(way,result)
end
