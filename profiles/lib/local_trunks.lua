-- Assigns extra_speeds list based on location tag
-- based on reading https://wiki.openstreetmap.org/wiki/Key:motorroad
-- and https://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access_restrictions
-- (esp #Alternative_ideas)
-- pass in way data and speed to set.
-- 

local Set = require('lib/set')
local Sequence = require('lib/sequence')

local trunk_allowed_set = Set 
{
  'AUS',
  'BLR',
  'BRA',
  'CHN',
  'HKG',
  'ISL',
  'ITA',
  'IRL',
  'NOR',
  'NZL',
  'OMN',
  'PHL',
  'POL',
  'ROU',
  'SWE',
  'THA',
  'TUR',
  'UKR',
  'GBR',
  'USA',
}

trunk_nomotorroad_allowed_set = Set
{
  'BEL',
  'CHL',
  'DEU',
  'FIN',
  'FRA',
  'GRC',
  'GBR',
  'HRV',
  'NLD',
  'RUS',
}

local LocalMotorways = {}

local walking_speed = 5

local bicycle_speed = 15

local default_speed = 10

local all_speeds = {
  bicycle_speed = Sequence
  {
    highway = {
      trunk = bicycle_speed,
      trunk_link = bicycle_speed
    }
  },
  
  walking_speed =  Sequence
  {
    highway = {
      trunk = walking_speed,
      trunk_link = walking_speed
    }
  },
  
  default_speed = Sequence
  {
    highway = {
      trunk = default_speed,
      trunk_link = default_speed
    }
  }
}

local function  set_speeds(thespeed)
  local defspeeds = {}
  defspeeds = all_speeds.default_speed
  local speeds = {}
  for key, set in pairs(defspeeds) do
    local items = {}
    for item, value in pairs(set) do
      items[item] = thespeed
--      print('key ' .. key .. ' item ' .. item .. ' speed ' .. items[item])
    end
    speeds[key] = items
  end
  return speeds
end

local function  get_nomotorroad_speeds(way, data, thespeed)
  if way:get_value_by_key("motorroad") then
    local motorroad = way:get_value_by_key("motorroad")
    if motorroad and motorroad == "yes" then
      return false
    end
  end
  speeds = {}
  if thespeed == walking_speed then
    speeds = all_speeds.walking_speed
  else
    if thespeed == bicycle_speed then
      speeds = all_speeds.bicycle_speed
    else  
      if thespeed == default_speed then
        speeds = all_speeds.default_speed
      else
        return set_speeds(thespeed)
      end
    end
  end
  return speeds
end  

function LocalMotorways.get_extra_speeds(way, data, thespeed)
  if data.highway == "trunk" or data.highway == "trunk_link" then
    if way:get_location_tag('ISO3166-1:alpha3') then
      location = way:get_location_tag('ISO3166-1:alpha3')
      if trunk_nomotorroad_allowed_set[location] then 
        return get_nomotorroad_speeds(way, data, thespeed)
      end
      speeds = {}
      if thespeed == walking_speed then
        speeds = all_speeds.walking_speed
      else
        if thespeed == bicycle_speed then
          speeds = all_speeds.bicycle_speed
        else  
          if thespeed == default_speed then
            speeds = all_speeds.default_speed
          else
            return set_speeds(thespeed)
          end
        end
      end
      return speeds
    end
  end
  return false    
end
return LocalMotorways

