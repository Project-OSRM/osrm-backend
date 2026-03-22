-- Country-specific access profile dispatcher
-- Based on: https://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access_restrictions

local country_data = require('lib/country_data')
local country_foot_data = require('lib/country_foot_data')
local country_bicycle_data = require('lib/country_bicycle_data')
local Tags = require('lib/tags')

local country_speeds = {}

-- Read the ISO alpha-3 country code from the way's location tags.
-- Tries multiple common GeoJSON property names.
-- Returns the ISO code or 'Worldwide' if files are loaded but the way is
-- outside all known polygons.  Returns nil when no location data files were
-- provided at all, signalling that profile defaults should be used instead.
function country_speeds.getcountrytag(way)
  if not way:has_location_data() then
    return nil
  end
  local location = way:get_location_tag('iso_a3_eh')
  if not location then location = way:get_location_tag('iso_a3') end
  if not location then location = way:get_location_tag('ISO_A3') end
  if not location then location = way:get_location_tag('ISO3_CODE') end
  if not location then
    local name = way:get_location_tag('name_en')
    if name then location = country_data.getCnameEntry(name) end
  end
  if location and country_data.inAccessSet(location) then
    return location
  end
  return 'Worldwide'
end

-- Return the access profile table for (country, transport mode).
-- data.location must be set by getcountrytag() first.
function country_speeds.getAccessProfile(data, profile)
  local country = data.location or 'Worldwide'
  if profile == 'bicycle' then
    return country_bicycle_data.getAccessProfile(country, 'bicycle')
  elseif profile == 'foot' then
    return country_foot_data.getAccessProfile(country, 'foot')
  end
  return nil
end

-- Speed handler for foot.lua when countryspeeds is enabled.
-- Replaces WayHandlers.speed in the handler sequence.
-- Uses country-specific highway access table, falls back to profile.hwyspeeds then profile.otherspeeds.
function country_speeds.wayspeed(profile, way, result, data)
  if result.forward_speed ~= -1 then
    return  -- already set by a route handler
  end

  local key, value, speed

  if profile.uselocationtags and profile.uselocationtags.countryspeeds and data.location then
    local extra = country_speeds.getAccessProfile(data, profile.profile)
    if extra then
      key, value, speed = Tags.get_constant_by_key_value(way, extra)
      if speed and speed == -1 then
        -- Explicitly blocked by country data
        return false
      end
    end
  end

  -- Fall back to profile highway speeds if not found in country data
  if not speed then
    if profile.hwyspeeds then
      key, value, speed = Tags.get_constant_by_key_value(way, profile.hwyspeeds)
    end
  end

  if speed then
    result.forward_speed = speed
    result.backward_speed = speed
  else
    -- Non-highway speeds (railway, amenity, man_made, leisure)
    if profile.otherspeeds then
      key, value, speed = Tags.get_constant_by_key_value(way, profile.otherspeeds)
    end
    if speed then
      result.forward_speed = speed
      result.backward_speed = speed
    else
      -- Access tag whitelist fallback
      if profile.access_tag_whitelist[data.forward_access] then
        result.forward_speed = profile.default_speed
      elseif data.forward_access and not profile.access_tag_blacklist[data.forward_access] then
        result.forward_speed = profile.default_speed
      elseif not data.forward_access and data.backward_access then
        result.forward_mode = mode.inaccessible
      end
      if profile.access_tag_whitelist[data.backward_access] then
        result.backward_speed = profile.default_speed
      elseif data.backward_access and not profile.access_tag_blacklist[data.backward_access] then
        result.backward_speed = profile.default_speed
      elseif not data.backward_access and data.forward_access then
        result.backward_mode = mode.inaccessible
      end
    end
  end

  if result.forward_speed == -1 and result.backward_speed == -1 and result.duration <= 0 then
    return false
  end
end

return country_speeds
