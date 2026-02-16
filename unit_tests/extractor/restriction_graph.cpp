#include "extractor/restriction_graph.hpp"
#include "extractor/maneuver_override.hpp"
#include "extractor/restriction.hpp"
#include "util/typedefs.hpp"

#include <boost/test/unit_test.hpp>
#include <iostream>

#include <vector>

BOOST_AUTO_TEST_SUITE(restriction_graph)

using namespace osrm;
using namespace osrm::extractor;

TurnRestriction makeWayRestriction(NodeID from, std::vector<NodeID> via, NodeID to, bool is_only)
{
    ViaWayPath vwp{from, std::move(via), to};
    return TurnRestriction({vwp}, is_only);
}

TurnRestriction makeNodeRestriction(NodeID from, NodeID via, NodeID to, bool is_only)
{
    ViaNodePath vnp{from, via, to};
    return TurnRestriction({vnp}, is_only);
}

struct instruction
{
    NodeID to;
    bool is_only;
};

std::ostream &operator<<(std::ostream &os, const instruction &in)
{
    os << std::boolalpha << in.to << ':' << in.is_only;
    return os;
}

bool operator==(const instruction &lhs, const instruction &rhs) noexcept
{
    return lhs.to == rhs.to && lhs.is_only == rhs.is_only;
}

bool operator!=(const instruction &lhs, const instruction &rhs) noexcept { return !(lhs == rhs); }

void checkInstructions(RestrictionGraph::RestrictionRange restrictions,
                       std::vector<instruction> expected_instructions)
{
    std::vector<instruction> actual_instructions;
    std::transform(restrictions.begin(),
                   restrictions.end(),
                   std::back_inserter(actual_instructions),
                   [](const auto &restriction) {
                       return instruction{restriction->turn_path.To(), bool(restriction->is_only)};
                   });
    std::sort(actual_instructions.begin(),
              actual_instructions.end(),
              [](const auto &lhs, const auto &rhs)
              { return (lhs.to < rhs.to) || (lhs.to == rhs.to && lhs.is_only); });
    std::sort(expected_instructions.begin(),
              expected_instructions.end(),
              [](const auto &lhs, const auto &rhs)
              { return (lhs.to < rhs.to) || (lhs.to == rhs.to && lhs.is_only); });

    BOOST_REQUIRE_EQUAL_COLLECTIONS(actual_instructions.begin(),
                                    actual_instructions.end(),
                                    expected_instructions.begin(),
                                    expected_instructions.end());
}

void checkEdges(RestrictionGraph::EdgeRange edges, std::vector<NodeID> expected_edges)
{
    std::vector<NodeID> actual_edges;
    std::transform(edges.begin(),
                   edges.end(),
                   std::back_inserter(actual_edges),
                   [&](const auto &edge) { return edge.node_based_to; });
    std::sort(actual_edges.begin(), actual_edges.end(), std::less<NodeID>());
    std::sort(expected_edges.begin(), expected_edges.end(), std::less<NodeID>());

    BOOST_REQUIRE_EQUAL_COLLECTIONS(
        actual_edges.begin(), actual_edges.end(), expected_edges.begin(), expected_edges.end());
}

std::map<NodeID, size_t> nextEdges(RestrictionGraph::EdgeRange edges)
{
    std::map<NodeID, size_t> res;
    std::transform(edges.begin(),
                   edges.end(),
                   std::inserter(res, res.end()),
                   [&](const auto &edge)
                   { return std::make_pair(edge.node_based_to, edge.target); });
    return res;
}

std::map<NodeID, size_t> checkNode(const RestrictionGraph &graph,
                                   const RestrictionID node_id,
                                   std::vector<instruction> expected_instructions,
                                   std::vector<NodeID> expected_edges)
{
    checkInstructions(graph.GetRestrictions(node_id), std::move(expected_instructions));
    checkEdges(graph.GetEdges(node_id), std::move(expected_edges));
    return nextEdges(graph.GetEdges(node_id));
}

std::map<NodeID, size_t>
validateStartRestrictionNode(RestrictionGraph &graph,
                             NodeID from,
                             NodeID to,
                             std::vector<instruction> restriction_instructions,
                             std::vector<NodeID> restriction_edges)
{
    BOOST_REQUIRE_EQUAL(graph.start_edge_to_node.count({from, to}), 1);
    const auto node_id = graph.start_edge_to_node[{from, to}];
    BOOST_REQUIRE_GE(node_id, graph.num_via_nodes);
    BOOST_REQUIRE_LT(node_id, graph.nodes.size());

    return checkNode(
        graph, node_id, std::move(restriction_instructions), std::move(restriction_edges));
}

