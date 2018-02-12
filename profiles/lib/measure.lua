local Sequence = require('lib/sequence')

Measure = {}

-- measurements conversion constants
local inch_to_meters = 0.0254
local feet_to_inches = 12

--- Parse string as a height in meters.
--- according to http://wiki.openstreetmap.org/wiki/Key:maxheight
function Measure.parse_value_meters(value)
  local n = tonumber(value:gsub(",", "."):match("%d+%.?%d*"))
  if n then
    inches = value:match("'.*")
    if inches then -- Imperial unit to metric
      -- try to parse feets/inch
      n = n * feet_to_inches
      local m = tonumber(inches:match("%d+"))
      if m then
        n = n + m
      end
      n = n * inch_to_meters
    end
    return n
  end
end

--- according to http://wiki.openstreetmap.org/wiki/Map_Features/Units#Explicit_specifications
local tonns_parse_patterns = Sequence {
  "%d+",
  "%d+.%d+",
  "%d+.%d+ ?t"
}

local kg_parse_patterns = Sequence {
  "%d+ ?kg"
}

--- Parse weight value in kilograms
function Measure.parse_value_kilograms(value)
  -- try to parse kilograms
  for i, templ in ipairs(kg_parse_patterns) do
    m = string.match(value, templ)
    if m then
      return tonumber(m)
    end
  end

  -- try to parse tonns
  for i, templ in ipairs(tonns_parse_patterns) do
    m = string.match(value, templ)
    if m then
      return tonumber(m) * 1000
    end
  end
end

-- default maxheight value defined in https://wiki.openstreetmap.org/wiki/Key:maxheight#Non-numerical_values
local default_maxheight = 4.5
-- Available Non numerical values equal to 4.5; below_default and no_indications are not considered
local height_non_numerical_values = Set { "default", "none", "no-sign", "unsigned" }

--- Get maxheight of specified way in meters. If there are no
--- max height, then return nil
function Measure.get_max_height(raw_value, element)
  if raw_value then
    if height_non_numerical_values[raw_value] then
      if element then
        return element:get_location_tag('maxheight') or default_maxheight
      else
        return default_maxheight
      end
    else
      return Measure.parse_value_meters(raw_value)
    end
  end
end

--- Get maxwidth of specified way in meters.
function Measure.get_max_width(raw_value)
  if raw_value then
    return Measure.parse_value_meters(raw_value)
  end
end

--- Get maxweight of specified way in kilogramms
function Measure.get_max_weight(raw_value)
  if raw_value then
    return Measure.parse_value_kilograms(raw_value)
  end
end


return Measure;
