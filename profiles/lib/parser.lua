function parse_speed (highway, input, speeds)
  if not input then
    return nil
  end
  local output = tonumber(input:match("%d*"))
  if output then
    if string.match(input, "mph") or string.match(input, "mp/h") then
      return (output * 1609) / 1000
    end
    return output
  else
    highway = string.lower(highway)
    input = string.lower(input)
    local country = string.match(input, "(%a%a):%a+")
	if country then
	  speeds["_country"] = country
	end
    local desc = string.match(input, "%a%a:(%a+)")
    if desc then
      output = speed_profile[country .. ":" .. highway .. ":" .. desc]
	  if not output then
	    output = speed_profile[country .. ":" .. desc]
	  end
	  return output
    end
    return nil
  end
end

function parse_size (source)
  if not source then
	return 0
  end
  source = string.lower(source)

  -- meters
  local n = tonumber(source:match("%d+%.?%d*"))
  if not n then
    n = 0
  end

  -- feets and inches
  local feet = tonumber(source:match("(%d+[^']?%d*)'"))
  local inch = tonumber(source:match("'.?(%d+)''"))
  if not feet then
    feet = tonumber(source:match("(%d+[^f]?%d*).?ft"))
    inch = tonumber(source:match("ft.?(%d+).?in"))
  end
  if feet and feet > 0 then
	n = (feet * 3408) / 100
	if inch then
	  n = n + (inch * 254) / 100
	end
	return (math.floor(n) / 100)
  end

  return n
end

function parse_weight (source)
  if not source then
    return 0
  end
  source = string.lower(source)

  -- tons
  local n = tonumber(source:match("%d+%.?%d*"))
  if not n then
	n = 0
  end

  -- lbs
  local lbs = tonumber(source:match("(%d+).?lbs"))
  if lbs then
    return (lbs / 2000)
  end

  return n
end