std::map<NodeID, size_t>
validateViaRestrictionNode(RestrictionGraph &graph,
                           size_t via_node_idx,
                           NodeID from,
                           NodeID to,
                           std::vector<instruction> restriction_instructions,
                           std::vector<NodeID> restriction_edges)
{
    BOOST_REQUIRE_GE(graph.via_edge_to_node.count({from, to}), 1);
    auto node_match_it = graph.via_edge_to_node.equal_range({from, to});

    BOOST_REQUIRE_MESSAGE(std::any_of(node_match_it.first,
                                      node_match_it.second,
                                      [&](const auto node_match)
                                      { return node_match.second == via_node_idx; }),
                          "Could not find expected via_node_idx "
                              << via_node_idx << " for graph edge " << from << "," << to);

    BOOST_REQUIRE_LT(via_node_idx, graph.num_via_nodes);
    return checkNode(
        graph, via_node_idx, std::move(restriction_instructions), std::move(restriction_edges));
}

BOOST_AUTO_TEST_CASE(empty_restrictions)
{
    //
    // Input
    //
    // Output
    //
    std::vector<TurnRestriction> empty;

    auto graph = constructRestrictionGraph(empty);

    BOOST_CHECK(graph.nodes.empty());
    BOOST_CHECK_EQUAL(graph.num_via_nodes, 0);
    BOOST_CHECK(graph.edges.empty());
    BOOST_CHECK(graph.start_edge_to_node.empty());
    BOOST_CHECK(graph.via_edge_to_node.empty());
}

BOOST_AUTO_TEST_CASE(node_restriction)
{
    //
    // Input
    // 0 -> 1: only_2
    //
    // Output
    // s(0,1,[only_2])
    //
    std::vector<TurnRestriction> input{
        makeNodeRestriction(0, 1, 2, true),
    };

    std::cout << "Construct graph" << std::endl;
    auto graph = constructRestrictionGraph(input);
    std::cout << "Constructed graph" << std::endl;

    BOOST_CHECK_EQUAL(graph.nodes.size(), 1);
    BOOST_CHECK_EQUAL(graph.num_via_nodes, 0);
    BOOST_CHECK_EQUAL(graph.start_edge_to_node.size(), 1);
    BOOST_CHECK_EQUAL(graph.via_edge_to_node.size(), 0);
    BOOST_CHECK_EQUAL(graph.edges.size(), 0);

    validateStartRestrictionNode(graph, 0, 1, {{2, true}}, {});
}

BOOST_AUTO_TEST_CASE(way_restriction)
{
    //
    // Input
    // 0 -> 1 -> 2 -> 3: only_4
    //
    // Output
    // s(0,1) -> v(1,2) -> v(2,3,[only_4])
    //
    std::vector<TurnRestriction> input{
        makeWayRestriction(0,
                           {
                               1,
                               2,
                               3,
                           },
                           4,
                           true),
    };

    auto graph = constructRestrictionGraph(input);

    BOOST_CHECK_EQUAL(graph.nodes.size(), 3);
    BOOST_CHECK_EQUAL(graph.num_via_nodes, 2);
    BOOST_CHECK_EQUAL(graph.start_edge_to_node.size(), 1);
    BOOST_CHECK_EQUAL(graph.via_edge_to_node.size(), 2);
    BOOST_CHECK_EQUAL(graph.edges.size(), 2);

    auto start_edges = validateStartRestrictionNode(graph, 0, 1, {}, {2});
    auto via_edges = validateViaRestrictionNode(graph, start_edges[2], 1, 2, {}, {3});
    validateViaRestrictionNode(graph, via_edges[3], 2, 3, {{4, true}}, {});
}

