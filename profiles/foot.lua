-- Foot profile

local Mode = require('lib/foot')

if not properties then
  properties = {}
end

properties.traffic_signal_penalty        = 2
properties.u_turn_penalty                = 2
properties.use_turn_restrictions         = false
properties.continue_straight_at_waypoint = false

function get_restrictions(vector)
  for i,v in ipairs(Mode.settings.access.tags) do
    vector:Add(v)
  end
end

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
