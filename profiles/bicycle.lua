-- Bicycle profile

local Mode = require('lib/bicycle')

-- these need to be global because they are accesed externaly

u_turn_penalty                  = 20
traffic_signal_penalty          = 2
use_turn_restrictions           = false

function get_exceptions(vector)
  for i,v in ipairs(Mode.settings.access.tags) do
    vector:Add(v)
  end
end

function node_function (node, result)
  Mode:process_node(node,result)
end

function way_function (way, result) 
  Mode:process(way,result)
end

function turn_function (angle)
  return Mode:turn(angle)
end