BOOST_AUTO_TEST_CASE(disconnected_restrictions)
{
    //
    // Input
    // 0 -> 1 -> 2 -> 3: only_4
    // 5 -> 6 -> 7 -> 8: only_9
    //
    // Output
    // s(0,1) -> v(1,2) -> v(2,3,[only_4])
    // s(5,6) -> v(6,7) -> v(7,8,[only_9])
    //
    std::vector<TurnRestriction> input{
        makeWayRestriction(0,
                           {
                               1,
                               2,
                               3,
                           },
                           4,
                           true),
        makeWayRestriction(5,
                           {
                               6,
                               7,
                               8,
                           },
                           9,
                           true),
    };

    auto graph = constructRestrictionGraph(input);

    BOOST_CHECK_EQUAL(graph.nodes.size(), 6);
    BOOST_CHECK_EQUAL(graph.num_via_nodes, 4);
    BOOST_CHECK_EQUAL(graph.edges.size(), 4);
    BOOST_CHECK_EQUAL(graph.start_edge_to_node.size(), 2);
    BOOST_CHECK_EQUAL(graph.via_edge_to_node.size(), 4);

    auto start_edges_1 = validateStartRestrictionNode(graph, 0, 1, {}, {2});
    auto start_edges_2 = validateStartRestrictionNode(graph, 5, 6, {}, {7});

    auto via_edges_1 = validateViaRestrictionNode(graph, start_edges_1[2], 1, 2, {}, {3});
    validateViaRestrictionNode(graph, via_edges_1[3], 2, 3, {{4, true}}, {});
    auto via_edges_2 = validateViaRestrictionNode(graph, start_edges_2[7], 6, 7, {}, {8});
    validateViaRestrictionNode(graph, via_edges_2[8], 7, 8, {{9, true}}, {});
}

BOOST_AUTO_TEST_CASE(same_prefix_restrictions)
{
    //
    // Input
    // 0 -> 1 -> 2 -> 3: only_4
    // 0 -> 1 -> 2 -> 5: only_6
    //
    // Output
    // s(0,1) -> v(1,2) -> v(2,3,[only_4])
    //                  \_>v(2,5,[only_6])
    //
    std::vector<TurnRestriction> input{
        makeWayRestriction(0,
                           {
                               1,
                               2,
                               3,
                           },
                           4,
                           true),
        makeWayRestriction(0,
                           {
                               1,
                               2,
                               5,
                           },
                           6,
                           true),
    };

    auto graph = constructRestrictionGraph(input);

    BOOST_CHECK_EQUAL(graph.nodes.size(), 4);
    BOOST_CHECK_EQUAL(graph.num_via_nodes, 3);
    BOOST_CHECK_EQUAL(graph.edges.size(), 3);
    BOOST_CHECK_EQUAL(graph.start_edge_to_node.size(), 1);
    BOOST_CHECK_EQUAL(graph.via_edge_to_node.size(), 3);

    auto start_edges = validateStartRestrictionNode(graph, 0, 1, {}, {2});

    auto via_edges = validateViaRestrictionNode(graph, start_edges[2], 1, 2, {}, {3, 5});
    validateViaRestrictionNode(graph, via_edges[3], 2, 3, {{4, true}}, {});
    validateViaRestrictionNode(graph, via_edges[5], 2, 5, {{6, true}}, {});
}

BOOST_AUTO_TEST_CASE(duplicate_edges)
{
    //
    // Input
    // 0 -> 1 -> 2 -> 3: only_4
    // 5 -> 1 -> 2 -> 3: only_6
    //
    // Output
    // s(0,1) -> v(1,2) -> v(2,3,[only_4])
    // s(5,1) -> v(1,2) -> v(2,3,[only_6])
    //
    std::vector<TurnRestriction> input{
        makeWayRestriction(0,
                           {
                               1,
                               2,
                               3,
                           },
                           4,
                           true),
        makeWayRestriction(5,
                           {
                               1,
                               2,
                               3,
                           },
                           6,
                           true),
    };

    auto graph = constructRestrictionGraph(input);

    BOOST_CHECK_EQUAL(graph.nodes.size(), 6);
    BOOST_CHECK_EQUAL(graph.num_via_nodes, 4);
    BOOST_CHECK_EQUAL(graph.edges.size(), 4);
    BOOST_CHECK_EQUAL(graph.start_edge_to_node.size(), 2);
    BOOST_CHECK_EQUAL(graph.via_edge_to_node.size(), 4);
    BOOST_CHECK_EQUAL(graph.via_edge_to_node.count({1, 2}), 2);
    BOOST_CHECK_EQUAL(graph.via_edge_to_node.count({2, 3}), 2);

    auto start_edges_1 = validateStartRestrictionNode(graph, 0, 1, {}, {2});
    auto start_edges_2 = validateStartRestrictionNode(graph, 5, 1, {}, {2});

    auto via_edges_1 = validateViaRestrictionNode(graph, start_edges_1[2], 1, 2, {}, {3});
    validateViaRestrictionNode(graph, via_edges_1[3], 2, 3, {{4, true}}, {});

    auto via_edges_2 = validateViaRestrictionNode(graph, start_edges_2[2], 1, 2, {}, {3});
    validateViaRestrictionNode(graph, via_edges_2[3], 2, 3, {{6, true}}, {});
}

