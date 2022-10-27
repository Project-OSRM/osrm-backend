local StopSign = {}

function StopSign.get_value(node)
    local tag = node:get_value_by_key("highway")
    if "stop" == tag then
        local direction = node:get_value_by_key("direction")
        if direction then
            if "forward" == direction then
                return stop_sign.direction_forward
            end
            if "backward" == direction then
                return stop_sign.direction_reverse
            end
        end
        return stop_sign.direction_all
    end
    return stop_sign.none
end

return StopSign

