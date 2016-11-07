TagCache = {}

function TagCache.get(data, key)
  local v = data.cache[key]
  if v then
    if v == '' then
      return nil
    else
      return v
    end
  else
    v = data.way:get_value_by_key(key)
    if v == nil then
      data.cache[key] = ''
      return nil
    else
      data.cache[key] = v
      if v == '' then
        return nil
      else
        return v
      end
    end
  end
end

function TagCache.set(data,key,value)
  if value then
    data.cache[key] = value
  else
    data.cache[key] = ''
  end    
end

return TagCache