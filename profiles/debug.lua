-- Enable calling our lua profile code directly, which makes it easier to debug.
-- We simulate the normal C++ environment by defining some globals and functions.
-- FIXME:
-- There are a few C++ tag helper methods that LUA code can call.
-- Debugging LUA code thay uses these will not work unless we reimplement the 
-- methods in LUA.

-- Usage:
-- > cd profiles
-- > lua5.1 debug.lua


-- for more convenient printing of tables
local pprint = require('lib/pprint')


-- globals that are normally set from C++

-- profiles code modifies this table
properties = {}

-- should match values defined in include/extractor/guidance/road_classification.hpp
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
  highway = 'primary'
}
-- function normally provided via C++
function way:get_value_by_key(k)
  return self[k]
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
way_function(way,result)

-- and print the output
pprint(way)
print("=>")
pprint(result)
