#include "engine/guidance/assemble_steps.hpp"

#include <boost/assert.hpp>

#include <cmath>
#include <cstddef>

namespace osrm
{
namespace engine
{
namespace guidance
{
namespace detail
{

std::pair<short, short> getDepartBearings(const LegGeometry &leg_geometry)
{
    BOOST_ASSERT(leg_geometry.locations.size() >= 2);
    const auto turn_coordinate = leg_geometry.locations.front();
    const auto post_turn_coordinate = *(leg_geometry.locations.begin() + 1);
    return std::make_pair<short, short>(
        0,
        std::round(util::coordinate_calculation::bearing(turn_coordinate, post_turn_coordinate)));
}

std::pair<short, short> getArriveBearings(const LegGeometry &leg_geometry)
{
    BOOST_ASSERT(leg_geometry.locations.size() >= 2);
    const auto turn_coordinate = leg_geometry.locations.back();
    const auto pre_turn_coordinate = *(leg_geometry.locations.end() - 2);
    return std::make_pair<short, short>(
        std::round(util::coordinate_calculation::bearing(pre_turn_coordinate, turn_coordinate)), 0);
}
} // ns detail
} // ns engine
} // ns guidance
} // ns detail