BOOST_AUTO_TEST_CASE(nested_restriction)
{
    //
    // Input
    // 0 -> 1 -> 2 -> 3: [only_4]
    // 1 -> 2: [only_4]
    //
    // Output
    // s(0,1) -> v(1,2,[only_4]) -> v(2,3,[only_4])
    // s(1,2,[only_4])
    //
    std::vector<TurnRestriction> input{
        makeWayRestriction(0,
                           {
                               1,
                               2,
                               3,
                           },
                           4,
                           true),
        makeNodeRestriction(1, 2, 4, true),
    };

    auto graph = constructRestrictionGraph(input);

    BOOST_CHECK_EQUAL(graph.nodes.size(), 4);
    BOOST_CHECK_EQUAL(graph.num_via_nodes, 2);
    BOOST_CHECK_EQUAL(graph.edges.size(), 2);
    BOOST_CHECK_EQUAL(graph.start_edge_to_node.size(), 2);
    BOOST_CHECK_EQUAL(graph.via_edge_to_node.size(), 2);

    auto start_edges_1 = validateStartRestrictionNode(graph, 0, 1, {}, {2});
    validateStartRestrictionNode(graph, 1, 2, {{4, true}}, {});

    auto via_edges_1 = validateViaRestrictionNode(graph, start_edges_1[2], 1, 2, {{4, true}}, {3});
    validateViaRestrictionNode(graph, via_edges_1[3], 2, 3, {{4, true}}, {});
}

BOOST_AUTO_TEST_CASE(partially_nested_restriction)
{
    //
    // Input
    // 0 -> 1 -> 2 -> 3: only_4
    // 2 -> 3 -> 4: only_5
    //
    // Output
    // s(0,1) -> v(1,2) -> v(2,3,[only_4])
    //                        |
    //                        t
    //                        |
    //           s(2,3) -> v(3,4,[only_5])
    //
    std::vector<TurnRestriction> input{
        makeWayRestriction(0,
                           {
                               1,
                               2,
                               3,
                           },
                           4,
                           true),
        makeWayRestriction(2, {3, 4}, 5, true),
    };

    auto graph = constructRestrictionGraph(input);

    BOOST_CHECK_EQUAL(graph.nodes.size(), 5);
    BOOST_CHECK_EQUAL(graph.num_via_nodes, 3);
    BOOST_CHECK_EQUAL(graph.edges.size(), 4);
    BOOST_CHECK_EQUAL(graph.start_edge_to_node.size(), 2);
    BOOST_CHECK_EQUAL(graph.via_edge_to_node.size(), 3);

    auto start_edges_1 = validateStartRestrictionNode(graph, 0, 1, {}, {2});
    auto start_edges_2 = validateStartRestrictionNode(graph, 2, 3, {}, {4});

    auto via_edges_1 = validateViaRestrictionNode(graph, start_edges_1[2], 1, 2, {}, {3});
    validateViaRestrictionNode(graph, via_edges_1[3], 2, 3, {{4, true}}, {4});
    validateViaRestrictionNode(graph, start_edges_2[4], 3, 4, {{5, true}}, {});
}

BOOST_AUTO_TEST_CASE(conflicting_nested_restriction)
{
    //
    // Input
    // 0 -> 1 -> 2 -> 3: no_4
    // 2 -> 3 -> 4: only_5
    //
    // Output
    // s(0,1) -> v(1,2) -> v(2,3,[no_4])
    // s(2,3) -> v(3,4,[only_5])
    //
    std::vector<TurnRestriction> input{
        makeWayRestriction(0,
                           {
                               1,
                               2,
                               3,
                           },
                           4,
                           false),
        makeWayRestriction(2, {3, 4}, 5, true),
    };

    auto graph = constructRestrictionGraph(input);

    BOOST_CHECK_EQUAL(graph.nodes.size(), 5);
    BOOST_CHECK_EQUAL(graph.num_via_nodes, 3);
    BOOST_CHECK_EQUAL(graph.edges.size(), 3);
    BOOST_CHECK_EQUAL(graph.start_edge_to_node.size(), 2);
    BOOST_CHECK_EQUAL(graph.via_edge_to_node.size(), 3);

    auto start_edges_1 = validateStartRestrictionNode(graph, 0, 1, {}, {2});
    auto start_edges_2 = validateStartRestrictionNode(graph, 2, 3, {}, {4});

    auto via_edges_1 = validateViaRestrictionNode(graph, start_edges_1[2], 1, 2, {}, {3});
    validateViaRestrictionNode(graph, via_edges_1[3], 2, 3, {{4, false}}, {});
    validateViaRestrictionNode(graph, start_edges_2[4], 3, 4, {{5, true}}, {});
}

