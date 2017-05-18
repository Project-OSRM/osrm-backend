-- Testbot, with turn penalty
-- Used for testing turn penalties

functions = require 'testbot'

functions.process_turn = function(profile, turn)
  turn.duration = 20 * math.abs(turn.angle) / 180
end

return functions