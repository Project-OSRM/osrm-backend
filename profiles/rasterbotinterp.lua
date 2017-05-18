-- Rasterbot with interpolation profile

functions = require('rasterbot')

functions.process_segment = function(profile, segment)
  local sourceData = raster:interpolate(profile.raster_source, segment.source.lon, segment.source.lat)
  local targetData = raster:interpolate(profile.raster_source, segment.target.lon, segment.target.lat)
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

return functions