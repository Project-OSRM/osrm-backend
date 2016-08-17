#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/toolkit.hpp"

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
    result += " bearing: ";
    result += std::to_string(road.turn.bearing);
    result += " instruction: ";
    result += std::to_string(static_cast<std::int32_t>(road.turn.instruction.type)) + " " +
              std::to_string(static_cast<std::int32_t>(road.turn.instruction.direction_modifier)) +
              " " + std::to_string(static_cast<std::int32_t>(road.turn.lane_data_id));
    return result;
}

Intersection::iterator findClosestTurn(Intersection &intersection, const double angle)
{
    return std::min_element(intersection.begin(),
                            intersection.end(),
                            [angle](const ConnectedRoad &lhs, const ConnectedRoad &rhs) {
                                return angularDeviation(lhs.turn.angle, angle) <
                                       angularDeviation(rhs.turn.angle, angle);
                            });
}
Intersection::const_iterator findClosestTurn(const Intersection &intersection, const double angle)
{
    return std::min_element(intersection.cbegin(),
                            intersection.cend(),
                            [angle](const ConnectedRoad &lhs, const ConnectedRoad &rhs) {
                                return angularDeviation(lhs.turn.angle, angle) <
                                       angularDeviation(rhs.turn.angle, angle);
                            });
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