BOOST_AUTO_TEST_CASE(restriction_edge_matches_start)
{
    //
    // Input
    // 0 -> 1 -> 2 -> 3: only_4
    // 3 -> 4 -> 5: only_6
    //
    // Output
    // s(0,1) -> v(1,2) -> v(2,3,[only_4])
    // s(3,4) -> v(4,5,[only_6])
    //
    std::vector<TurnRestriction> input{
        makeWayRestriction(0,
                           {
                               1,
                               2,
                               3,
                           },
                           4,
                           true),
        makeWayRestriction(3, {4, 5}, 6, true),
    };

    auto graph = constructRestrictionGraph(input);

    BOOST_CHECK_EQUAL(graph.nodes.size(), 5);
    BOOST_CHECK_EQUAL(graph.num_via_nodes, 3);
    BOOST_CHECK_EQUAL(graph.edges.size(), 3);
    BOOST_CHECK_EQUAL(graph.start_edge_to_node.size(), 2);
    BOOST_CHECK_EQUAL(graph.via_edge_to_node.size(), 3);

    auto start_edges_1 = validateStartRestrictionNode(graph, 0, 1, {}, {2});
    auto start_edges_2 = validateStartRestrictionNode(graph, 3, 4, {}, {5});

    auto via_edges_1 = validateViaRestrictionNode(graph, start_edges_1[2], 1, 2, {}, {3});
    validateViaRestrictionNode(graph, via_edges_1[3], 2, 3, {{4, true}}, {});
    validateViaRestrictionNode(graph, start_edges_2[5], 4, 5, {{6, true}}, {});
}

BOOST_AUTO_TEST_CASE(self_nested_restriction)
{
    //
    // Input
    // 0 -> 1 -> 0 -> 1 -> 2: only_3
    //
    // Output
    // s(0,1) -> v(1,0) -> v(0,1) -> v(1,2,[only_3])
    //              \___t___/
    //
    std::vector<TurnRestriction> input{
        makeWayRestriction(0,
                           {
                               1,
                               0,
                               1,
                               2,
                           },
                           3,
                           true),
    };

    auto graph = constructRestrictionGraph(input);

    BOOST_CHECK_EQUAL(graph.nodes.size(), 4);
    BOOST_CHECK_EQUAL(graph.num_via_nodes, 3);
    BOOST_CHECK_EQUAL(graph.edges.size(), 4);
    BOOST_CHECK_EQUAL(graph.start_edge_to_node.size(), 1);
    BOOST_CHECK_EQUAL(graph.via_edge_to_node.size(), 3);

    auto start_edges_1 = validateStartRestrictionNode(graph, 0, 1, {}, {0});

    auto via_edges_1 = validateViaRestrictionNode(graph, start_edges_1[0], 1, 0, {}, {1});
    auto via_edges_2 = validateViaRestrictionNode(graph, via_edges_1[1], 0, 1, {}, {0, 2});
    validateViaRestrictionNode(graph, via_edges_2[2], 1, 2, {{3, true}}, {});
}

BOOST_AUTO_TEST_CASE(single_node_restriction)
{
    //
    // Input
    // 0 -> 0 -> 0 -> 0 -> 0: only_0
    //
    // Output
    // s(0,0) -> v(0,0) -> v(0,0) -> v(0,0,[only_0])
    //                                 \t/
    //
    std::vector<TurnRestriction> input{
        makeWayRestriction(0,
                           {
                               0,
                               0,
                               0,
                               0,
                           },
                           0,
                           true),
    };

    auto graph = constructRestrictionGraph(input);

    BOOST_CHECK_EQUAL(graph.nodes.size(), 4);
    BOOST_CHECK_EQUAL(graph.num_via_nodes, 3);
    BOOST_CHECK_EQUAL(graph.edges.size(), 4);
    BOOST_CHECK_EQUAL(graph.start_edge_to_node.size(), 1);
    BOOST_CHECK_EQUAL(graph.via_edge_to_node.size(), 3);
    BOOST_CHECK_EQUAL(graph.via_edge_to_node.count({0, 0}), 3);

    auto start_edges_1 = validateStartRestrictionNode(graph, 0, 0, {}, {0});

    auto via_edges_1 = validateViaRestrictionNode(graph, start_edges_1[0], 0, 0, {}, {0});
    auto via_edges_2 = validateViaRestrictionNode(graph, via_edges_1[0], 0, 0, {}, {0});
    validateViaRestrictionNode(graph, via_edges_2[0], 0, 0, {{0, true}}, {0});
}

BOOST_AUTO_TEST_SUITE_END()
