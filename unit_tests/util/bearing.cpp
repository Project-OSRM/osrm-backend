#include "util/bearing.hpp"
#include "util/typedefs.hpp"

#include <boost/functional/hash.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(bearing_test)

using namespace osrm;
using namespace osrm::util;

// Verify that the bearing-bounds checking function behaves as expected
BOOST_AUTO_TEST_CASE(bearing_range_test)
{
    // Simple, non-edge-case checks
    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(45, 45, 10));
    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(35, 45, 10));
    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(55, 45, 10));

    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(34, 45, 10));
    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(56, 45, 10));

    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(34, 45, 10));
    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(56, 45, 10));

    // When angle+limit goes > 360
    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(359, 355, 10));

    // When angle-limit goes < 0
    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(359, 5, 10));
    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(354, 5, 10));
    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(16, 5, 10));

    // Checking other cases of wraparound
    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(359, -5, 10));
    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(344, -5, 10));
    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(6, -5, 10));

    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(-1, 5, 10));
    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(-6, 5, 10));

    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(-721, 5, 10));
    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(719, 5, 10));

    BOOST_CHECK_EQUAL(false, bearing::CheckInBounds(1, 1, -1));
    BOOST_CHECK_EQUAL(true, bearing::CheckInBounds(1, 1, 0));
}

BOOST_AUTO_TEST_CASE(bearing_angle_test)
{
    BOOST_CHECK_EQUAL(bearing::angleBetween(257.78421507794314, 77.784215077943117), 0.);
    BOOST_CHECK_EQUAL(bearing::angleBetween(77.784215077943117, 257.78421507794314), 0.);

    // Check quadrants
    BOOST_CHECK_EQUAL(bearing::angleBetween(45., 45.), 180.);
    BOOST_CHECK_EQUAL(bearing::angleBetween(45., 315.), 270.);
    BOOST_CHECK_EQUAL(bearing::angleBetween(45., 225.), 0.);
    BOOST_CHECK_EQUAL(bearing::angleBetween(45., 135.), 90.);
    BOOST_CHECK_EQUAL(bearing::angleBetween(45., 90.), 135.);

    // Check edge cases
    BOOST_CHECK_EQUAL(bearing::angleBetween(0., 360.), 180.);
    BOOST_CHECK_EQUAL(bearing::angleBetween(360., 0.), 180.);
    BOOST_CHECK_EQUAL(bearing::angleBetween(0., 0.), 180.);
    BOOST_CHECK_EQUAL(bearing::angleBetween(360., 360.), 180.);

    BOOST_CHECK_EQUAL(bearing::angleBetween(0., 180.), 0.);
    BOOST_CHECK_EQUAL(bearing::angleBetween(360., 180.), 0.);
    BOOST_CHECK_EQUAL(bearing::angleBetween(180., 0.), 0.);
    BOOST_CHECK_EQUAL(bearing::angleBetween(180., 360.), 0.);

    BOOST_CHECK_EQUAL(bearing::angleBetween(90., 270.01), 359.99);

    // 5 digits loss due to cancellation in  90+180-269.99
    BOOST_CHECK_CLOSE(bearing::angleBetween(90., 269.99), 0.01, 1e-10);
}

BOOST_AUTO_TEST_SUITE_END()
