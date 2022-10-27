local GiveWay = {}

function GiveWay.get_value(node)
    local tag = node:get_value_by_key("highway")
    if "give_way" == tag then
        local direction = node:get_value_by_key("direction")
        if direction then
            if "forward" == direction then
                return give_way.direction_forward
            end
            if "backward" == direction then
                return give_way.direction_reverse
            end
        end
        -- return give_way.direction_all
        return true
    end
    -- return give_way.none
    return false
end

return GiveWay

