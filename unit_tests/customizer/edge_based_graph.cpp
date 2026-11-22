#include "customizer/edge_based_graph.hpp"

#include "util/typedefs.hpp"

#include <boost/test/unit_test.hpp>

#include <vector>

BOOST_AUTO_TEST_SUITE(directed_edge_lookup)

BOOST_AUTO_TEST_CASE(reverse_search_arcs_do_not_hide_forward_turns)
{
    using Graph = osrm::customizer::MultiLevelEdgeBasedGraph;
    const Graph graph{std::vector<Graph::NodeArrayEntry>{{0}, {2}, {4}},
                      std::vector<Graph::EdgeArrayEntry>{{1, {9}}, {1, {7}}, {0, {7}}, {0, {9}}},
                      std::vector<Graph::EdgeOffset>{0, 0, 1},
                      std::vector<EdgeWeight>{{1}, {1}},
                      std::vector<EdgeDuration>{{1}, {1}},
                      std::vector<EdgeDistance>{{1}, {1}},
                      std::vector<bool>{false, true, false, true},
                      std::vector<bool>{true, false, true, false}};

    const auto forward = graph.FindEdge(0, 1);
    BOOST_REQUIRE(forward != SPECIAL_EDGEID);
    BOOST_CHECK(graph.IsForwardEdge(forward));
    BOOST_CHECK_EQUAL(graph.GetEdgeData(forward).turn_id, 7);

    const auto reverse = graph.FindEdge(1, 0);
    BOOST_REQUIRE(reverse != SPECIAL_EDGEID);
    BOOST_CHECK(graph.IsForwardEdge(reverse));
    BOOST_CHECK_EQUAL(graph.GetEdgeData(reverse).turn_id, 9);
    BOOST_CHECK_EQUAL(graph.FindEdge(0, 0), SPECIAL_EDGEID);
}

BOOST_AUTO_TEST_CASE(reverse_only_arcs_are_not_outgoing_turns)
{
    using Graph = osrm::customizer::MultiLevelEdgeBasedGraph;
    const Graph graph{std::vector<Graph::NodeArrayEntry>{{0}, {1}, {2}},
                      std::vector<Graph::EdgeArrayEntry>{{1, {7}}, {0, {7}}},
                      std::vector<Graph::EdgeOffset>{0, 0, 1},
                      std::vector<EdgeWeight>{{1}, {1}},
                      std::vector<EdgeDuration>{{1}, {1}},
                      std::vector<EdgeDistance>{{1}, {1}},
                      std::vector<bool>{false, true},
                      std::vector<bool>{true, false}};

    BOOST_CHECK_EQUAL(graph.FindEdge(0, 1), SPECIAL_EDGEID);
    BOOST_CHECK_EQUAL(graph.FindEdge(1, 0), 1);
}

BOOST_AUTO_TEST_SUITE_END()
