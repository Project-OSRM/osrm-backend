#include "guidance/turn_classification.hpp"

#include "guidance/intersection.hpp"
#include "guidance/turn_instruction.hpp"
#include "util/guidance/entry_class.hpp"
#include "util/typedefs.hpp"

#include <boost/test/unit_test.hpp>

#include <cstddef>

BOOST_AUTO_TEST_SUITE(turn_classification)

using namespace osrm;
using namespace osrm::guidance;

/**
 * An intersection with more roads than EntryClass can hold: a vertex of a meshed
 * plaza, where every line of sight is a way and the bearings are too close to be
 * discretized.  The roads within the capacity are recorded and answer entry, the rest
 * answer none, and the classification carries on.
 */
BOOST_AUTO_TEST_CASE(records_the_roads_within_capacity_and_no_more)
{
    const std::size_t capacity = util::guidance::EntryClass::CAPACITY;
    const std::size_t roads = capacity + 8;

    Intersection intersection;
    for (std::size_t i = 0; i < roads; ++i)
    {
        const double bearing = 360.0 * static_cast<double>(i) / static_cast<double>(roads);
        const extractor::intersection::IntersectionEdgeGeometry geometry{
            static_cast<EdgeID>(i), bearing, bearing, 10.0};
        const extractor::intersection::IntersectionViewData view{geometry, true, bearing};
        intersection.push_back(
            ConnectedRoad{view, TurnInstruction::NO_TURN(), INVALID_LANE_DATAID});
    }

    const auto entry_class = classifyIntersection(intersection, util::Coordinate{}).first;

    for (std::size_t i = 0; i < roads; ++i)
    {
        BOOST_CHECK_EQUAL(entry_class.allowsEntry(static_cast<std::uint32_t>(i)), i < capacity);
    }
}

BOOST_AUTO_TEST_SUITE_END()
