-- Stand-in class used when running the processors manually form other LUA code, instead
-- of from the C++ side.

Tags = {}

function Tags:new(object)
  object = object or {}
  setmetatable(object, self)
  self.__index = self         -- trick to save memory, see http://lua-users.org/lists/lua-l/2013-04/msg00617.html
  return object
end

function Tags:get_value_by_key(k)
  return self[k]
end

return Tags