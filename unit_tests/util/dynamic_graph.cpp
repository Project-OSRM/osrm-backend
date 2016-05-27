#include "util/dynamic_graph.hpp"
#include "util/typedefs.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <vector>

BOOST_AUTO_TEST_SUITE(dynamic_graph)

using namespace osrm;
using namespace osrm::util;

struct TestData
{
    EdgeID id;
};

typedef DynamicGraph<TestData> TestDynamicGraph;
typedef TestDynamicGraph::InputEdge TestInputEdge;

BOOST_AUTO_TEST_CASE(find_test)
{
    /*
     *  (0) -1-> (1)
     *  ^ ^
     *  2 5
     *  | |
     *  (3) -3-> (4)
     *      <-4-
     */
    std::vector<TestInputEdge> input_edges = {TestInputEdge{0, 1, TestData{1}},
                                              TestInputEdge{3, 0, TestData{2}},
                                              TestInputEdge{3, 0, TestData{5}},
                                              TestInputEdge{3, 4, TestData{3}},
                                              TestInputEdge{4, 3, TestData{4}}};
    TestDynamicGraph simple_graph(5, input_edges);

    auto eit = simple_graph.FindEdge(0, 1);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 1);

    eit = simple_graph.FindEdge(1, 0);
    BOOST_CHECK_EQUAL(eit, SPECIAL_EDGEID);

    eit = simple_graph.FindEdgeInEitherDirection(1, 0);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 1);

    bool reverse = false;
    eit = simple_graph.FindEdgeIndicateIfReverse(1, 0, reverse);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 1);
    BOOST_CHECK(reverse);

    eit = simple_graph.FindEdge(3, 1);
    BOOST_CHECK_EQUAL(eit, SPECIAL_EDGEID);
    eit = simple_graph.FindEdge(0, 4);
    BOOST_CHECK_EQUAL(eit, SPECIAL_EDGEID);

    eit = simple_graph.FindEdge(3, 4);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 3);
    eit = simple_graph.FindEdgeInEitherDirection(3, 4);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 3);

    eit = simple_graph.FindEdge(3, 0);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 2);
}

BOOST_AUTO_TEST_SUITE_END()
