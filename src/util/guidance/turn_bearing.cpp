#include "util/guidance/turn_bearing.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace util
{
namespace guidance
{

constexpr double bearing_scale = 360.0 / 256.0;

// discretizes a bearing into distinct units of 1.4 degrees
TurnBearing::TurnBearing(const double value) : bearing(value / bearing_scale)
{
    BOOST_ASSERT_MSG(value >= 0 && value <= 360.0, "Bearing value needs to be between 0 and 360");
}

double TurnBearing::Get() const { return bearing * bearing_scale; }

} // namespace guidance
} // namespace util
} // namespace osrm
