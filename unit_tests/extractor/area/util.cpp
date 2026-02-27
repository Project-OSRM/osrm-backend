#include "extractor/area/util.hpp"

#include "extractor/area/typedefs.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <boost/geometry/geometries/point.hpp>

#include <boost/test/unit_test.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(bg::cs::cartesian)

BOOST_AUTO_TEST_SUITE(area_util_test)

using namespace osrm;
using namespace osrm::extractor::area;

using point_t = boost::tuple<int, int>;

BOOST_AUTO_TEST_CASE(area_util_test_geometry)
{
    point_t o(0, 0);
    point_t x(10, 0);
    point_t y(0, 10);
    point_t d(10, 10);
    point_t dd(20, 20);

    BOOST_CHECK(left(&o, &x, &d));
    BOOST_CHECK(left(&o, &d, &y));
    BOOST_CHECK(left(&o, &x, &y));

    BOOST_CHECK(!left(&o, &y, &d));
    BOOST_CHECK(!left(&o, &d, &x));
    BOOST_CHECK(!left(&o, &y, &x));

    BOOST_CHECK(collinear(&o, &d, &dd));

    // some degenerate cases
    BOOST_CHECK(!left(&o, &d, &d));
    BOOST_CHECK(!left(&o, &o, &o));

    // intersections
    point_t expected(5, 5);
    point_t i(0, 0);
    BOOST_CHECK(intersect(&o, &d, &x, &y, &i));
    BOOST_CHECK(bg::equals(i, expected));

    BOOST_CHECK(!intersect(&dd, &d, &x, &y, &i));
    BOOST_CHECK(intersect(&dd, &d, &x, &y, &i, true));
    BOOST_CHECK(bg::equals(i, expected));

    BOOST_CHECK(!intersect(&o, &x, &x, &d));
    BOOST_CHECK(!intersect(&o, &y, &y, &d));
    BOOST_CHECK(!intersect(&o, &d, &d, &x));
    BOOST_CHECK(!intersect(&o, &d, &d, &y));

    // in closed cone clockwise
    BOOST_CHECK(in_closed_cone(&y, &o, &x, &y));
    BOOST_CHECK(in_closed_cone(&y, &o, &x, &d));
    BOOST_CHECK(in_closed_cone(&y, &o, &x, &x));

    BOOST_CHECK(!in_closed_cone(&x, &o, &y, &d));
}

BOOST_AUTO_TEST_SUITE_END()
