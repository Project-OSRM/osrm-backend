#include "guidance/intersection.hpp"

#include <limits>
#include <string>

#include <boost/range/adaptors.hpp>

namespace osrm::extractor::intersection
{

bool IntersectionViewData::CompareByAngle(const IntersectionViewData &other) const
{
    return angle < other.angle;
}

std::string toString(const IntersectionEdgeGeometry &shape)
{
    std::string result = "[shape] " + std::to_string(shape.eid) +
                         " bearing: " + std::to_string(shape.perceived_bearing);
    return result;
}

std::string toString(const IntersectionViewData &view)
{
    std::string result = "[view] ";
    result += std::to_string(view.eid);
    result += " allows entry: ";
    result += std::to_string(view.entry_allowed);
    result += " angle: ";
    result += std::to_string(view.angle);
    result += " bearing: ";
    result += std::to_string(view.perceived_bearing);
    return result;
}

} // namespace osrm::extractor::intersection
