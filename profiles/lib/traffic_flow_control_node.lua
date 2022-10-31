local TrafficFlowControlNode = {}

function TrafficFlowControlNode.get_value(node, tag_name)
    local tag = node:get_value_by_key("highway")
    if tag_name == tag then
        local direction = node:get_value_by_key("direction")
        if direction then
            if "forward" == direction then
                return traffic_flow_control_direction.direction_forward
            end
            if "backward" == direction then
                return traffic_flow_control_direction.direction_reverse
            end
        end
        return traffic_flow_control_direction.direction_all
    end
    return traffic_flow_control_direction.none
end

return TrafficFlowControlNode

