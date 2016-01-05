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
    const std::string polyline = "_gjaR_gjaR_pR_ibE_pR_ibE_pR_ibE_pR_ibE";
    PolylineCompressor pc;
    std::vector<util::FixedPointCoordinate> coords = pc.decode_string(polyline);

    // Test coordinates; these would be the coordinates we give the loc parameter,
    // e.g. loc=10.00,10.0&loc=10.01,10.1...
    util::FixedPointCoordinate coord1(10.00 * COORDINATE_PRECISION, 10.0 * COORDINATE_PRECISION);
    util::FixedPointCoordinate coord2(10.01 * COORDINATE_PRECISION, 10.1 * COORDINATE_PRECISION);
    util::FixedPointCoordinate coord3(10.02 * COORDINATE_PRECISION, 10.2 * COORDINATE_PRECISION);
    util::FixedPointCoordinate coord4(10.03 * COORDINATE_PRECISION, 10.3 * COORDINATE_PRECISION);
    util::FixedPointCoordinate coord5(10.04 * COORDINATE_PRECISION, 10.4 * COORDINATE_PRECISION);

    // Put the test coordinates into the vector for comparison
    std::vector<util::FixedPointCoordinate> cmp_coords;
    cmp_coords.emplace_back(coord1);
    cmp_coords.emplace_back(coord2);
    cmp_coords.emplace_back(coord3);
    cmp_coords.emplace_back(coord4);
    cmp_coords.emplace_back(coord5);

    BOOST_CHECK_EQUAL(cmp_coords.size(), coords.size());

    for (unsigned i = 0; i < cmp_coords.size(); ++i)
    {
        const double cmp1_lat = coords.at(i).lat;
        const double cmp2_lat = cmp_coords.at(i).lat;
        BOOST_CHECK_CLOSE(cmp1_lat, cmp2_lat, 0.0001);

        const double cmp1_lon = coords.at(i).lon;
        const double cmp2_lon = cmp_coords.at(i).lon;
        BOOST_CHECK_CLOSE(cmp1_lon, cmp2_lon, 0.0001);
    }
}

BOOST_AUTO_TEST_SUITE_END()
