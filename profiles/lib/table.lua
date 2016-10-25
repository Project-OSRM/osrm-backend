-- Utilities for copying and merging tables
-- When merging, Sets are merged, while Sequences are simply copied.
-- Note: Does not currently handle recursive tables

Table = {}

function Table.shallow_copy(orig)
    local orig_type = type(orig)
    local copy
    if orig_type == 'table' then
        copy = {}
        for orig_key, orig_value in pairs(orig) do
            copy[orig_key] = orig_value
        end
    else -- number, string, boolean, etc
        copy = orig
    end
    return copy
end

function Table.deep_copy(obj)
  if type(obj) ~= 'table' then
    return obj
  end
  local s = seen or {}
  local res = setmetatable({}, getmetatable(obj))
  s[obj] = res
  for k, v in pairs(obj) do
    res[Table.deep_copy(k)] = Table.deep_copy(v)
  end
  return res
end

function Table.deep_merge(from, to)
  if type(from) ~= 'table' then
    return from
  else
    if to then
      local res = Table.shallow_copy(to)
      for k, v in pairs(from) do
        res[k] = Table.deep_merge(v, to[k] )
      end
      return res
    else
      return Table.deep_copy(from)
    end
  end
end

return Table