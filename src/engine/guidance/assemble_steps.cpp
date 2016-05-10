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
namespace
{
void fillInIntermediate(Intersection &intersection,
                        const LegGeometry &leg_geometry,
                        const std::size_t segment_index)
{
    auto turn_index = leg_geometry.BackIndex(segment_index);
    BOOST_ASSERT(turn_index > 0);
    BOOST_ASSERT(turn_index + 1 < leg_geometry.locations.size());

    // TODO chose a bigger look-a-head to smooth complex geometry
    const auto pre_turn_coordinate = leg_geometry.locations[turn_index - 1];
    const auto turn_coordinate = leg_geometry.locations[turn_index];
    const auto post_turn_coordinate = leg_geometry.locations[turn_index + 1];

    intersection.bearing_before =
        util::coordinate_calculation::bearing(pre_turn_coordinate, turn_coordinate);
    intersection.bearing_after =
        util::coordinate_calculation::bearing(turn_coordinate, post_turn_coordinate);

    intersection.location = turn_coordinate;
}

void fillInDepart(Intersection &intersection, const LegGeometry &leg_geometry)
{
    BOOST_ASSERT(leg_geometry.locations.size() >= 2);
    const auto turn_coordinate = leg_geometry.locations.front();
    const auto post_turn_coordinate = *(leg_geometry.locations.begin() + 1);
    intersection.location = turn_coordinate;
    intersection.bearing_before = 0;
    intersection.bearing_after =
        util::coordinate_calculation::bearing(turn_coordinate, post_turn_coordinate);
    std::cout << "Depart: " << intersection.bearing_before << " " << intersection.bearing_after << std::endl;
}

void fillInArrive(Intersection &intersection, const LegGeometry &leg_geometry)
{
    BOOST_ASSERT(leg_geometry.locations.size() >= 2);
    const auto turn_coordinate = leg_geometry.locations.back();
    const auto pre_turn_coordinate = *(leg_geometry.locations.end() - 2);
    intersection.location = turn_coordinate;
    intersection.bearing_before =
        util::coordinate_calculation::bearing(pre_turn_coordinate, turn_coordinate);
    intersection.bearing_after = 0;
    std::cout << "Arrive: " << intersection.bearing_before << " " << intersection.bearing_after << std::endl;
}
} // namespace

Intersection intersectionFromGeometry(const WaypointType waypoint_type,
                                      const double segment_duration,
                                      const LegGeometry &leg_geometry,
                                      const std::size_t segment_index)
{
    Intersection intersection;
    intersection.duration = segment_duration;
    intersection.distance = leg_geometry.segment_distances[segment_index];
    switch (waypoint_type)
    {
    case WaypointType::None:
        fillInIntermediate(intersection, leg_geometry, segment_index);
        break;
    case WaypointType::Depart:
        fillInDepart(intersection, leg_geometry);
        break;
    case WaypointType::Arrive:
        fillInArrive(intersection, leg_geometry);
        break;
    }
    return intersection;
}
} // ns detail
} // ns engine
} // ns guidance
} // ns detail
