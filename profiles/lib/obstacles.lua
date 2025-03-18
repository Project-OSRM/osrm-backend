-- Assigns obstacle value to nodes as defined by
-- include/extractor/obstacles.hpp

local Obstacles = {}

-- process the obstacles at the given node
-- note: does not process barriers
function Obstacles.process_node(profile, node)
    local highway = node:get_value_by_key("highway")
    if highway then
        local type = obstacle_type[highway]
        -- barriers already handled in car.lua
        if type and type ~= obstacle_type.barrier then
            local direction = node:get_value_by_key("direction")
            local weight = 0
            local duration = 0
            local minor = false

            if type == obstacle_type.traffic_signals then
                -- traffic_signals:direction trumps direction
                direction = node:get_value_by_key("traffic_signals:direction") or direction
                -- traffic_signal_penalty is deprecated
                -- but there's still unit_tests using it
                duration = profile.properties.traffic_signal_penalty or 2
            end
            if type == obstacle_type.stop then
                if node:get_value_by_key("stop") == "minor" then
                    type = obstacle_type.stop_minor
                end
                duration = 2
            end
            if type == obstacle_type.give_way then
                duration = 1
            end
            obstacle_map:add(node, Obstacle.new(type, obstacle_direction[direction] or obstacle_direction.none, duration, weight))
        end
    end
end

-- return true if the source road of this turn is a minor road at the intersection
function Obstacles.entering_by_minor_road(turn)
    -- implementation: comparing road speeds (anybody has a better idea?)
    local max_speed = turn.target_speed
    for _, turn_leg in pairs(turn.roads_on_the_right) do
        max_speed = math.max(max_speed, turn_leg.speed)
    end
    for _, turn_leg in pairs(turn.roads_on_the_left) do
        max_speed = math.max(max_speed, turn_leg.speed)
    end
    return max_speed > turn.source_speed
end

return Obstacles
