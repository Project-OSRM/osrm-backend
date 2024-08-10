-- Assigns extra_speeds list based on location tag
-- based on reading https://wiki.openstreetmap.org/wiki/Key:motorroad
-- and https://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access_restrictions
-- (esp #Alternative_ideas)
-- We treat all cases of motorroad="yes" as no access.
-- pass in way data and speed to set.
-- 

local Set = require('lib/set')
local Sequence = require('lib/sequence')
local country_foot_data = require('lib/country_foot_data')
local country_bicycle_data = require('lib/country_bicycle_data')
local country_vehicle_data = require('lib/country_vehicle_data')
local country_data = require('lib/country_data')
local Tags = require('lib/tags')

local country_speeds = {}

function country_speeds.inAccessSet(country)
  return country_data.inAccessSet(country)
end

function country_speeds.getCnameEntry(country)
  return country_data.getCnameEntry(country)
end

function country_speeds.getcountrytag(way)
  local location = way:get_location_tag('iso_a3_eh')
  if not location then
    location = way:get_location_tag("iso_a3")
  end
  if not location then
    location = way:get_location_tag("ISO_A3")
  end
  if not location then
    location = way:get_location_tag("ISO3_CODE")
  end
  if not location then
    countryname = way:get_location_tag("name_en")
    if countryname then
      location = country_speeds.getCnameEntry(countryname)
    end
  end
  if country_data.inAccessSet(location) then
    return location
  end
  return 'Worldwide'
end  

function country_speeds.getAccessProfile(data, profile)
  local country = data.location
  local prof = profile
  if profile == 'foot' then
    return country_foot_data.getAccessProfile(country, prof)
  end
  if profile == 'bicycle' then
    return country_bicycle_data.getAccessProfile(country, prof)
  end
  if profile == 'vehicle' then
    return country_vehicle_data.getAccessProfile(country, prof)
  end
  return false
end

-- handle speed (excluding maxspeed)
function country_speeds.wayspeed(profile,way,result,data)
  if result.forward_speed ~= -1 then
    return        -- abort if already set, eg. by a route
  end
  local key,value,speed
  
  if profile.uselocationtags and profile.uselocationtags.countryspeeds then
    -- check for location tags to handle speeds 
    local extra_speeds = country_speeds.getAccessProfile(data, profile.profile)
    if extra_speeds then
       key,value,speed = Tags.get_constant_by_key_value(way,extra_speeds)
    else
      key,value,speed = Tags.get_constant_by_key_value(way,profile.hwyspeeds)  
    end   
    if speed and speed ~= -1 then
      result.forward_speed = speed
      result.backward_speed = speed
    else
      speed = false 
    end
  else
    key,value,speed = Tags.get_constant_by_key_value(way,profile.hwyspeeds)  
  end
   
  if speed then
    -- set speed by way type
    result.forward_speed = speed
    result.backward_speed = speed
  else
    if profile.otherspeeds then
      key,value,speed = Tags.get_constant_by_key_value(way,profile.otherspeeds)    
    end
  end

  if speed then
    -- set speed by way type
    result.forward_speed = speed
    result.backward_speed = speed
  end

  if not speed then
  -- Set the avg speed on ways that are marked accessible
    if profile.access_tag_whitelist[data.forward_access] then
      result.forward_speed = profile.default_speed
    elseif data.forward_access and not profile.access_tag_blacklist[data.forward_access] then
      result.forward_speed = profile.default_speed -- fallback to the avg speed if access tag is not blacklisted
    elseif not data.forward_access and data.backward_access then
       result.forward_mode = mode.inaccessible
    end

    if profile.access_tag_whitelist[data.backward_access] then
      result.backward_speed = profile.default_speed
    elseif data.backward_access and not profile.access_tag_blacklist[data.backward_access] then
      result.backward_speed = profile.default_speed -- fallback to the avg speed if access tag is not blacklisted
    elseif not data.backward_access and data.forward_access then
       result.backward_mode = mode.inaccessible
    end
  end

  if result.forward_speed == -1 and result.backward_speed == -1 and result.duration <= 0 then
    return false
  end
end

return country_speeds

