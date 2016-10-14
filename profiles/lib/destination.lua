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


function Destination.get_destination_forward(way)
  local destination_ref = way:get_value_by_key("destination:ref")
  local destination_forward = way:get_value_by_key("destination:forward")

  -- Assemble destination as: "A59: Düsseldorf, Köln"
  --          destination:ref  ^    ^  destination

  local rv = ""

  if destination_ref and destination_ref ~= "" then
    rv = rv .. string.gsub(destination_ref, ";", ", ")
  end

  if destination_forward and destination_forward ~= "" then
      if rv ~= "" then
          rv = rv .. ": "
      end

      rv = rv .. string.gsub(destination_forward, ";", ", ")
  end

  io.write('\n\nrv -- ' .. rv .. ' -- \n\n')

  return rv
end


function Destination.get_destination_backward(way)
  local destination_ref = way:get_value_by_key("destination:ref")
  local destination_backward = way:get_value_by_key("destination:backward")

  -- Assemble destination as: "A59: Düsseldorf, Köln"
  --          destination:ref  ^    ^  destination

  local rv = ""

  if destination_ref and destination_ref ~= "" then
    rv = rv .. string.gsub(destination_ref, ";", ", ")
  end

  if destination_backward and destination_backward ~= "" then
      if rv ~= "" then
          rv = rv .. ": "
      end

      rv = rv .. string.gsub(destination_backward, ";", ", ")
  end

  return rv
end

return Destination
