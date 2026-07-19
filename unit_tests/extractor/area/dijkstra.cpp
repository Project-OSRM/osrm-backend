#include "extractor/area/dijkstra.hpp"

#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <osmium/osm/node_ref.hpp>

#include "util/log.hpp"

BOOST_AUTO_TEST_SUITE(area_util_test)

using namespace osrm;
using namespace osrm::extractor::area;

BOOST_AUTO_TEST_CASE(area_dijkstra_test)
{
#define CHECK_EQUAL_RANGES(a, b)                                                                   \
    BOOST_CHECK_EQUAL_COLLECTIONS((a).begin(), (a).end(), (b).begin(), (b).end())

    // osrm::util::LogPolicy::GetInstance().SetLevel(logDEBUG);
    // osrm::util::LogPolicy::GetInstance().Unmute();

    auto dist = [](const osmium::NodeRef &a, const osmium::NodeRef &b)
    {
        auto ax = a.location().lon();
        auto ay = a.location().lat();
        auto bx = b.location().lon();
        auto by = b.location().lat();
        auto dx = bx - ax;
        auto dy = by - ay;
        return sqrt((dx * dx) + (dy * dy));
    };

    osmium::NodeRef u{0, {0, 0}};
    osmium::NodeRef v{1, {0, 1}};
    osmium::NodeRef w{2, {1, 1}};
    osmium::NodeRef x{3, {2, 1}};
    osmium::NodeRef y{4, {3, 1}};
    osmium::NodeRef z{5, {4, 1}};

    Dijkstra<osmium::NodeRef> d;
    auto add = [&](const osmium::NodeRef &a, const osmium::NodeRef &b)
    { d.add_edge(a, b, dist(a, b)); };

    add(u, v);
    add(v, w);
    add(w, x);
    add(x, y);
    add(y, z);
    BOOST_CHECK(d.num_edges() == 5);
    BOOST_CHECK(d.num_vertices() == 6);

    std::vector<size_t> expected{0, 0, 1, 2, 3, 4};
    BOOST_CHECK_NO_THROW(d.run(d.index_of(u)));
    CHECK_EQUAL_RANGES(d.get_predecessors(), expected);

    add(u, w);
    BOOST_CHECK(d.num_edges() == 6);
    BOOST_CHECK(d.num_vertices() == 6);

    expected = {0, 0, 0, 2, 3, 4};
    BOOST_CHECK_NO_THROW(d.run(d.index_of(u)));
    CHECK_EQUAL_RANGES(d.get_predecessors(), expected);

    add(u, x);
    add(u, y);
    add(u, z);
    BOOST_CHECK(d.num_edges() == 9);
    BOOST_CHECK(d.num_vertices() == 6);

    expected = {0, 0, 0, 0, 0, 0};
    BOOST_CHECK_NO_THROW(d.run(d.index_of(u)));
    CHECK_EQUAL_RANGES(d.get_predecessors(), expected);
}

BOOST_AUTO_TEST_CASE(area_dijkstra_stable_tie_break_test)
{
    auto build = [](bool z_first)
    {
        Dijkstra<int> d;
        d.add_edge(0, 1, 1.0); // g-a
        d.add_edge(5, 6, 1.0); // c-i
        d.add_edge(7, 8, 1.0); // d-j

        // Equal-cost alternatives from a to c/d.
        if (z_first)
        {
            d.add_edge(1, 3, 1.0); // a-z
            d.add_edge(1, 2, 1.0); // a-u
            d.add_edge(1, 4, 1.0); // a-y
        }
        else
        {
            d.add_edge(1, 2, 1.0); // a-u
            d.add_edge(1, 4, 1.0); // a-y
            d.add_edge(1, 3, 1.0); // a-z
        }

        d.add_edge(2, 5, 1.0); // u-c
        d.add_edge(3, 5, 1.0); // z-c
        d.add_edge(4, 7, 2.0); // y-d
        d.add_edge(3, 7, 2.0); // z-d

        return d;
    };

    auto d1 = build(false);
    auto d2 = build(true);

    BOOST_CHECK_NO_THROW(d1.run(d1.index_of(0)));
    BOOST_CHECK_NO_THROW(d2.run(d2.index_of(0)));

    const auto &p1 = d1.get_predecessors();
    const auto &p2 = d2.get_predecessors();

    // Stable predecessor selection must not depend on edge insertion order.
    BOOST_CHECK_EQUAL(p1[d1.index_of(5)], d1.index_of(2)); // c <- u
    BOOST_CHECK_EQUAL(p1[d1.index_of(7)], d1.index_of(3)); // d <- z
    BOOST_CHECK_EQUAL(p2[d2.index_of(5)], d2.index_of(2)); // c <- u
    BOOST_CHECK_EQUAL(p2[d2.index_of(7)], d2.index_of(3)); // d <- z
}

BOOST_AUTO_TEST_SUITE_END()
