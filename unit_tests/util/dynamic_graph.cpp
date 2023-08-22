#include "util/dynamic_graph.hpp"
#include "util/typedefs.hpp"

#include "../common/range_tools.hpp"

#include <boost/test/unit_test.hpp>

#include <vector>

BOOST_AUTO_TEST_SUITE(dynamic_graph)

using namespace osrm;
using namespace osrm::util;

struct TestData
{
    EdgeID id;
};

using TestDynamicGraph = DynamicGraph<TestData>;
using TestInputEdge = TestDynamicGraph::InputEdge;

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

BOOST_AUTO_TEST_CASE(renumber_test)
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

    /*
     *  (1) -1-> (3)
     *  ^ ^
     *  2 5
     *  | |
     *  (0) -3-> (2)
     *      <-4-
     */
    simple_graph.Renumber({1, 3, 4, 0, 2});

    auto eit = simple_graph.FindEdge(1, 3);
    BOOST_CHECK(eit != SPECIAL_EDGEID);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 1);

    eit = simple_graph.FindEdge(3, 1);
    BOOST_CHECK_EQUAL(eit, SPECIAL_EDGEID);

    eit = simple_graph.FindEdgeInEitherDirection(3, 1);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 1);

    bool reverse = false;
    eit = simple_graph.FindEdgeIndicateIfReverse(3, 1, reverse);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 1);
    BOOST_CHECK(reverse);

    eit = simple_graph.FindEdge(0, 3);
    BOOST_CHECK_EQUAL(eit, SPECIAL_EDGEID);
    eit = simple_graph.FindEdge(1, 2);
    BOOST_CHECK_EQUAL(eit, SPECIAL_EDGEID);

    eit = simple_graph.FindEdge(0, 2);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 3);
    eit = simple_graph.FindEdgeInEitherDirection(0, 2);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 3);

    eit = simple_graph.FindEdge(0, 1);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 2);
}

BOOST_AUTO_TEST_CASE(filter_test)
{
    /*
     *  (0) -1-> (1)
     *  ^ ^       ^
     *  2 5       1
     *  | |       |
     *  (3) -3-> (4)
     *      <-4-
     */
    std::vector<TestInputEdge> input_edges = {TestInputEdge{0, 1, TestData{1}},
                                              TestInputEdge{3, 0, TestData{2}},
                                              TestInputEdge{3, 0, TestData{5}},
                                              TestInputEdge{3, 4, TestData{3}},
                                              TestInputEdge{4, 1, TestData{1}},
                                              TestInputEdge{4, 3, TestData{4}}};
    TestDynamicGraph simple_graph(5, input_edges);

    // only keep 0, 1, 4 -> filter out all edges from/to 3
    auto filtered_simple_graph =
        simple_graph.Filter([](const NodeID node) { return node == 0 || node == 1 || node == 4; });

    REQUIRE_SIZE_RANGE(filtered_simple_graph.GetAdjacentEdgeRange(0), 1);
    CHECK_EQUAL_RANGE(filtered_simple_graph.GetAdjacentEdgeRange(0),
                      filtered_simple_graph.FindEdge(0, 1));

    REQUIRE_SIZE_RANGE(filtered_simple_graph.GetAdjacentEdgeRange(1), 0);

    REQUIRE_SIZE_RANGE(filtered_simple_graph.GetAdjacentEdgeRange(2), 0);

    REQUIRE_SIZE_RANGE(filtered_simple_graph.GetAdjacentEdgeRange(3), 0);

    REQUIRE_SIZE_RANGE(filtered_simple_graph.GetAdjacentEdgeRange(4), 1);
    CHECK_EQUAL_RANGE(filtered_simple_graph.GetAdjacentEdgeRange(4),
                      filtered_simple_graph.FindEdge(4, 1));
}

BOOST_AUTO_TEST_SUITE_END()
