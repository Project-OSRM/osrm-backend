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

// Point type with floating-point coordinates. The real coordinate types fed to
// collinear() both store doubles: VisibilityGraph::Vertex (Mercator-projected
// meters) and osmium::NodeRef (geographic degrees).
using dpoint_t = boost::tuple<double, double>;

// Regression test for the collinear() tolerance (commits e7232f56c..209f53dcd).
//
// collinear() must work across two very different coordinate scales:
//
//   - Mercator meters (Vertex): edges ~tens of metres, resolution ~0.01 m.
//     The tolerance has to be loose enough that a triple that is collinear at
//     coordinate resolution — but whose area2 picks up a sub-resolution, non-zero
//     value from compiler-dependent floating point (e.g. GCC FMA contraction /
//     80-bit vs 64-bit) — is still reported collinear, so the sweep does not flip
//     behaviour between compilers.
//
//   - Geographic degrees (NodeRef): edges ~1e-3 deg, resolution ~1e-7 deg. Here
//     the tolerance has to stay tight enough that a genuine rectangular ring
//     corner is NOT swallowed as collinear (a too-loose tolerance would drop the
//     corner from obstacle detection).
//
// The fix replaced the previous tolerance with eps = max(1e-15, scale * 1e-9),
// which satisfies both.
BOOST_AUTO_TEST_CASE(area_util_test_collinear_tolerance)
{
    // --- Mercator-metre scale (Vertex) ---

    // A genuine right-angle corner (50 m x 50 m) is not collinear.
    {
        dpoint_t a(0.0, 0.0);
        dpoint_t b(50.0, 0.0);
        dpoint_t c(50.0, 50.0);
        BOOST_CHECK(!collinear(&a, &b, &c));
    }

    // Three points that are collinear at coordinate resolution but whose middle
    // point carries a 1 nm (sub-resolution) perturbation. This is a DETERMINISTIC
    // stand-in for the compiler-dependent floating-point noise the fix targets —
    // not a reproduction of real noise, which is platform-specific and would make
    // the test flaky (e.g. differing between macOS ARM and Intel). area2 here is
    // exactly -(100 * 1e-9) = -1e-7, a single correctly-rounded IEEE-754 multiply
    // that is bit-stable on every platform (with or without FMA). It lands
    // between the previous meter-scale tolerance (~1.1e-9, which reported "not
    // collinear") and the corrected one (~5e-6 for these ~50-100 m edges, which
    // absorbs it) — so the fix flips this case to collinear on all platforms.
    {
        dpoint_t a(0.0, 0.0);
        dpoint_t b(50.0, 1e-9);
        dpoint_t c(100.0, 0.0);
        BOOST_CHECK(collinear(&a, &b, &c));
    }

    // --- Geographic-degree scale (NodeRef) ---

    // A genuine rectangular building corner (~34 m x ~33 m expressed in degrees)
    // must NOT be treated as collinear, otherwise it would be excluded from
    // obstacle detection. area2 ~ 1.5e-7 is far above the degree-scale
    // tolerance (~1e-15).
    {
        dpoint_t a(13.4000000, 52.5000000);
        dpoint_t b(13.4005000, 52.5000000);
        dpoint_t c(13.4005000, 52.5003000);
        BOOST_CHECK(!collinear(&a, &b, &c));
    }

    // Points that are genuinely collinear at degree scale (a straight diagonal)
    // stay collinear: area2 is only a few 1e-18 of floating-point residual,
    // comfortably within the degree-scale tolerance.
    {
        dpoint_t a(13.400, 52.500);
        dpoint_t b(13.401, 52.501);
        dpoint_t c(13.402, 52.502);
        BOOST_CHECK(collinear(&a, &b, &c));
    }
}

BOOST_AUTO_TEST_SUITE_END()
