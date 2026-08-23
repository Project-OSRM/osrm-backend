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

BOOST_AUTO_TEST_CASE(area_dijkstra_equal_distance_tie_break_test)
{
    Dijkstra<unsigned> d;
    d.add_edge(0, 2, 1.);
    d.add_edge(0, 1, 1.);
    d.add_edge(1, 3, 1.);
    d.add_edge(2, 3, 1.);

    std::vector<size_t> expected{0, 0, 0, 2};
    BOOST_CHECK_NO_THROW(d.run(d.index_of(0)));
    CHECK_EQUAL_RANGES(d.get_predecessors(), expected);
}

// Following the predecessors from anywhere reaches the source.
//
// This is what every caller of get_predecessors() does, and none of them can check it
// first: recovering a path means walking the array until the root turns up. If two
// entries point at each other the walk never ends, and in an extractor that means the
// whole run never finishes, with no output and no error to look at.
//
// The shape that breaks it needs an edge of no length between two vertices the same
// distance from the source, which sounds contrived and is not. Plaza rings routinely
// carry two nodes at the same location, and a ring edge between them weighs zero.
//
// With one, both vertices settle at the same distance, and the tie-break that prefers
// the lower-numbered predecessor then fires in both directions: each is a better
// predecessor for the other than the source is, so each ends up pointing at the other.
BOOST_AUTO_TEST_CASE(area_dijkstra_leaves_no_cycle_in_the_predecessors)
{
    // Vertex numbers deliberately out of step with insertion order, because the
    // tie-break compares the vertices and the walk uses their indices.
    Dijkstra<unsigned> d;
    d.add_edge(10, 3, 1.0); // the source, and one arm
    d.add_edge(10, 2, 1.0); // the other arm, exactly as long
    d.add_edge(3, 2, 0.0);  // two nodes mapped at the same place

    const auto source = d.index_of(10);
    BOOST_CHECK_NO_THROW(d.run(source));

    const auto &predecessors = d.get_predecessors();
    for (std::size_t start = 0; start < d.num_vertices(); ++start)
    {
        std::size_t v = start;
        std::size_t steps = 0;
        while (v != source && v != predecessors.at(v))
        {
            v = predecessors.at(v);
            BOOST_REQUIRE_MESSAGE(++steps <= d.num_vertices(),
                                  "predecessors cycle, walking from vertex "
                                      << d.get_vertex(start));
        }
    }

    // Both arms are still a distance of one away, and the tie is still broken the same
    // way every run: vertex 2 is reached through vertex 3, because going that way costs
    // the same and 3 is the lower predecessor. Only the loop is gone, not the choice.
    BOOST_CHECK_CLOSE(d.get_distances().at(d.index_of(3)), 1.0, 1e-9);
    BOOST_CHECK_CLOSE(d.get_distances().at(d.index_of(2)), 1.0, 1e-9);
    BOOST_CHECK_EQUAL(predecessors.at(d.index_of(3)), source);
    BOOST_CHECK_EQUAL(predecessors.at(d.index_of(2)), d.index_of(3));
}

BOOST_AUTO_TEST_SUITE_END()
