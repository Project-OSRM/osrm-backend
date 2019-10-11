
local tags = require('lib/tags')
local Measure = require('lib/measure')

-- Test get speed uint by key
local way = {
    speed_unit = 'M'
}

function way:get_value_by_key(k)
    return self[k]
end

local speed_unit = tags.get_speed_unit_by_key(way, 'speed_unit')
print(speed_unit)

-- Test cases:
-- 1. speed without unit + speed unit in mile per hour
local source = "50"
speed_unit = "M"

local n = Measure.get_max_speed(source, speed_unit)
print(n)

-- 2. speed without unit + speed unit in kilometer per hour
source = "50"
speed_unit = "K"

n = Measure.get_max_speed(source, speed_unit)
print(n)

-- 3. speed with unit + speed unit in mile per hour
source = "50 mph"
speed_unit = "M"

n = Measure.get_max_speed(source, speed_unit)
print(n)

-- 4. speed with unit + speed unit in kilometer per hour
source = "50 mph"
speed_unit = "K"

n = Measure.get_max_speed(source, speed_unit)
print(n)

-- 5. speed with unit + speed unit in kilometer per hour
source = "50 kph"
speed_unit = "K"

n = Measure.get_max_speed(source, speed_unit)
print(n)

-- 6. speed with unit + speed unit in mile per hour
source = "50 kph"
speed_unit = "M"

n = Measure.get_max_speed(source, speed_unit)
print(n)