#include <boost/test/unit_test.hpp>

#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"

using namespace osrm::util;

BOOST_AUTO_TEST_SUITE(antimeridian_tests)

BOOST_AUTO_TEST_CASE(project_point_on_segment_crossing_antimeridian)
{
    // segment from lon 179.5 to -179.5 (crosses antimeridian)
    FloatCoordinate source{FloatLongitude{179.5}, FloatLatitude{0}};
    FloatCoordinate target{FloatLongitude{-179.5}, FloatLatitude{0}};
    FloatCoordinate query{FloatLongitude{180.0}, FloatLatitude{0}}; // midpoint over antimeridian

    auto [ratio, projected] = coordinate_calculation::projectPointOnSegmentAntimeridian(source, target, query);

    // Expect the projected point to be near lon 180 / -180 (wrapped) and ratio around 0.5
    BOOST_CHECK_CLOSE(ratio, 0.5, 1.0); // 1% tolerance
    // Projected longitude should compare equal to 180 or -180 when converted to fixed coordinate
    BOOST_CHECK((static_cast<double>(projected.lon) > 179.0) ||
                (static_cast<double>(projected.lon) < -179.0));
}

BOOST_AUTO_TEST_SUITE_END()
