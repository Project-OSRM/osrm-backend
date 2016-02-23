#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/unit_test.hpp>

#include "engine/polyline_compressor.hpp"
#include "util/coordinate_calculation.hpp"

#include <osrm/coordinate.hpp>

#include <cmath>
#include <vector>

BOOST_AUTO_TEST_SUITE(polyline)

using namespace osrm;
using namespace osrm::engine;

BOOST_AUTO_TEST_CASE(decode)
{
    // Polyline string for the 5 coordinates
    const std::string polyline = "_c`|@_c`|@o}@_pRo}@_pRo}@_pRo}@_pR";
    const auto coords = decodePolyline(polyline);

    // Test coordinates; these would be the coordinates we give the loc parameter,
    // e.g. loc=10.00,10.0&loc=10.01,10.1...
    util::Coordinate coord1(util::FloatLongitude(10.0), util::FloatLatitude(10.00));
    util::Coordinate coord2(util::FloatLongitude(10.1), util::FloatLatitude(10.01));
    util::Coordinate coord3(util::FloatLongitude(10.2), util::FloatLatitude(10.02));
    util::Coordinate coord4(util::FloatLongitude(10.3), util::FloatLatitude(10.03));
    util::Coordinate coord5(util::FloatLongitude(10.4), util::FloatLatitude(10.04));

    // Put the test coordinates into the vector for comparison
    std::vector<util::Coordinate> cmp_coords = {coord1, coord2, coord3, coord4, coord5};

    BOOST_CHECK_EQUAL(cmp_coords.size(), coords.size());

    for (unsigned i = 0; i < cmp_coords.size(); ++i)
    {
        BOOST_CHECK_CLOSE(static_cast<double>(util::toFloating(coords[i].lat)),
                          static_cast<double>(util::toFloating(cmp_coords[i].lat)), 0.0001);
        BOOST_CHECK_CLOSE(static_cast<double>(util::toFloating(coords[i].lon)),
                          static_cast<double>(util::toFloating(cmp_coords[i].lon)), 0.0001);
    }
}

BOOST_AUTO_TEST_SUITE_END()
