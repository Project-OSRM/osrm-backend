local Directional = {}

-- return [forward,backward] values for a specific tag.
-- e.g. for maxspeed search forward:
--   maxspeed:forward
--   maxspeed
-- and backward:
--   maxspeed:backward
--   maxspeed

function Directional.get_values_by_key(way,data,key)
  local forward = way:get_value_by_key(key .. ':forward')
  local backward = way:get_value_by_key(key .. ':backward')
  
  if forward and backward then
    return forward, backward
  end

  local common = way:get_value_by_key(key)
  return forward or common,
         backward or common
end

-- return [forward,backward] values, searching a 
-- prioritized sequence of tags
-- e.g. for the sequence [maxspeed,advisory] search forward:
--   maxspeed:forward
--   maxspeed
--   advisory:forward
--   advisory
-- and for backward:
--   maxspeed:backward
--   maxspeed
--   advisory:backward
--   advisory

function Directional.get_values_by_set(way,data,keys)
  local forward, backward
  for i,key in ipairs(keys) do
    if not forward then
      forward = way:get_value_by_key(key .. ':forward')
    end
    if not backward then
      backward = way:get_value_by_key(key .. ':backward')
    end
    if not forward or not backward then
      local common = way:get_value_by_key(key)
      forward = forward or common
      backward = backward or common
    end
    if forward and backward then
      break
    end
  end

  return forward, backward
end

return Directional