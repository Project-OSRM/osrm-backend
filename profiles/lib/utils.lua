-- Profile functions to implement common algorithms of data processing
--
-- You can run a selection you find useful in your profile,
-- or do you own processing if/when required.

Utils = {}

-- split string 'a; b; c' to table with values ['a', 'b', 'c']
-- so it use just one separator ';'
function Utils.string_list_tokens(str)
  result = {}
  local idx = 0
  for s in str.gmatch(str, "([^;]*)") do
    if s ~= nil and s ~= '' then
      idx = idx + 1
      result[idx] = s:gsub("^%s*(.-)%s*$", "%1")
    end
  end

  return result
end

-- same as Utils.StringListTokens, but with many possible separators:
-- ',' | ';' | ' '| '(' | ')'
function Utils.tokenize_common(str)
  result = {}
  local idx = 0
  for s in str.gmatch(str, "%S+") do
    if s ~= nil and s ~= '' then
      idx = idx + 1
      result[idx] = s:gsub("^%s*(.-)%s*$", "%1")
    end
  end

  return result
end

-- returns true, if string contains a number
function Utils.is_number(str)
  return (tonumber(str) ~= nil)
end

return Utils