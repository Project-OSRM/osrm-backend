-- This files shows how you can runt he LUA profile processing
-- stand-alone without the C++ engine, which is useful for
-- debugging profiles.

-- You 

-- Run this from the pofiles/ folder using:
-- > lua5.1 debug.lua


function applyAccessTokens(turn_lanes,vehicle_lane)
end

function trimLaneString(turn_lanes, psv_lanes_forward, psv_lanes_backward)
end

properties = {}



-- pull in some required utilities
local Tag = require('lib/tags')
local pprint = require('lib/pprint')

-- require the profile you want to debug
require('./car')
Mode = Car

-- setup the input OSM tags
tags = {
  name = 'Hyper Heights',
  highway = 'primary',
  --lanes = '1',
  --foot = 'yes',
  --vehicle = 'no',
  --['foot:forward'] = 'yes',
  --ref = 'A1',
  --route = 'ferry',
  --['name:pronunciation'] = 'Hajper Haijts',
  --['hov:forward'] = 'designated',
  --['hov:lanes:forward'] = 'designated',
  --impassable = 'yes',
  --oneway = '-1',
  --junction = 'roundabout',
  --oneway = 'reversible',
  --access = 'delivery',
  --bridge = 'movable',
  --bicycle = 'yes',
  --railway = 'train',
  --railway = 'construction',
  --bicycle = 'yes',
  ['turn:lanes:forward'] = 'left|through|right',
  ['lanes:psv:forward'] = '1',
}

-- setup a way object with the above tags
way = Tag:new( tags )

-- process the way!
result = {}
Mode:process(way,result)

-- report the results
print( "Input:" )
pprint( tags )

print( "\nOutput:" )
pprint( Mode.tmp.main )
