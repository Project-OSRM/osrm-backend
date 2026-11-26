-- CO2 emission model used by the 'co2' and 'routability-co2' weights.
--
-- The weight unit is the gram of CO2. A way's rate is therefore expressed in
-- meters per gram: rate = 1000 / emission factor (g/km) at the way's speed.
--
-- The emission factor depends on the travel speed through a piecewise linear
-- curve (profile.co2.curve). Speeds below the first point or above the last
-- point use the first or last value. Time spent at turns, traffic signals and
-- other obstacles is converted with an idle rate (profile.co2.idle_g_per_s).

local Co2 = {}

-- true when the profile routes on a CO2 based weight
function Co2.is_co2_weight(profile)
  local name = profile.properties.weight_name
  return name == 'co2' or name == 'routability-co2'
end

-- emission factor in g/km for a speed in km/h
function Co2.grams_per_km(profile, speed)
  local curve = profile.co2.curve
  local n = #curve
  if speed <= curve[1][1] then
    return curve[1][2]
  end
  for i = 2, n do
    local s1, g1 = curve[i][1], curve[i][2]
    if speed <= s1 then
      local s0, g0 = curve[i-1][1], curve[i-1][2]
      return g0 + (g1 - g0) * (speed - s0) / (s1 - s0)
    end
  end
  return curve[n][2]
end

-- rate in meters per gram of CO2 for a speed in km/h
function Co2.rate(profile, speed)
  return 1000 / Co2.grams_per_km(profile, speed)
end

-- sanity check of the configuration, called once from setup()
function Co2.validate(profile)
  local curve = profile.co2 and profile.co2.curve
  assert(curve and #curve >= 1, "co2.curve must contain at least one {speed, grams_per_km} point")
  for i, point in ipairs(curve) do
    assert(point[1] > 0 and point[2] > 0, "co2.curve values must be positive")
    if i > 1 then
      assert(point[1] > curve[i-1][1], "co2.curve speeds must be strictly increasing")
    end
  end
  assert(profile.co2.idle_g_per_s > 0, "co2.idle_g_per_s must be positive")
  assert(profile.co2.ferry_g_per_km > 0, "co2.ferry_g_per_km must be positive")
end

return Co2
