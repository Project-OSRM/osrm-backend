api_version = 0

-- Testbot, with turn penalty
-- Used for testing turn penalties

require 'testbot'

function turn_function (angle)
    -- multiplying by 10 converts to deci-seconds see issue #1318
    return 10*20*math.abs(angle)/180
end
