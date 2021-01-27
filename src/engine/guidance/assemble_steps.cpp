#include "engine/guidance/assemble_steps.hpp"

#include <boost/assert.hpp>

#include "util/bearing.hpp"
#include "util/log.hpp"

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

std::pair<short, short> getDepartBearings(const LegGeometry &leg_geometry,
                                          const PhantomNode &source_node,
                                          const bool traversed_in_reverse)
{
    BOOST_ASSERT(leg_geometry.locations.size() >= 2);
    const auto turn_coordinate = leg_geometry.locations.front();
    const auto post_turn_coordinate = *(leg_geometry.locations.begin() + 1);

    if (util::coordinate_calculation::haversineDistance(turn_coordinate, post_turn_coordinate) <= 1)
    {
        return std::make_pair<short, short>(0, source_node.GetBearing(traversed_in_reverse));
    }
    return std::make_pair<short, short>(
        0,
        std::round(util::coordinate_calculation::bearing(turn_coordinate, post_turn_coordinate)));
}

std::pair<short, short> getArriveBearings(const LegGeometry &leg_geometry,
                                          const PhantomNode &target_node,
                                          const bool traversed_in_reverse)
{
    BOOST_ASSERT(leg_geometry.locations.size() >= 2);
    const auto turn_coordinate = leg_geometry.locations.back();
    const auto pre_turn_coordinate = *(leg_geometry.locations.end() - 2);
    if (util::coordinate_calculation::haversineDistance(turn_coordinate, pre_turn_coordinate) <= 1)
    {
        return std::make_pair<short, short>(target_node.GetBearing(traversed_in_reverse), 0);
    }
    return std::make_pair<short, short>(
        std::round(util::coordinate_calculation::bearing(pre_turn_coordinate, turn_coordinate)), 0);
}
} // namespace detail
} // namespace guidance
} // namespace engine
} // namespace osrm
