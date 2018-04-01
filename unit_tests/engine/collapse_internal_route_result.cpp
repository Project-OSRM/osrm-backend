#include "engine/internal_route_result.hpp"
#include "engine/phantom_node.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <iterator>
#include <vector>

BOOST_AUTO_TEST_SUITE(collapse_test)

using namespace osrm;
using namespace osrm::util;
using namespace osrm::engine;

BOOST_AUTO_TEST_CASE(unchanged_collapse_route_result)
{
    PhantomNode source;
    PhantomNode target;
    source.forward_segment_id = {1, true};
    target.forward_segment_id = {6, true};
    PathData pathy{0, 2, 17, false, 2, 3, 4, 5, 0, {}, 4, 2, {}, 2, {1.0}, {1.0}, false};
    PathData kathy{0, 1, 16, false, 1, 2, 3, 4, 1, {}, 3, 1, {}, 1, {2.0}, {3.0}, false};
    InternalRouteResult one_leg_result;
    one_leg_result.unpacked_path_segments = {{pathy, kathy}};
    one_leg_result.segment_end_coordinates = {PhantomNodes{source, target}};
    one_leg_result.source_traversed_in_reverse = {true};
    one_leg_result.target_traversed_in_reverse = {true};
    one_leg_result.shortest_path_weight = 50;

    auto collapsed = CollapseInternalRouteResult(one_leg_result, {true, true});
    BOOST_CHECK_EQUAL(one_leg_result.unpacked_path_segments[0].front().turn_via_node,
                      collapsed.unpacked_path_segments[0].front().turn_via_node);
    BOOST_CHECK_EQUAL(one_leg_result.unpacked_path_segments[0].back().turn_via_node,
                      collapsed.unpacked_path_segments[0].back().turn_via_node);
}

BOOST_AUTO_TEST_CASE(two_legs_to_one_leg)
{
    // from_edge_based_node, turn_via_node, name_id, is_segregated, weight_until_turn,
    // weight_of_turn,
    // duration_until_turn, duration_of_turn, turn_instruction, lane_data, travel_mode, classes,
    // entry_class, datasource_id, pre_turn_bearing, post_turn_bearing, left_hand
    PathData pathy{0, 2, 17, false, 2, 3, 4, 5, 0, {}, 4, 2, {}, 2, {1.0}, {1.0}, false};
    PathData kathy{0, 1, 16, false, 1, 2, 3, 4, 1, {}, 3, 1, {}, 1, {2.0}, {3.0}, false};
    PathData cathy{0, 3, 16, false, 1, 2, 3, 4, 1, {}, 3, 1, {}, 1, {2.0}, {3.0}, false};
    PhantomNode node_1;
    PhantomNode node_2;
    PhantomNode node_3;
    node_1.forward_segment_id = {1, true};
    node_2.forward_segment_id = {6, true};
    node_3.forward_segment_id = {12, true};
    InternalRouteResult two_leg_result;
    two_leg_result.unpacked_path_segments = {{pathy, kathy}, {kathy, cathy}};
    two_leg_result.segment_end_coordinates = {PhantomNodes{node_1, node_2},
                                              PhantomNodes{node_2, node_3}};
    two_leg_result.source_traversed_in_reverse = {true, false};
    two_leg_result.target_traversed_in_reverse = {true, false};
    two_leg_result.shortest_path_weight = 80;

    auto collapsed = CollapseInternalRouteResult(two_leg_result, {true, false, true, true});
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments.size(), 1);
    BOOST_CHECK_EQUAL(collapsed.segment_end_coordinates.size(), 1);
    BOOST_CHECK_EQUAL(collapsed.segment_end_coordinates[0].target_phantom.forward_segment_id.id,
                      12);
    BOOST_CHECK_EQUAL(collapsed.segment_end_coordinates[0].source_phantom.forward_segment_id.id, 1);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[0].size(), 4);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[0][0].turn_via_node, 2);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[0][1].turn_via_node, 1);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[0][2].turn_via_node, 1);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[0][3].turn_via_node, 3);
}

