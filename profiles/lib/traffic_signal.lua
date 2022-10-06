-- Assigns traffic light value to node as defined by
-- include/extractor/traffic_lights.hpp

local TrafficSignal = {}

function TrafficSignal.get_value(node)
    local tag = node:get_value_by_key("highway")
    if "traffic_signals" == tag then
        local direction = node:get_value_by_key("traffic_signals:direction")
        if direction then
            if "forward" == direction then
                return traffic_lights.direction_forward
            end
            if "backward" == direction then
                return traffic_lights.direction_reverse
            end
        end
        -- return traffic_lights.direction_all
        return true
    end
    -- return traffic_lights.none
    return false
end

return TrafficSignal

