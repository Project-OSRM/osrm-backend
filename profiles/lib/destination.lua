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

-- Assemble destination as: "A59: Düsseldorf, Köln"
--          destination:ref  ^    ^  destination

function Destination.get_destination(way, is_forward)
  ref  = Destination.get_directional_tag(way, is_forward, 'destination:ref')
  dest = Destination.get_directional_tag(way, is_forward, 'destination')
  street = Destination.get_directional_tag(way, is_forward, 'destination:street')
  if ref and dest then
    return ref .. ': ' .. dest
  else
    return ref or dest or street or ''
  end
end

return Destination
