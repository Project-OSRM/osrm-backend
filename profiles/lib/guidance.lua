local Guidance = {}

-- Guidance: Default Mapping from roads to types/priorities
highway_classes = { ["motorway"] = road_priority_class.motorway,
                    ["motorway_link"] = road_priority_class.link_road,
                    ["trunk"] = road_priority_class.trunk,
                    ["trunk_link"] = road_priority_class.link_road,
                    ["primary"] = road_priority_class.primary,
                    ["primary_link"] = road_priority_class.link_road,
                    ["secondary"] = road_priority_class.secondary,
                    ["secondary_link"] = road_priority_class.link_road,
                    ["tertiary"] = road_priority_class.tertiary,
                    ["tertiary_link"] = road_priority_class.link_road,
                    ["unclassified"] = road_priority_class.side_residential,
                    ["residential"] = road_priority_class.side_residential,
                    ["service"] = road_priority_class.connectivity,
                    ["living_street"] = road_priority_class.main_residential,
                    ["track"] = road_priority_class.bike_path,
                    ["path"] = road_priority_class.bike_path,
                    ["footway"] = road_priority_class.foot_path,
                    ["pedestrian"] = road_priority_class.foot_path,
                    ["steps"] = road_priority_class.foot_path}

default_highway_class = road_priority_class.connectivity;

motorway_types = { ["motorway"] = true, ["motorway_link"] = true, ["trunk"] = true, ["trunk_link"] = true }

-- these road types are set with a car in mind. For bicycle/walk we probably need different ones
road_types = { ["motorway"] = true,
               ["motorway_link"] = true,
               ["trunk"] = true,
               ["trunk_link"] = true,
               ["primary"] = true,
               ["primary_link"] = true,
               ["secondary"] = true,
               ["secondary_link"] = true,
               ["tertiary"] = true,
               ["tertiary_link"] = true,
               ["unclassified"] = true,
               ["residential"] = true,
               ["living_street"] = true }

link_types = { ["motorway_link"] = true, ["trunk_link"] = true, ["primary_link"] = true, ["secondary_link"] = true, ["tertiary_link"] = true }

function Guidance.set_classification (highway, result)
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
end

-- returns forward,backward psv lane count
local function get_psv_counts(way)
    local psv = way:get_value_by_key("lanes:psv")
    local psv_forward = way:get_value_by_key("lanes:psv:forward");
    local psv_backward = way:get_value_by_key("lanes:psv:backward");

    local fw = 0;
    local bw = 0;
    if( psv and psv ~= "" ) then
        fw = tonumber(psv)
        if( fw == nil ) then
            fw = 0
        end
    end 
    if( psv_forward and psv_forward ~= "" ) then
        fw = tonumber(psv_forward)
        if( fw == nil ) then
            fw = 0
        end
    end 
    if( psv_backward and psv_backward ~= "" ) then
        bw = tonumber(psv_backward);
        if( bw == nil ) then
            fw = 0
        end
    end
    return fw, bw
end

-- this is broken for left-sided driving. It needs to switch left and right in case of left-sided driving
function Guidance.get_turn_lanes(way)
    local fw_psv = 0
    local bw_psv = 0 
    fw_psv, bw_psv = get_psv_counts(way)

    local turn_lanes = way:get_value_by_key("turn:lanes")
    local turn_lanes_fw = way:get_value_by_key("turn:lanes:forward")
    local turn_lanes_bw = way:get_value_by_key("turn:lanes:backward")

    if( fw_psv ~= 0 or bw_psv ~= 0 ) then
        turn_lanes = trimLaneString(turn_lanes, bw_psv, fw_psv )
        turn_lanes_fw = trimLaneString(turn_lanes_fw, bw_psv, fw_psv )
        --backwards turn lanes need to treat bw_psv as fw_psv and vice versa
        turn_lanes_bw = trimLaneString(turn_lanes_bw, fw_psv, bw_psv )
    end

    return turn_lanes, turn_lanes_fw, turn_lanes_bw
end

return Guidance
