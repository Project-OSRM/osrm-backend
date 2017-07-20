local Sequence = require('lib/sequence')

Measure = {}

-- measurements conversion constants
local inch_to_meters = 0.0254
local feet_to_inches = 12

--- according to http://wiki.openstreetmap.org/wiki/Key:maxheight
local meters_parse_patterns = Sequence {
  "%d+",
  "%d+.%d+",
  "%d+.%d+ m",
  "%d+,%d+ m", -- wrong
  "%d+.%d+m",  -- wrong
  "%d+,%d+m",  -- wrong
}

local feet_parse_patterns = Sequence {
  "%d+\'%d+\'",
}

function Measure.convert_feet_to_inches(feet)
  return feet * feet_to_inches
end

function Measure.convert_inches_to_meters(inches)
  return inches * inch_to_meters
end

--- Parse string as a height in meters.
function Measure.parse_value_meters(value)
  -- try to parse meters
  for i, templ in ipairs(meters_parse_patterns) do
    m = string.match(value, templ)
    if m then
      return tonumber(m)
    end
  end

  -- try to parse feets/inch
  for i, templ in ipairs(feet_parse_patterns) do
    m = string.match(value, templ)
    if m then
      feet, inch = m
      feet = tonumber(feet)
      inch = tonumber(inch)

      inch = inch + feet * feet_to_inches
      return Measure.convert_inches_to_meters(inch)
    end
  end

  return
end

--- Get maxheight of specified way in meters. If there are no
--- max height, then return nil
function Measure.get_max_height(way)
  raw_value = way:get_value_by_key('maxheight')
  if raw_value then
    return Measure.parse_value_meters(raw_value)
  end

  -- TODO: parse another tags
end

--- Get maxwidth of specified way in meters.
function Measure.get_max_width(way)
  raw_value = way:get_value_by_key('maxwidth')
  if raw_value then
    print(way:id(), raw_value)
    return Measure.parse_value_meters(raw_value)
  end
end


return Measure;