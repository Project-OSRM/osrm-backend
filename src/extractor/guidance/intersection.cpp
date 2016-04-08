#include "extractor/guidance/intersection.hpp"

namespace osrm
{
namespace extractor
{
namespace guidance
{

ConnectedRoad::ConnectedRoad(const TurnOperation turn, const bool entry_allowed)
    : entry_allowed(entry_allowed), turn(turn)
{
}

std::string toString(const ConnectedRoad &road)
{
    std::string result = "[connection] ";
    result += std::to_string(road.turn.eid);
    result += " allows entry: ";
    result += std::to_string(road.entry_allowed);
    result += " angle: ";
    result += std::to_string(road.turn.angle);
    result += " instruction: ";
    result += std::to_string(static_cast<std::int32_t>(road.turn.instruction.type)) + " " +
              std::to_string(static_cast<std::int32_t>(road.turn.instruction.direction_modifier));
    return result;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
