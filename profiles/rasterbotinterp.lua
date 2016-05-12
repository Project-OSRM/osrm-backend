api_version = 1
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

  result.forward_mode = mode.cycling
  result.backward_mode = mode.cycling

  result.forward_speed = 15
  result.backward_speed = 15
end

function source_function ()
  local path = os.getenv('OSRM_RASTER_SOURCE')
  if not path then
    path = "rastersource.asc"
  end
  raster_source = sources:load(
    path,
    0,    -- lon_min
    0.1,  -- lon_max
    0,    -- lat_min
    0.1,  -- lat_max
    5,    -- nrows
    4     -- ncols
  )
end

function segment_function (segment)
  local sourceData = sources:interpolate(raster_source, segment.source.lon, segment.source.lat)
  local targetData = sources:interpolate(raster_source, segment.target.lon, segment.target.lat)
  io.write("evaluating segment: " .. sourceData.datum .. " " .. targetData.datum .. "\n")
  local invalid = sourceData.invalid_data()
  local scaled_weight = segment.weight
  local scaled_duration = segment.duration

  if sourceData.datum ~= invalid and targetData.datum ~= invalid then
    local slope = math.abs(sourceData.datum - targetData.datum) / segment.distance
    io.write("   slope: " .. slope .. "\n")
    io.write("   was weight: " .. segment.weight .. "\n")
    io.write("   was speed: " .. segment.duration .. "\n")

    scaled_weight = scaled_weight / (1 - (slope * 5))
    io.write("   new weight: " .. scaled_weight .. "\n")
    scaled_duration = scaled_duration / (1 - (slope * 5))
    io.write("   new speed: " .. scaled_duration .. "\n")
  end
  segment.weight = scaled_weight
  segment.duration = scaled_duration
end
