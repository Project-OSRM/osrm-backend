-- Rasterbot profile

-- Minimalist node_ and way_functions in order to test source_ and segment_functions

function node_function (node, result)
end

function way_function (way, result)
  local highway = way:get_value_by_key("highway")
  local name = way:get_value_by_key("name")

  if name then
    result.name = name
  end

  result.forward_speed = 15
  result.backward_speed = 15
end

function source_function ()
  raster_source = sources:load(
    "../test/rastersource.asc",
    0,    -- lon_min
    0.1,  -- lon_max
    0,    -- lat_min
    0.1,  -- lat_max
    5,    -- nrows
    4     -- ncols
  )
end

function segment_function (source, target, distance, weight)
  local sourceData = sources:query(raster_source, source.lon, source.lat)
  local targetData = sources:query(raster_source, target.lon, target.lat)
  print ("evaluating segment: " .. sourceData.datum .. " " .. targetData.datum)
  local invalid = sourceData.invalid_data()

  if sourceData.datum ~= invalid and targetData.datum ~= invalid then
    local slope = math.abs(sourceData.datum - targetData.datum) / distance
    print ("   slope: " .. slope)
    print ("   was speed: " .. weight.speed)

    weight.speed = weight.speed * (1 - (slope * 5))
    print ("   new speed: " .. weight.speed)
  end
end
