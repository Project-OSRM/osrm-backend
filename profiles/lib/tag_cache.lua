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

function TagCache.set(cache,key,value)
  if value then
    cache[key] = value
  else
    cache[key] = ''
  end    
end

return TagCache