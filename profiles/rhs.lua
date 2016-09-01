-- Testbot, with turn penalty
-- Used for testing turn penalties

require 'testbot'

properties.left_hand_driving = false

local turn_penalty           = 50
local turn_bias              = properties.left_hand_driving and 1/1.2 or 1.2

function turn_function (angle)
  ---- compute turn penalty as angle^2, with a left/right bias
  -- multiplying by 10 converts to deci-seconds see issue #1318
  k = 10*turn_penalty/(90.0*90.0)
  if angle>=0 then
    return angle*angle*k/turn_bias
  else
    return angle*angle*k*turn_bias
  end
end
