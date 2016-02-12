-- Set of items
-- Fast check for inclusion, but unordered.
-- 
-- Instead of having to do:
-- whitelist = { 'apple'=true, 'cherries'=true, 'melons'=true }
-- 
-- you can do:
-- whitelist = Set { 'apple', 'cherries', 'melons' }
--
-- and then use it as:
-- print( whitelist['cherries'] )     => true

function Set(source)
  set = {}
  if source then
    for i,v in ipairs(source) do
      set[v] = true
    end
  end
  return set
end

return Set