-- handling max speeds, based on the "maxspeed" tag

local math = math
local MaxSpeed = {}

-- parse the maxspeed tag and km/h
-- tag values specifying miles/hour will be converted to km/h
-- optionally takes a table of defaults, used to convert e.g. FR:urban to 50.
function MaxSpeed.parse_maxspeed(source, defaults, overrides)
  if not source then
    return 0
  end
  local n = tonumber(source:match("%d*"))
  if n then
    -- parse direct values like 90, 90 km/h, 40mph
    if string.match(source, "mph") or string.match(source, "mp/h") then
      n = (n*1609)/1000   -- convert to km/h
    end
  else
    -- parse defaults values like FR:urban, usign the specified table of defaults
    if maxspeed_defaults then
      source = string.lower(source)
      n = overrides[source]
      if not n then
        local highway_type = string.match(source, "%a%a:(%a+)")
        n = defaults[highway_type]
      end
    end
  end
  if not n then
    return 0
  else
    return n
  end
end

return MaxSpeed
