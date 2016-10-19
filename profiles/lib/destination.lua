local Destination = {}

function Destination.get_destination(way, is_forward)
  local destination = way:get_value_by_key("destination")
  local destination_forward = way:get_value_by_key("destination:forward")
  local destination_backward = way:get_value_by_key("destination:backward")
  local destination_ref = way:get_value_by_key("destination:ref")
  local destination_ref_forward = way:get_value_by_key("destination:ref:forward")
  local destination_ref_backward = way:get_value_by_key("destination:ref:backward")

  -- Assemble destination as: "A59: Düsseldorf, Köln"
  --          destination:ref  ^    ^  destination
  
  local rv = ""

  if destination_ref then
    if is_forward == true and destination_ref == "" then
      if destination_ref_forward then
        destination_ref = destination_ref_forward
      end
    elseif is_forward == false then
      if destination_ref_backward then
        destination_ref = destination_ref_backward
      end
    end

    rv = rv .. string.gsub(destination_ref, ";", ", ")
  end

  if destination then 
    if is_forward == true and destination == "" then
      if destination_forward then
        destination = destination_forward
      end
    elseif is_forward == false then
      if destination_backward then
        destination = destination_backward
      end
    end

    if destination ~= "" then
      if rv ~= "" then
          rv = rv .. ": "
      end

      rv = rv .. string.gsub(destination, ";", ", ")
    end
  end

  return rv
end

return Destination