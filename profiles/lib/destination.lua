local Destination = {}

function Destination.get_destination(way)
  local destination = way:get_value_by_key("destination")
  local destination_ref = way:get_value_by_key("destination:ref")

  -- Assemble destination as: "A59: Düsseldorf, Köln"
  --          destination:ref  ^    ^  destination

  local rv = ""

  if destination_ref and destination_ref ~= "" then
    rv = rv .. string.gsub(destination_ref, ";", ", ")
  end

  if destination and destination ~= "" then
      if rv ~= "" then
          rv = rv .. ": "
      end

      rv = rv .. string.gsub(destination, ";", ", ")
  end

  return rv
end

return Destination
