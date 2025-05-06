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

    osrm::util::LogPolicy::GetInstance().SetLevel(logDEBUG);
    osrm::util::LogPolicy::GetInstance().Unmute();

    osmium::NodeRef u{0, {0, 0}};
    osmium::NodeRef v{1, {1, 0}};
    osmium::NodeRef w{2, {1, 1}};
    osmium::NodeRef x{3, {0, 1}};

    DijkstraImpl<osmium::NodeRef> d;

    d.add_edge(u, v, 1.0);
    d.add_edge(u, w, 1.0);
    BOOST_CHECK(d.num_edges() == 2);
    BOOST_CHECK(d.num_vertices() == 3);

    BOOST_CHECK_NO_THROW(d.run(d.index_of(u)));

    std::vector<size_t> expected{0, 0, 0};
    CHECK_EQUAL_RANGES(d.get_predecessors(), expected);

    d.add_edge(v, x, 1.0);
    d.add_edge(w, x, 1.1);
    BOOST_CHECK(d.num_edges() == 4);
    BOOST_CHECK(d.num_vertices() == 4);

    BOOST_CHECK_NO_THROW(d.run(d.index_of(u)));

    expected = {0, 0, 0, 1};
    CHECK_EQUAL_RANGES(d.get_predecessors(), expected);
}

BOOST_AUTO_TEST_SUITE_END()
