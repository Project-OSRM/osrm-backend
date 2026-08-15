#include "extractor/area/util.hpp"

#include "extractor/area/typedefs.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <boost/geometry/geometries/point.hpp>

#include <boost/test/unit_test.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(boost::geometry::cs::cartesian)

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
    BOOST_CHECK(boost::geometry::equals(i, expected));

    BOOST_CHECK(!intersect(&dd, &d, &x, &y, &i));
    BOOST_CHECK(intersect(&dd, &d, &x, &y, &i, true));
    BOOST_CHECK(boost::geometry::equals(i, expected));

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

// A segment that ends on, or passes through, the endpoint of another does not cross it.
//
// intersect() already means to say this: with std::less_equal it rejects the parameters at
// 0 and 1, so a touch is not a crossing. It says it by dividing, and division does not land
// on 1 exactly. The coordinates below are from a plaza in the fuzz corpus, at the scale the
// engine actually stores, and there the touch computes to 0.9999999999998549, a hundredth
// of a nanometre short of the endpoint and on the wrong side of the test.
//
// The cost of getting this wrong is a sight line deleted from a visibility graph. Walking
// along a wall is allowed, so a line that grazes an obstacle's corner and continues along
// its face is real, and dropping it makes the planner go the long way round. Two other
// places in this codebase already work around the same weakness by hand.
BOOST_AUTO_TEST_CASE(area_util_a_touch_at_an_endpoint_is_not_a_crossing)
{
    using point_t = boost::geometry::model::d2::point_xy<double>;

    // Vertical, running up to the top right corner of a rectangular obstacle.
    const point_t from(1.003304, 1.0009790000000001);
    const point_t to(1.003304, 1.0028539999999999);
    // The obstacle's bottom edge. Its right endpoint lies exactly on the segment above.
    const point_t edge_left(1.002507, 1.002066);
    const point_t edge_right(1.003304, 1.002066);

    BOOST_CHECK(!intersect(&from, &to, &edge_left, &edge_right));
    // And the same the other way round, since which segment is named first is arbitrary.
    BOOST_CHECK(!intersect(&edge_left, &edge_right, &from, &to));

    // The obstacle's right edge, which the segment runs along and ends on. Collinear, so
    // it was already rejected as parallel, but it is the other half of the same picture.
    const point_t edge_top(1.003304, 1.002854);
    BOOST_CHECK(!intersect(&from, &to, &edge_right, &edge_top));

    // Unit-scale versions of the same shapes, so the property is stated independently of
    // the coordinates that happened to expose it.
    const point_t a(0, 0), b(0, 10), c(-5, 5), e(0, 5);
    BOOST_CHECK(!intersect(&a, &b, &c, &e)); // e is the endpoint, sitting on a..b
    BOOST_CHECK(!intersect(&c, &e, &a, &b));

    // A real crossing still crosses.
    const point_t g(-5, 5), h(5, 5);
    BOOST_CHECK(intersect(&a, &b, &g, &h));
}

// The same property over a grid of lon/lat sized coordinates, where the rounding bites.
//
// The single case above passes with or without the fix on some compilers, because whether
// the quotient lands on 1 depends on multiply-add being contracted into fma. That makes it
// a poor guard: it would go on passing while the predicate was wrong everywhere else. Here
// a few hundred touches are built at the scale the engine stores, near 1.0 with differences
// around 1e-6, which is where the cancellation is worst and where the answer stops being
// reproducible unless the degenerate case is decided exactly.
//
// Every configuration is a T: a vertical segment, and a horizontal one whose endpoint lies
// exactly on it. None of them is a crossing.
BOOST_AUTO_TEST_CASE(area_util_touching_is_never_a_crossing_at_lonlat_scale)
{
    using point_t = boost::geometry::model::d2::point_xy<double>;

    constexpr double ORIGIN = 1.0;
    constexpr double GRID = 1e-6; // the coordinate cell the engine stores
    std::size_t checked = 0, wrong = 0;

    for (int column = 1; column <= 40; ++column)
    {
        for (int low = 1; low <= 12; ++low)
        {
            for (int high = low + 2; high <= low + 14; high += 3)
            {
                const auto x = ORIGIN + column * 797 * GRID;
                const auto y0 = ORIGIN + low * 1087 * GRID;
                const auto y1 = ORIGIN + high * 1087 * GRID;
                // Strictly between the two, so the touch is in the vertical segment's
                // interior and cannot be dismissed as a shared endpoint.
                const auto y = ORIGIN + ((low + high) / 2) * 1087 * GRID;

                const point_t top(x, y1), bottom(x, y0);
                const point_t left(x - 613 * GRID, y), touch(x, y);

                ++checked;
                // The horizontal segment ends on the vertical one: a touch, not a crossing.
                if (intersect(&bottom, &top, &left, &touch))
                    ++wrong;
                // And with the arguments the other way round.
                if (intersect(&left, &touch, &bottom, &top))
                    ++wrong;
            }
        }
    }

    BOOST_TEST_MESSAGE("touching configurations checked: " << checked
                                                           << ", reported as crossings: " << wrong);
    BOOST_CHECK_GT(checked, 500u);
    BOOST_CHECK_EQUAL(wrong, 0u);
}

BOOST_AUTO_TEST_SUITE_END()
