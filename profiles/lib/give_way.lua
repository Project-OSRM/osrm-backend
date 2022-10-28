local GiveWay = {}

function GiveWay.get_value(node)
    local tag = node:get_value_by_key("highway")
    if "give_way" == tag then
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

return GiveWay

