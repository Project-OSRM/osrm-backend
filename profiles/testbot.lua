-- Testbot profile

-- Moves at fixed, well-known speeds, practical for testing speed and travel times:

-- Primary road:  36km/h = 36000m/3600s = 100m/10s
-- Secondary road:  18km/h = 18000m/3600s = 100m/20s
-- Tertiary road:  12km/h = 12000m/3600s = 100m/30s

speed_profile = {
  ["primary"] = 36,
  ["secondary"] = 18,
  ["tertiary"] = 12,
  ["steps"] = 6,
  ["default"] = 24
}

-- these settings are read directly by osrm

properties.continue_straight_at_waypoint = true
properties.use_turn_restrictions         = true
properties.traffic_signal_penalty        = 7     -- seconds
properties.u_turn_penalty                = 20

function limit_speed(speed, limits)
  -- don't use ipairs(), since it stops at the first nil value
  for i=1, #limits do
    limit = limits[i]
    if limit ~= nil and limit > 0 then
      if limit < speed then
        return limit        -- stop at first speedlimit that's smaller than speed
      end
    end
  end
  return speed
end

function node_function (node, result)
  local traffic_signal = node:get_value_by_key("highway")

  if traffic_signal and traffic_signal == "traffic_signals" then
    result.traffic_lights = true
    -- TODO: a way to set the penalty value
  end
end

function way_function (way, result)
  local highway = way:get_value_by_key("highway")
  local name = way:get_value_by_key("name")
  local oneway = way:get_value_by_key("oneway")
  local route = way:get_value_by_key("route")
  local duration = way:get_value_by_key("duration")
  local maxspeed = tonumber(way:get_value_by_key ( "maxspeed"))
  local maxspeed_forward = tonumber(way:get_value_by_key( "maxspeed:forward"))
  local maxspeed_backward = tonumber(way:get_value_by_key( "maxspeed:backward"))
  local junction = way:get_value_by_key("junction")

  if name then
    result.name = name
  end

  result.forward_mode = mode.driving
  result.backward_mode = mode.driving

  if duration and durationIsValid(duration) then
    result.duration = math.max( 1, parseDuration(duration) )
    result.forward_mode = mode.route
    result.backward_mode = mode.route
  else
    local speed_forw = speed_profile[highway] or speed_profile['default']
    local speed_back = speed_forw

    if highway == "river" then
      local temp_speed = speed_forw
      result.forward_mode = mode.river_down
      result.backward_mode = mode.river_up
      speed_forw = temp_speed*1.5
      speed_back = temp_speed/1.5
    elseif highway == "steps" then
      result.forward_mode = mode.steps_down
      result.backward_mode = mode.steps_up
    end

    if maxspeed_forward ~= nil and maxspeed_forward > 0 then
      speed_forw = maxspeed_forward
    else
      if maxspeed ~= nil and maxspeed > 0 and speed_forw > maxspeed then
        speed_forw = maxspeed
      end
    end

    if maxspeed_backward ~= nil and maxspeed_backward > 0 then
      speed_back = maxspeed_backward
    else
      if maxspeed ~=nil and maxspeed > 0 and speed_back > maxspeed then
        speed_back = maxspeed
      end
    end

    result.forward_speed = speed_forw
    result.backward_speed = speed_back
  end

  if oneway == "no" or oneway == "0" or oneway == "false" then
    -- nothing to do
  elseif oneway == "-1" then
    result.forward_mode = mode.inaccessible
  elseif oneway == "yes" or oneway == "1" or oneway == "true" or junction == "roundabout" then
    result.backward_mode = mode.inaccessible
  end

  if junction == 'roundabout' then
    result.roundabout = true
  end
end
