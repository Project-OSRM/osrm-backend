TagCache = {}

function TagCache.get(way,cache,key)
  local v = cache[key]
  if v then
    if v == '' then
      return nil
    else
      return v
    end
  else
    v = way:get_value_by_key(key)
    if v == nil then
      cache[key] = ''
      return nil
    else
      cache[key] = v
      if v == '' then
        return nil
      else
        return v
      end
    end
  end
end

return TagCache