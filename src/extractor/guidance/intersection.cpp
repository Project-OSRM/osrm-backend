#include "extractor/guidance/intersection.hpp"

#include <limits>
#include <string>

using osrm::util::angularDeviation;

namespace osrm
{
namespace extractor
{
namespace guidance
{

bool IntersectionViewData::CompareByAngle(const IntersectionViewData &other) const
{
    return angle < other.angle;
}

bool ConnectedRoad::compareByAngle(const ConnectedRoad &other) const { return angle < other.angle; }

void ConnectedRoad::mirror()
{
    const constexpr DirectionModifier::Enum mirrored_modifiers[] = {DirectionModifier::UTurn,
                                                                    DirectionModifier::SharpLeft,
                                                                    DirectionModifier::Left,
                                                                    DirectionModifier::SlightLeft,
                                                                    DirectionModifier::Straight,
                                                                    DirectionModifier::SlightRight,
                                                                    DirectionModifier::Right,
                                                                    DirectionModifier::SharpRight};

    static_assert(sizeof(mirrored_modifiers) / sizeof(DirectionModifier::Enum) ==
                      DirectionModifier::MaxDirectionModifier,
                  "The list of mirrored modifiers needs to match the available modifiers in size.");

    if (angularDeviation(angle, 0) > std::numeric_limits<double>::epsilon())
    {
        angle = 360 - angle;
        instruction.direction_modifier = mirrored_modifiers[instruction.direction_modifier];
    }
}

ConnectedRoad ConnectedRoad::getMirroredCopy() const
{
    ConnectedRoad copy(*this);
    copy.mirror();
    return copy;
}

std::string toString(const ConnectedRoad &road)
{
    std::string result = "[connection] ";
    result += std::to_string(road.eid);
    result += " allows entry: ";
    result += std::to_string(road.entry_allowed);
    result += " angle: ";
    result += std::to_string(road.angle);
    result += " bearing: ";
    result += std::to_string(road.bearing);
    result += " instruction: ";
    result += std::to_string(static_cast<std::int32_t>(road.instruction.type)) + " " +
              std::to_string(static_cast<std::int32_t>(road.instruction.direction_modifier)) + " " +
              std::to_string(static_cast<std::int32_t>(road.lane_data_id));
    return result;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