BOOST_AUTO_TEST_CASE(three_legs_to_two_legs)
{
    PathData pathy{0, 2, 17, false, 2, 3, 4, 5, 0, {}, 4, 2, {}, 2, {1.0}, {1.0}, false};
    PathData kathy{0, 1, 16, false, 1, 2, 3, 4, 1, {}, 3, 1, {}, 1, {2.0}, {3.0}, false};
    PathData qathy{0, 5, 16, false, 1, 2, 3, 4, 1, {}, 3, 1, {}, 1, {2.0}, {3.0}, false};
    PathData cathy{0, 3, 16, false, 1, 2, 3, 4, 1, {}, 3, 1, {}, 1, {2.0}, {3.0}, false};
    PathData mathy{0, 4, 18, false, 8, 9, 13, 4, 2, {}, 4, 2, {}, 2, {3.0}, {1.0}, false};
    PhantomNode node_1;
    PhantomNode node_2;
    PhantomNode node_3;
    PhantomNode node_4;
    node_1.forward_segment_id = {1, true};
    node_2.forward_segment_id = {6, true};
    node_3.forward_segment_id = {12, true};
    node_4.forward_segment_id = {18, true};
    InternalRouteResult three_leg_result;
    three_leg_result.unpacked_path_segments = {std::vector<PathData>{pathy, kathy},
                                               std::vector<PathData>{kathy, qathy, cathy},
                                               std::vector<PathData>{cathy, mathy}};
    three_leg_result.segment_end_coordinates = {
        PhantomNodes{node_1, node_2}, PhantomNodes{node_2, node_3}, PhantomNodes{node_3, node_4}};
    three_leg_result.source_traversed_in_reverse = {true, false, true},
    three_leg_result.target_traversed_in_reverse = {true, false, true},
    three_leg_result.shortest_path_weight = 140;

    auto collapsed = CollapseInternalRouteResult(three_leg_result, {true, true, false, true});
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments.size(), 2);
    BOOST_CHECK_EQUAL(collapsed.segment_end_coordinates.size(), 2);
    BOOST_CHECK_EQUAL(collapsed.segment_end_coordinates[0].source_phantom.forward_segment_id.id, 1);
    BOOST_CHECK_EQUAL(collapsed.segment_end_coordinates[0].target_phantom.forward_segment_id.id, 6);
    BOOST_CHECK_EQUAL(collapsed.segment_end_coordinates[1].source_phantom.forward_segment_id.id, 6);
    BOOST_CHECK_EQUAL(collapsed.segment_end_coordinates[1].target_phantom.forward_segment_id.id,
                      18);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[0].size(), 2);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[1].size(), 5);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[0][0].turn_via_node, 2);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[0][1].turn_via_node, 1);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[1][0].turn_via_node, 1);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[1][1].turn_via_node, 5);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[1][2].turn_via_node, 3);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[1][3].turn_via_node, 3);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[1][4].turn_via_node, 4);
}

BOOST_AUTO_TEST_CASE(two_legs_to_two_legs)
{
    PathData pathy{0, 2, 17, false, 2, 3, 4, 5, 0, {}, 4, 2, {}, 2, {1.0}, {1.0}, false};
    PathData kathy{0, 1, 16, false, 1, 2, 3, 4, 1, {}, 3, 1, {}, 1, {2.0}, {3.0}, false};
    PathData cathy{0, 3, 16, false, 1, 2, 3, 4, 1, {}, 3, 1, {}, 1, {2.0}, {3.0}, false};
    PhantomNode node_1;
    PhantomNode node_2;
    PhantomNode node_3;
    node_1.forward_segment_id = {1, true};
    node_2.forward_segment_id = {6, true};
    node_3.forward_segment_id = {12, true};
    InternalRouteResult two_leg_result;
    two_leg_result.unpacked_path_segments = {{pathy, kathy}, {kathy, cathy}};
    two_leg_result.segment_end_coordinates = {PhantomNodes{node_1, node_2},
                                              PhantomNodes{node_2, node_3}};
    two_leg_result.source_traversed_in_reverse = {true, false};
    two_leg_result.target_traversed_in_reverse = {true, false};
    two_leg_result.shortest_path_weight = 80;

    auto collapsed = CollapseInternalRouteResult(two_leg_result, {true, true, true});
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments.size(), 2);
    BOOST_CHECK_EQUAL(collapsed.segment_end_coordinates.size(), 2);
    BOOST_CHECK_EQUAL(collapsed.segment_end_coordinates[0].source_phantom.forward_segment_id.id, 1);
    BOOST_CHECK_EQUAL(collapsed.segment_end_coordinates[0].target_phantom.forward_segment_id.id, 6);
    BOOST_CHECK_EQUAL(collapsed.segment_end_coordinates[1].source_phantom.forward_segment_id.id, 6);
    BOOST_CHECK_EQUAL(collapsed.segment_end_coordinates[1].target_phantom.forward_segment_id.id,
                      12);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[0].size(), 2);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[1].size(), 2);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[0][0].turn_via_node, 2);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[0][1].turn_via_node, 1);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[1][0].turn_via_node, 1);
    BOOST_CHECK_EQUAL(collapsed.unpacked_path_segments[1][1].turn_via_node, 3);
}

BOOST_AUTO_TEST_SUITE_END()
