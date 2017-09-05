-- Profile functions dealing with various aspects of relation parsing
--
-- You can run a selection you find useful in your profile,
-- or do you own processing if/when required.

Relations = {}

function Relations.Merge(relations)
  local result = {}

  for _, r in ipairs(relations) do
    for k, v in pairs(r) do
      result[k] = v
    end
  end

  return result
end

return Relations