local Tags = require('lib/tags')
local Set = require('lib/set')

local Guidance = {}

-- Guidance: Default Mapping from roads to types/priorities
highway_classes = {
  motorway = road_priority_class.motorway,
  motorway_link = road_priority_class.link_road,
  trunk = road_priority_class.trunk,
  trunk_link = road_priority_class.link_road,
  primary = road_priority_class.primary,
  primary_link = road_priority_class.link_road,
  secondary = road_priority_class.secondary,
  secondary_link = road_priority_class.link_road,
  tertiary = road_priority_class.tertiary,
  tertiary_link = road_priority_class.link_road,
  unclassified = road_priority_class.side_residential,
  residential = road_priority_class.side_residential,
  service = road_priority_class.connectivity,
  living_street = road_priority_class.main_residential,
  track = road_priority_class.bike_path,
  path = road_priority_class.bike_path,
  footway = road_priority_class.foot_path,
  pedestrian = road_priority_class.foot_path,
  steps = road_priority_class.foot_path
}

default_highway_class = road_priority_class.connectivity;

motorway_types = Set {
  'motorway',
  'motorway_link',
  'trunk',
  'trunk_link'
}

-- these road types are set with a car in mind. For bicycle/walk we probably need different ones
road_types = Set {
  'motorway',
  'motorway_link',
  'trunk',
  'trunk_link',
  'primary',
  'primary_link',
  'secondary',
  'secondary_link',
  'tertiary',
  'tertiary_link',
  'unclassified',
  'residential',
  'living_street'
}

link_types = Set {
  'motorway_link',
  'trunk_link',
  'primary_link',
  'secondary_link',
  'tertiary_link'
}

function Guidance.set_classification (highway, result, input_way)
  if motorway_types[highway] then
    result.road_classification.motorway_class = true;
  end
  if link_types[highway] then
    result.road_classification.link_class = true;
  end
  if highway_classes[highway] ~= nil then
    result.road_classification.road_priority_class = highway_classes[highway]
  else
    result.road_classification.road_priority_class = default_highway_class
  end
  if road_types[highway] then
    result.road_classification.may_be_ignored = false;
  else
    result.road_classification.may_be_ignored = true;
  end

  local lane_count = input_way:get_value_by_key("lanes")
  if lane_count then
    local lc = tonumber(lane_count)
    if lc ~= nil then
      result.road_classification.num_lanes = lc
    end
  else
    local total_count = 0
    local forward_count = input_way:get_value_by_key("lanes:forward")
    if forward_count then
      local fc = tonumber(forward_count)
      if fc ~= nil then
        total_count = fc
      end
    end
    local backward_count = input_way:get_value_by_key("lanes:backward")
    if backward_count then
      local bc = tonumber(backward_count)
      if bc ~= nil then
        total_count = total_count + bc
      end
    end
    if total_count ~= 0 then
      result.road_classification.num_lanes = total_count
    end
  end
end

-- returns forward,backward psv lane count
local function get_psv_counts(way,data)
  local psv_forward, psv_backward = Tags.get_forward_backward_by_key(way,data,'lanes:psv')
  if psv_forward then
    psv_forward = tonumber(psv_forward)
  end
  if psv_backward then
    psv_backward = tonumber(psv_backward)
  end
  return psv_forward or 0,
         psv_backward or 0
end

-- trims lane string with regard to supported lanes
local function process_lanes(turn_lanes,vehicle_lanes,first_count,second_count)
  if turn_lanes then
    if vehicle_lanes then
      return applyAccessTokens(turn_lanes,vehicle_lanes)
    elseif first_count ~= 0 or second_count ~= 0 then
      return trimLaneString(turn_lanes, first_count, second_count)
    else
      return turn_lanes
    end
  end
end

-- this is broken for left-sided driving. It needs to switch left and right in case of left-sided driving
function Guidance.get_turn_lanes(way,data)
  local psv_fw, psv_bw = get_psv_counts(way,data)
  local turn_lanes_fw, turn_lanes_bw = Tags.get_forward_backward_by_key(way,data,'turn:lanes')
  local vehicle_lanes_fw, vehicle_lanes_bw = Tags.get_forward_backward_by_key(way,data,'vehicle:lanes')
  
  --note: backward lanes swap psv_bw and psv_fw
  return process_lanes(turn_lanes_fw,vehicle_lanes_fw,psv_bw,psv_fw) or turn_lanes,
         process_lanes(turn_lanes_bw,vehicle_lanes_bw,psv_fw,psv_bw) or turn_lanes
end

return Guidance
