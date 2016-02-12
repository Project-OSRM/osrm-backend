-- Car profile

local Mode = require('lib/car')

if not properties then
  properties = {}
end

properties.u_turn_penalty                  = 20
properties.traffic_signal_penalty          = 2
properties.use_turn_restrictions           = true
properties.continue_straight_at_waypoint   = true
properties.left_hand_driving               = false

function get_name_suffix_list(vector)
  -- A list of suffixes to suppress in name change instructions
  local suffix_list = { "N", "NE", "E", "SE", "S", "SW", "W", "NW", "North", "South", "West", "East" }
  for index,suffix in ipairs(suffix_list) do
      vector:Add(suffix)
  end
end

function get_restrictions(vector)
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
