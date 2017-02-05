-- Script for testing lua profiles
--
-- You pass in osm tags, and get the result table.
-- This makes it possible to test tag parsing in profiles directly,
-- Without having to go use osrm binaries or osm files
--
-- The script accepts the profile name and a hash of key/values as arguments,
-- and outputs a formattet result table. Example usage:
--
-- > cd profiles
-- > lua5.1 ./lib/testing.lua bicycle highway primary oneway yes
-- { 
--   backward_mode = 6,
--   backward_rate = 1.6666666666667,
--   backward_speed = 6,
--   duration = 0,
--   forward_mode = 2,
--   forward_rate = 4.1666666666667,
--   forward_speed = 15,
--   road_classification = { 
--     may_be_ignored = false,
--     road_priority_class = 4 
--   } 
-- }
-- 

local Debug = require('lib/profile_debugger')
local pprint = require('lib/pprint')


function parse_command_arguments()
  local key = nil
  local data = {}
  local profile
  for i, v in ipairs(arg) do
    if i==1 then
      profile = v
    else
      if key then
        data[key] = v
        key = nil
      else
        key = v
      end
    end
  end
  return profile,data
end


local profile, way = parse_command_arguments()
Debug.load_profile(profile)     -- load profile
local result = {}               -- results go here
Debug.way_function(way,result)  -- call way function
pprint(result)                  -- and print output
