TagCache = {}

function TagCache.get(way,cache,key)
  local v = cache[key]
  if v == nil then
    v = way:get_value_by_key(key)
    cache[key] = v or false
    return v
  elseif v == false then
    return nil
  else
    return v
  end
end

return TagCache