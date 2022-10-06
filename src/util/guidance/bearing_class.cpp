#include "util/guidance/bearing_class.hpp"
#include "util/bearing.hpp"

#include <algorithm>
#include <boost/assert.hpp>

namespace osrm
{
namespace util
{
namespace guidance
{

BearingClass::BearingClass() { available_bearings.reserve(10); }

bool BearingClass::operator==(const BearingClass &other) const
{
    BOOST_ASSERT(std::is_sorted(available_bearings.begin(), available_bearings.end()));
    BOOST_ASSERT(std::is_sorted(other.available_bearings.begin(), other.available_bearings.end()));
    if (other.available_bearings.size() != available_bearings.size())
        return false;
    for (std::size_t i = 0; i < available_bearings.size(); ++i)
        if (available_bearings[i] != other.available_bearings[i])
            return false;
    return true;
}

bool BearingClass::operator<(const BearingClass &other) const
{
    BOOST_ASSERT(std::is_sorted(available_bearings.begin(), available_bearings.end()));
    BOOST_ASSERT(std::is_sorted(other.available_bearings.begin(), other.available_bearings.end()));
    if (available_bearings.size() < other.available_bearings.size())
        return true;
    if (available_bearings.size() > other.available_bearings.size())
        return false;

    for (std::size_t i = 0; i < available_bearings.size(); ++i)
    {
        if (available_bearings[i] < other.available_bearings[i])
            return true;
        if (available_bearings[i] > other.available_bearings[i])
            return false;
    }

    return false;
}

void BearingClass::add(const DiscreteBearing bearing) { available_bearings.push_back(bearing); }

const std::vector<DiscreteBearing> &BearingClass::getAvailableBearings() const
{
    return available_bearings;
}

DiscreteBearing BearingClass::getDiscreteBearing(const double bearing)
{
    BOOST_ASSERT(0. <= bearing && bearing <= 360.);
    auto shifted_bearing = (bearing + 0.5 * discrete_step_size);
    if (shifted_bearing >= 360.)
        shifted_bearing -= 360;
    return static_cast<DiscreteBearing>(shifted_bearing / discrete_step_size);
}

std::size_t BearingClass::findMatchingBearing(const double bearing) const
{
    BOOST_ASSERT(!available_bearings.empty());
    // the small size of the intersections allows a linear compare
    auto discrete_bearing = static_cast<DiscreteBearing>(bearing);
    auto max_element =
        std::max_element(available_bearings.begin(),
                         available_bearings.end(),
                         [&](const DiscreteBearing first, const DiscreteBearing second) {
                             return angularDeviation(first, discrete_bearing) >
                                    angularDeviation(second, discrete_bearing);
                         });

    return std::distance(available_bearings.begin(), max_element);
}

} // namespace guidance
} // namespace util
} // namespace osrm
