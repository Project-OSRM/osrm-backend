local Destination = {}

function Destination.get_directional_tag(way, is_forward, tag)
  local v
  if is_forward then
    v = way:get_value_by_key(tag .. ':forward') or way:get_value_by_key(tag)
  else
    v = way:get_value_by_key(tag .. ':backward') or way:get_value_by_key(tag)
  end
  if v then
    return v.gsub(v, ';', ', ')
  end
end

function Destination.join(a,b)
  if a and b then
    return a .. ': ' .. b
  else
    return a or b
  end
end

-- Assemble destination as: "A59: Düsseldorf, Köln"
--          destination:ref  ^    ^  destination

function Destination.get_destination(way, is_forward)
  destination_ref = Destination.get_directional_tag(way, is_forward, 'destination:ref')
  destination     = Destination.get_directional_tag(way, is_forward, 'destination')
  return Destination.join(destination_ref, destination) or ''
end

return Destination