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

BOOST_AUTO_TEST_SUITE_END()
