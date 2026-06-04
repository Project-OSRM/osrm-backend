#include "guidance/turn_instruction.hpp"
#include "guidance/turn_lane_matcher.hpp"

#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include <boost/test/unit_test.hpp>

namespace TurnLaneType = osrm::extractor::TurnLaneType;
using namespace osrm::guidance;
using namespace osrm::guidance::lanes;

namespace
{

namespace IntersectionDetail = osrm::extractor::intersection;

// Helper to create a TurnInstruction with given type and direction modifier.
TurnInstruction makeInstruction(TurnType::Enum type, DirectionModifier::Enum modifier)
{ return {type, modifier}; }

// Helper to create a ConnectedRoad.
// eid, entry_allowed, angle, instruction, lane_data_id
ConnectedRoad makeRoad(EdgeID eid,
                       bool entry_allowed,
                       double angle,
                       TurnInstruction instruction = {TurnType::Turn, DirectionModifier::Straight},
                       LaneDataID lane_data_id = INVALID_LANE_DATAID)
{
    IntersectionDetail::IntersectionEdgeGeometry geom;
    geom.eid = eid;
    geom.initial_bearing = 0.0;
    geom.perceived_bearing = 0.0;
    geom.segment_length = 0.0;

    IntersectionDetail::IntersectionViewData view(geom, entry_allowed, angle);
    return ConnectedRoad(view, instruction, lane_data_id);
}

// Build an Intersection from a list of road specs.
// Each spec: {entry_allowed, angle, TurnInstruction}
Intersection makeIntersection(const std::vector<std::tuple<bool, double, TurnInstruction>> &specs)
{
    Intersection intersection;
    EdgeID eid = 0;
    for (const auto &[allowed, angle, instr] : specs)
    {
        intersection.push_back(makeRoad(eid++, allowed, angle, instr));
    }
    return intersection;
}

// Build LaneDataVector from a list of TurnLaneType::Mask tags.
LaneDataVector makeLaneData(const std::vector<TurnLaneType::Mask> &tags)
{
    LaneDataVector result;
    LaneID from = 0;
    for (auto tag : tags)
    {
        result.push_back({tag, from, from});
        from++;
    }
    return result;
}

// Build a minimal NodeBasedDynamicGraph for use in triviallyMatchLanesToTurns tests.
// Each edge is defined as {source, target, reversed}.
osrm::util::NodeBasedDynamicGraph
makeGraph(const std::vector<std::tuple<NodeID, NodeID, bool>> &edges, NodeID num_nodes)
{
    using InputEdge = osrm::util::NodeBasedDynamicGraph::InputEdge;
    std::vector<InputEdge> input_edges;
    input_edges.reserve(edges.size());
    for (const auto &[src, tgt, rev] : edges)
    {
        input_edges.push_back(InputEdge{src,
                                        tgt,
                                        EdgeWeight{1},
                                        EdgeDuration{1},
                                        EdgeDistance{1},
                                        GeometryID{0, false},
                                        rev,
                                        osrm::extractor::NodeBasedEdgeClassification(),
                                        0});
    }
    return osrm::util::NodeBasedDynamicGraph(num_nodes, input_edges);
}

} // anonymous namespace

// ============================================================================
// getMatchingModifier tests
// ============================================================================

BOOST_AUTO_TEST_SUITE(TurnLaneMatcherGetMatchingModifier)

BOOST_AUTO_TEST_CASE(uturn_tag_maps_to_uturn)
{ BOOST_CHECK_EQUAL(getMatchingModifier(TurnLaneType::uturn), DirectionModifier::UTurn); }

BOOST_AUTO_TEST_CASE(sharp_right_tag_maps_to_sharp_right)
{
    BOOST_CHECK_EQUAL(getMatchingModifier(TurnLaneType::sharp_right),
                      DirectionModifier::SharpRight);
}

BOOST_AUTO_TEST_CASE(right_tag_maps_to_right)
{ BOOST_CHECK_EQUAL(getMatchingModifier(TurnLaneType::right), DirectionModifier::Right); }

BOOST_AUTO_TEST_CASE(slight_right_tag_maps_to_slight_right)
{
    BOOST_CHECK_EQUAL(getMatchingModifier(TurnLaneType::slight_right),
                      DirectionModifier::SlightRight);
}

BOOST_AUTO_TEST_CASE(straight_tag_maps_to_straight)
{ BOOST_CHECK_EQUAL(getMatchingModifier(TurnLaneType::straight), DirectionModifier::Straight); }

BOOST_AUTO_TEST_CASE(slight_left_tag_maps_to_slight_left)
{
    BOOST_CHECK_EQUAL(getMatchingModifier(TurnLaneType::slight_left),
                      DirectionModifier::SlightLeft);
}

BOOST_AUTO_TEST_CASE(left_tag_maps_to_left)
{ BOOST_CHECK_EQUAL(getMatchingModifier(TurnLaneType::left), DirectionModifier::Left); }

BOOST_AUTO_TEST_CASE(sharp_left_tag_maps_to_sharp_left)
{ BOOST_CHECK_EQUAL(getMatchingModifier(TurnLaneType::sharp_left), DirectionModifier::SharpLeft); }

BOOST_AUTO_TEST_CASE(merge_to_left_tag_maps_to_straight)
{
    BOOST_CHECK_EQUAL(getMatchingModifier(TurnLaneType::merge_to_left),
                      DirectionModifier::Straight);
}

BOOST_AUTO_TEST_CASE(merge_to_right_tag_maps_to_straight)
{
    BOOST_CHECK_EQUAL(getMatchingModifier(TurnLaneType::merge_to_right),
                      DirectionModifier::Straight);
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// isValidMatch tests - uturn tag
// ============================================================================

BOOST_AUTO_TEST_SUITE(TurnLaneMatcherIsValidMatch)

BOOST_AUTO_TEST_CASE(uturn_tag_matches_left_modifier)
{
    // uturn is valid when instruction has a left modifier
    auto instr = makeInstruction(TurnType::Turn, DirectionModifier::Left);
    BOOST_CHECK(isValidMatch(TurnLaneType::uturn, instr));

    instr = makeInstruction(TurnType::Turn, DirectionModifier::SharpLeft);
    BOOST_CHECK(isValidMatch(TurnLaneType::uturn, instr));

    instr = makeInstruction(TurnType::Turn, DirectionModifier::SlightLeft);
    BOOST_CHECK(isValidMatch(TurnLaneType::uturn, instr));
}

BOOST_AUTO_TEST_CASE(uturn_tag_matches_uturn_direction)
{
    auto instr = makeInstruction(TurnType::Turn, DirectionModifier::UTurn);
    BOOST_CHECK(isValidMatch(TurnLaneType::uturn, instr));
}

BOOST_AUTO_TEST_CASE(uturn_tag_does_not_match_right_modifier)
{
    auto instr = makeInstruction(TurnType::Turn, DirectionModifier::Right);
    BOOST_CHECK(!isValidMatch(TurnLaneType::uturn, instr));
}

BOOST_AUTO_TEST_CASE(uturn_tag_does_not_match_straight)
{
    auto instr = makeInstruction(TurnType::Turn, DirectionModifier::Straight);
    BOOST_CHECK(!isValidMatch(TurnLaneType::uturn, instr));
}

// ============================================================================
// isValidMatch tests - right-side tags
// ============================================================================

BOOST_AUTO_TEST_CASE(sharp_right_matches_right_modifier)
{
    auto instr = makeInstruction(TurnType::Turn, DirectionModifier::SharpRight);
    BOOST_CHECK(isValidMatch(TurnLaneType::sharp_right, instr));

    instr = makeInstruction(TurnType::Turn, DirectionModifier::Right);
    BOOST_CHECK(isValidMatch(TurnLaneType::right, instr));

    instr = makeInstruction(TurnType::Turn, DirectionModifier::SlightRight);
    BOOST_CHECK(isValidMatch(TurnLaneType::slight_right, instr));
}

BOOST_AUTO_TEST_CASE(right_tags_match_leaves_roundabout)
{
    auto instr = makeInstruction(TurnType::ExitRoundabout, DirectionModifier::Straight);
    BOOST_CHECK(isValidMatch(TurnLaneType::right, instr));
}

BOOST_AUTO_TEST_CASE(sharp_right_tag_merge_mirrored_matches_left_modifier)
{
    // Merge type mirrors: left modifier → valid for "right" lane tag
    auto instr = makeInstruction(TurnType::Merge, DirectionModifier::Left);
    BOOST_CHECK(isValidMatch(TurnLaneType::sharp_right, instr));

    instr = makeInstruction(TurnType::Merge, DirectionModifier::SlightLeft);
    BOOST_CHECK(isValidMatch(TurnLaneType::slight_right, instr));
}

BOOST_AUTO_TEST_CASE(sharp_right_tag_merge_mirrored_does_not_match_right_modifier)
{
    // Merge with right modifier → should NOT match right lane tag (mirrored logic)
    auto instr = makeInstruction(TurnType::Merge, DirectionModifier::Right);
    BOOST_CHECK(!isValidMatch(TurnLaneType::sharp_right, instr));
}

BOOST_AUTO_TEST_CASE(right_tag_non_merge_does_not_match_left_modifier)
{
    // Non-merge instruction with left modifier → NOT valid for right lane tag
    auto instr = makeInstruction(TurnType::Turn, DirectionModifier::Left);
    BOOST_CHECK(!isValidMatch(TurnLaneType::right, instr));
}

// ============================================================================
// isValidMatch tests - straight tag (UNCOVERED: lines 66-79)
// ============================================================================

BOOST_AUTO_TEST_CASE(straight_tag_matches_straight_modifier)
{
    auto instr = makeInstruction(TurnType::Turn, DirectionModifier::Straight);
    BOOST_CHECK(isValidMatch(TurnLaneType::straight, instr));
}

BOOST_AUTO_TEST_CASE(straight_tag_matches_suppressed_type)
{
    auto instr = makeInstruction(TurnType::Suppressed, DirectionModifier::SlightRight);
    BOOST_CHECK(isValidMatch(TurnLaneType::straight, instr));
}

BOOST_AUTO_TEST_CASE(straight_tag_matches_new_name_type)
{
    auto instr = makeInstruction(TurnType::NewName, DirectionModifier::Straight);
    BOOST_CHECK(isValidMatch(TurnLaneType::straight, instr));
}

BOOST_AUTO_TEST_CASE(straight_tag_matches_stay_on_roundabout)
{
    auto instr = makeInstruction(TurnType::StayOnRoundabout, DirectionModifier::Straight);
    BOOST_CHECK(isValidMatch(TurnLaneType::straight, instr));
}

BOOST_AUTO_TEST_CASE(straight_tag_matches_enters_roundabout)
{
    auto instr = makeInstruction(TurnType::EnterRoundabout, DirectionModifier::Right);
    BOOST_CHECK(isValidMatch(TurnLaneType::straight, instr));

    instr = makeInstruction(TurnType::EnterRotary, DirectionModifier::Left);
    BOOST_CHECK(isValidMatch(TurnLaneType::straight, instr));

    instr = makeInstruction(TurnType::EnterRoundaboutIntersection, DirectionModifier::Straight);
    BOOST_CHECK(isValidMatch(TurnLaneType::straight, instr));

    instr = makeInstruction(TurnType::EnterAndExitRoundabout, DirectionModifier::Straight);
    BOOST_CHECK(isValidMatch(TurnLaneType::straight, instr));
}

BOOST_AUTO_TEST_CASE(straight_tag_matches_fork_with_slight_modifier)
{
    // Fork + SlightLeft
    auto instr = makeInstruction(TurnType::Fork, DirectionModifier::SlightLeft);
    BOOST_CHECK(isValidMatch(TurnLaneType::straight, instr));

    // Fork + SlightRight
    instr = makeInstruction(TurnType::Fork, DirectionModifier::SlightRight);
    BOOST_CHECK(isValidMatch(TurnLaneType::straight, instr));
}

BOOST_AUTO_TEST_CASE(straight_tag_does_not_match_fork_with_non_slight_modifier)
{
    // Fork + Right → NOT valid (only Fork+SlightLeft/SlightRight are valid for straight tag)
    auto instr = makeInstruction(TurnType::Fork, DirectionModifier::Right);
    BOOST_CHECK(!isValidMatch(TurnLaneType::straight, instr));

    // Fork + Left → NOT valid
    instr = makeInstruction(TurnType::Fork, DirectionModifier::Left);
    BOOST_CHECK(!isValidMatch(TurnLaneType::straight, instr));
}

BOOST_AUTO_TEST_CASE(straight_tag_matches_continue_with_slight_modifier)
{
    // Continue + SlightLeft
    auto instr = makeInstruction(TurnType::Continue, DirectionModifier::SlightLeft);
    BOOST_CHECK(isValidMatch(TurnLaneType::straight, instr));

    // Continue + SlightRight
    instr = makeInstruction(TurnType::Continue, DirectionModifier::SlightRight);
    BOOST_CHECK(isValidMatch(TurnLaneType::straight, instr));
}

BOOST_AUTO_TEST_CASE(straight_tag_does_not_match_continue_with_non_slight_modifier)
{
    // Continue + Right → NOT valid (only Continue+SlightLeft/SlightRight are valid for straight
    // tag)
    auto instr = makeInstruction(TurnType::Continue, DirectionModifier::Right);
    BOOST_CHECK(!isValidMatch(TurnLaneType::straight, instr));

    // Continue + Left → NOT valid
    instr = makeInstruction(TurnType::Continue, DirectionModifier::Left);
    BOOST_CHECK(!isValidMatch(TurnLaneType::straight, instr));
}

BOOST_AUTO_TEST_CASE(straight_tag_does_not_match_random_turn)
{
    auto instr = makeInstruction(TurnType::Turn, DirectionModifier::Right);
    BOOST_CHECK(!isValidMatch(TurnLaneType::straight, instr));

    instr = makeInstruction(TurnType::Turn, DirectionModifier::Left);
    BOOST_CHECK(!isValidMatch(TurnLaneType::straight, instr));
}

// ============================================================================
// isValidMatch tests - left-side tags (UNCOVERED: lines 80-89)
// ============================================================================

BOOST_AUTO_TEST_CASE(left_tag_matches_left_modifier)
{
    auto instr = makeInstruction(TurnType::Turn, DirectionModifier::Left);
    BOOST_CHECK(isValidMatch(TurnLaneType::left, instr));

    instr = makeInstruction(TurnType::Turn, DirectionModifier::SharpLeft);
    BOOST_CHECK(isValidMatch(TurnLaneType::sharp_left, instr));

    instr = makeInstruction(TurnType::Turn, DirectionModifier::SlightLeft);
    BOOST_CHECK(isValidMatch(TurnLaneType::slight_left, instr));
}

BOOST_AUTO_TEST_CASE(left_tag_matches_stay_on_roundabout)
{
    auto instr = makeInstruction(TurnType::StayOnRoundabout, DirectionModifier::Straight);
    BOOST_CHECK(isValidMatch(TurnLaneType::left, instr));
    BOOST_CHECK(isValidMatch(TurnLaneType::slight_left, instr));
    BOOST_CHECK(isValidMatch(TurnLaneType::sharp_left, instr));
}

BOOST_AUTO_TEST_CASE(left_tag_merge_mirrored_matches_right_modifier)
{
    // Merge with right modifier → valid for left lane tag (mirrored)
    auto instr = makeInstruction(TurnType::Merge, DirectionModifier::Right);
    BOOST_CHECK(isValidMatch(TurnLaneType::left, instr));

    instr = makeInstruction(TurnType::Merge, DirectionModifier::SharpRight);
    BOOST_CHECK(isValidMatch(TurnLaneType::sharp_left, instr));

    instr = makeInstruction(TurnType::Merge, DirectionModifier::SlightRight);
    BOOST_CHECK(isValidMatch(TurnLaneType::slight_left, instr));
}

BOOST_AUTO_TEST_CASE(left_tag_merge_mirrored_does_not_match_left_modifier)
{
    // Merge with left modifier → NOT valid for left lane tag
    auto instr = makeInstruction(TurnType::Merge, DirectionModifier::Left);
    BOOST_CHECK(!isValidMatch(TurnLaneType::left, instr));
}

BOOST_AUTO_TEST_CASE(left_tag_non_merge_does_not_match_right_modifier)
{
    // Non-merge, non-StayOnRoundabout, right modifier → NOT valid
    auto instr = makeInstruction(TurnType::Turn, DirectionModifier::Right);
    BOOST_CHECK(!isValidMatch(TurnLaneType::left, instr));
}

BOOST_AUTO_TEST_CASE(left_tag_non_merge_straight_not_valid)
{
    auto instr = makeInstruction(TurnType::Turn, DirectionModifier::Straight);
    BOOST_CHECK(!isValidMatch(TurnLaneType::left, instr));
}

// ============================================================================
// isValidMatch tests - unhandled tags (line 91)
// ============================================================================

BOOST_AUTO_TEST_CASE(merge_to_left_tag_always_returns_false)
{
    // merge_to_left and merge_to_right fall through to return false
    auto instr = makeInstruction(TurnType::Turn, DirectionModifier::Straight);
    BOOST_CHECK(!isValidMatch(TurnLaneType::merge_to_left, instr));

    instr = makeInstruction(TurnType::Merge, DirectionModifier::Left);
    BOOST_CHECK(!isValidMatch(TurnLaneType::merge_to_left, instr));
}

BOOST_AUTO_TEST_CASE(merge_to_right_tag_always_returns_false)
{
    auto instr = makeInstruction(TurnType::Turn, DirectionModifier::Straight);
    BOOST_CHECK(!isValidMatch(TurnLaneType::merge_to_right, instr));
}

BOOST_AUTO_TEST_CASE(none_tag_returns_false)
{
    auto instr = makeInstruction(TurnType::Turn, DirectionModifier::Straight);
    BOOST_CHECK(!isValidMatch(TurnLaneType::none, instr));
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// getMatchingQuality tests
// ============================================================================

BOOST_AUTO_TEST_SUITE(TurnLaneMatcherGetMatchingQuality)

BOOST_AUTO_TEST_CASE(straight_tag_quality)
{
    // Straight ideal angle is 180 (from idealized_turn_angles[Straight=4])
    ConnectedRoad road = makeRoad(0, true, 180.0);
    BOOST_CHECK_SMALL(getMatchingQuality(TurnLaneType::straight, road), 1e-6);

    road = makeRoad(0, true, 200.0);
    BOOST_CHECK_CLOSE(getMatchingQuality(TurnLaneType::straight, road), 20.0, 0.1);
}

BOOST_AUTO_TEST_CASE(right_tag_quality)
{
    // Right ideal angle is 90
    ConnectedRoad road = makeRoad(0, true, 90.0);
    BOOST_CHECK_SMALL(getMatchingQuality(TurnLaneType::right, road), 1e-6);

    road = makeRoad(0, true, 100.0);
    BOOST_CHECK_CLOSE(getMatchingQuality(TurnLaneType::right, road), 10.0, 0.1);
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// findBestMatch tests
// ============================================================================

BOOST_AUTO_TEST_SUITE(TurnLaneMatcherFindBestMatch)

BOOST_AUTO_TEST_CASE(prefers_valid_match_over_invalid)
{
    Intersection intersection = makeIntersection(
        {{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},        // [0] u-turn
         {true, 90.0, {TurnType::Turn, DirectionModifier::Right}},       // [1] valid for right
         {true, 180.0, {TurnType::Turn, DirectionModifier::Straight}}}); // [2] invalid for right

    auto it = findBestMatch(TurnLaneType::right, intersection);
    // Should pick [1] because it's a valid match for 'right' tag
    BOOST_CHECK_EQUAL(std::distance(intersection.cbegin(), it), 1);
}

BOOST_AUTO_TEST_CASE(prefers_entry_allowed_when_both_valid)
{
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {false, 90.0, {TurnType::Turn, DirectionModifier::Right}},
                          {true, 100.0, {TurnType::Turn, DirectionModifier::Right}}});

    auto it = findBestMatch(TurnLaneType::right, intersection);
    // Should pick [2] because entry_allowed=true, even though [1] has closer angle
    BOOST_CHECK_EQUAL(std::distance(intersection.cbegin(), it), 2);
}

BOOST_AUTO_TEST_CASE(prefers_better_angle_when_both_valid_and_allowed)
{
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 95.0, {TurnType::Turn, DirectionModifier::Right}},
                          {true, 85.0, {TurnType::Turn, DirectionModifier::Right}}});

    auto it = findBestMatch(TurnLaneType::right, intersection);
    // [2] has angle 85. Right ideal is 90. |85-90|=5 < |95-90|=5. Actually both are 5.
    // Let's make it cleaner with different angles.
    // Actually 85→|85-90|=5, 95→|95-90|=5. Both equal. The first min_element wins.
    // Let's fix the test to have clearly different angles.
    intersection = makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                                     {true, 110.0, {TurnType::Turn, DirectionModifier::Right}},
                                     {true, 85.0, {TurnType::Turn, DirectionModifier::Right}}});

    it = findBestMatch(TurnLaneType::right, intersection);
    // [2] has angle 85, dev=5. [1] has angle 110, dev=20. Should pick [2].
    BOOST_CHECK_EQUAL(std::distance(intersection.cbegin(), it), 2);
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// findBestMatchForReverse tests
// ============================================================================

BOOST_AUTO_TEST_SUITE(TurnLaneMatcherFindBestMatchForReverse)

BOOST_AUTO_TEST_CASE(neighbor_is_last_returns_begin)
{
    // When the neighbor (e.g. sharp_left) is the last element, the u-turn must be at [0]
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 90.0, {TurnType::Turn, DirectionModifier::Right}},
                          {true, 315.0, {TurnType::Turn, DirectionModifier::SharpLeft}}});

    // sharp_left is at index 2 (the last element)
    auto it = findBestMatchForReverse(TurnLaneType::sharp_left, intersection);
    // neighbor is at end, so neighbor_itr + 1 == cend() → returns begin()
    BOOST_CHECK_EQUAL(std::distance(intersection.cbegin(), it), 0);
}

BOOST_AUTO_TEST_CASE(finds_best_uturn_after_neighbor)
{
    // Intersection with u-turn at [0] and two right-turn roads.
    // findBestMatch(right) picks [1] (angle 90, better match than [2] at 100).
    // findBestMatchForReverse(right) searches from [1] to end for best u-turn.
    // Neither [1] nor [2] is valid for u-turn, so quality decides: [1] wins.
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 90.0, {TurnType::Turn, DirectionModifier::Right}},
                          {true, 100.0, {TurnType::Turn, DirectionModifier::Right}}});

    auto it = findBestMatchForReverse(TurnLaneType::right, intersection);
    // neighbor is 'right' at [1]; search from [1] to end for best u-turn match
    BOOST_CHECK_EQUAL(std::distance(intersection.cbegin(), it), 1);
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// canMatchTrivially tests (UNCOVERED: lines 186-188)
// ============================================================================

BOOST_AUTO_TEST_SUITE(TurnLaneMatcherCanMatchTrivially)

BOOST_AUTO_TEST_CASE(empty_lane_data_returns_true)
{
    // Empty lane_data: road_index=1, lane=0. Loop doesn't execute.
    // return lane == lane_data.size() → 0==0 → true.
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}}});
    LaneDataVector lanes;
    BOOST_CHECK(canMatchTrivially(intersection, lanes));
}

BOOST_AUTO_TEST_CASE(valid_trivial_match_returns_true)
{
    // Intersection: [0] u-turn, [1] right, [2] straight, [3] left
    // Lane data: right, straight, left → should match trivially
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 90.0, {TurnType::Turn, DirectionModifier::Right}},
                          {true, 180.0, {TurnType::Turn, DirectionModifier::Straight}},
                          {true, 270.0, {TurnType::Turn, DirectionModifier::Left}}});

    LaneDataVector lanes =
        makeLaneData({TurnLaneType::right, TurnLaneType::straight, TurnLaneType::left});
    BOOST_CHECK(canMatchTrivially(intersection, lanes));
}

BOOST_AUTO_TEST_CASE(invalid_match_returns_false)
{
    // Lane data says straight but best match for straight is at [2] where the road is Left
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 90.0, {TurnType::Turn, DirectionModifier::Right}},
                          {true, 270.0, {TurnType::Turn, DirectionModifier::Left}}});

    LaneDataVector lanes = makeLaneData({TurnLaneType::straight});
    BOOST_CHECK(!canMatchTrivially(intersection, lanes));
}

BOOST_AUTO_TEST_CASE(end_condition_all_lanes_matched)
{
    // lane == lane_data.size()
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 90.0, {TurnType::Turn, DirectionModifier::Right}}});

    LaneDataVector lanes = makeLaneData({TurnLaneType::right});
    // lane becomes 1 after matching, road_index=2, loop exits.
    // return lane(1) == lane_data.size()(1) → true
    BOOST_CHECK(canMatchTrivially(intersection, lanes));
}

BOOST_AUTO_TEST_CASE(end_condition_one_left_uturn)
{
    // lane + 1 == lane_data.size() && lane_data.back().tag == uturn
    // e.g. 2 lanes: right, uturn. Only 'right' matches directly.
    // The uturn is left unmatched but that's OK.
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 90.0, {TurnType::Turn, DirectionModifier::Right}}});

    LaneDataVector lanes = makeLaneData({TurnLaneType::right, TurnLaneType::uturn});
    // lane matches right at [1], lane becomes 1. Loop exits.
    // lane(1) + 1 == lane_data.size()(2) && back().tag == uturn → true
    BOOST_CHECK(canMatchTrivially(intersection, lanes));
}

BOOST_AUTO_TEST_CASE(end_condition_one_left_not_uturn_returns_false)
{
    // One unmatched lane that is NOT uturn → false
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 90.0, {TurnType::Turn, DirectionModifier::Right}}});

    // Only one road with entry_allowed but two non-uturn lanes
    LaneDataVector lanes = makeLaneData({TurnLaneType::right, TurnLaneType::left});
    BOOST_CHECK(!canMatchTrivially(intersection, lanes));
}

BOOST_AUTO_TEST_CASE(first_lane_uturn_entry_allowed_skips_lane_zero)
{
    // First lane tag is uturn AND intersection[0].entry_allowed → lane = 1
    // This means we skip attempting to match the u-turn lane
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 180.0, {TurnType::Turn, DirectionModifier::Straight}}});

    LaneDataVector lanes = makeLaneData({TurnLaneType::uturn, TurnLaneType::straight});
    // First tag is uturn, entry allowed → lane skips to 1
    // Then road_index=1 matches straight at [1] → lane becomes 2 == size
    BOOST_CHECK(canMatchTrivially(intersection, lanes));
}

BOOST_AUTO_TEST_CASE(first_lane_uturn_entry_not_allowed_does_not_skip)
{
    // First lane tag is uturn but intersection[0].entry_allowed is false → lane stays 0
    Intersection intersection =
        makeIntersection({{false, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 180.0, {TurnType::Turn, DirectionModifier::Straight}}});

    LaneDataVector lanes = makeLaneData({TurnLaneType::uturn});
    // lane stays 0. But isValidMatch(uturn, Straight) → false.
    // So the loop finds it's not valid and returns false.
    BOOST_CHECK(!canMatchTrivially(intersection, lanes));
}

BOOST_AUTO_TEST_CASE(best_match_not_at_expected_position_returns_false)
{
    // Lane data says 'right'. findBestMatch for 'right' picks [2] (angle 90, ideal).
    // But road_index=1 checks intersection[1] first, which has angle 95.
    // Since findBestMatch returns [2] != begin()+1, returns false.
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 95.0, {TurnType::Turn, DirectionModifier::Right}},
                          {true, 90.0, {TurnType::Turn, DirectionModifier::Right}}});

    LaneDataVector lanes = makeLaneData({TurnLaneType::right});
    BOOST_CHECK(!canMatchTrivially(intersection, lanes));
}

BOOST_AUTO_TEST_SUITE_END()

// ============================================================================
// triviallyMatchLanesToTurns tests (UNCOVERED: lines 190-268)
// ============================================================================

BOOST_AUTO_TEST_SUITE(TurnLaneMatcherTriviallyMatchLanesToTurns)

BOOST_AUTO_TEST_CASE(empty_lane_data_returns_unchanged)
{
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 90.0, {TurnType::Turn, DirectionModifier::Right}}});

    auto graph = makeGraph({}, 0);
    osrm::util::guidance::LaneDataIdMap id_map;

    Intersection result =
        triviallyMatchLanesToTurns(intersection, {}, graph, LaneDescriptionID{0}, id_map);

    // Should return unchanged (no lanes to match)
    BOOST_CHECK_EQUAL(result.size(), 2);
}

BOOST_AUTO_TEST_CASE(first_lane_uturn_entry_allowed_not_reversed)
{
    // First lane is uturn, entry[0] allowed, edge NOT reversed
    // → u_turn=0, road_index=1 (but since lane_data has size 1, loop won't execute)
    // Actually wait: lane starts at 1 (due to u-turn skip). lane_data size is 2.
    // After matching the u-turn, lane=1. If lane_data has only the u-turn entry,
    // we need lane_data to have 2 entries: uturn (front) + something.
    // But from code: it checks lane_data.front().tag == uturn, then matches lane_data.back().
    // So lane_data needs at least 2 entries.
    // Actually let me re-read the code more carefully.

    // Lines 206-232:
    // if (!lane_data.empty() && lane_data.front().tag == TurnLaneType::uturn)
    // {
    //     if (intersection[0].entry_allowed)
    //     {
    //         ...
    //         matchRoad(intersection[u_turn], lane_data.back());
    //         lane = 1;
    //     }
    //     else return intersection;
    // }
    //
    // Lines 234-246: for loop matching remaining lanes
    //
    // Lines 248-266: trailing u-turn handling

    // So if lane_data = [uturn, straight]:
    // - front().tag == uturn → enter the if
    // - intersection[0].entry_allowed = true
    // - reversed = false → u_turn = 0
    // - intersection[0] gets Continue+UTurn
    // - matchRoad with lane_data.back() (the straight lane data!)
    //
    // Wait, that doesn't make sense. Why match lane_data.back() for the u-turn?
    // Let me re-read...
    //
    // Oh! I think the u-turn lane data entry has special from/to values that
    // represent the u-turn lanes. And in the LaneDataVector, when there's a u-turn
    // at the beginning, there might be a corresponding u-turn entry at the back too.
    //
    // Actually, I think the logic might be: when the front tag is uturn, the back
    // tag is also uturn (the lane data vector has uturn at both ends to bracket the
    // normal lanes). Or maybe the u-turn lane data is always stored at the back.
    //
    // Let me look at how LaneDataVector is constructed in production code.
    // Actually this is getting too deep. Let me just write tests with the right
    // data structure assumptions and see.

    // For the test, let's use lane_data = [uturn] (single entry).
    // front().tag == uturn, intersection[0].entry_allowed = true
    // reversed = false → u_turn = 0
    // matchRoad(intersection[0], lane_data.back()) → matches the only entry
    // lane = 1
    // Loop: road_index=1, lane=1, lane < lane_data.size() (1<1) → false
    // Trailing check: lane(1) + 1 == 1? No. 2 != 1.
    // Returns intersection.
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 180.0, {TurnType::Turn, DirectionModifier::Straight}}});

    auto graph = makeGraph({{0, 1, false}}, 2);
    osrm::util::guidance::LaneDataIdMap id_map;

    LaneDataVector lanes = makeLaneData({TurnLaneType::uturn});

    Intersection result =
        triviallyMatchLanesToTurns(intersection, lanes, graph, LaneDescriptionID{0}, id_map);

    // intersection[0] should become Continue + UTurn
    BOOST_CHECK_EQUAL(result[0].instruction.type, TurnType::Continue);
    BOOST_CHECK_EQUAL(result[0].instruction.direction_modifier, DirectionModifier::UTurn);
}

BOOST_AUTO_TEST_CASE(first_lane_uturn_entry_not_allowed_returns_early)
{
    Intersection intersection =
        makeIntersection({{false, 0.0, {TurnType::Turn, DirectionModifier::UTurn}}});

    auto graph = makeGraph({}, 0);
    osrm::util::guidance::LaneDataIdMap id_map;

    LaneDataVector lanes = makeLaneData({TurnLaneType::uturn});

    Intersection result =
        triviallyMatchLanesToTurns(intersection, lanes, graph, LaneDescriptionID{0}, id_map);

    // Should return unchanged (early return at line 231)
    BOOST_CHECK_EQUAL(result[0].instruction.type, TurnType::Turn);
}

BOOST_AUTO_TEST_CASE(first_lane_uturn_reversed_with_sharp_right)
{
    // reversed=true → u_turn=1 if intersection[1] is SharpRight
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 45.0, {TurnType::Turn, DirectionModifier::SharpRight}},
                          {true, 180.0, {TurnType::Turn, DirectionModifier::Straight}}});

    // Edge from node 0→1 with reversed=true
    auto graph = makeGraph({{0, 1, true}}, 2);
    osrm::util::guidance::LaneDataIdMap id_map;

    LaneDataVector lanes = makeLaneData({TurnLaneType::uturn});

    Intersection result =
        triviallyMatchLanesToTurns(intersection, lanes, graph, LaneDescriptionID{0}, id_map);

    // u_turn=1, road_index=2 → intersection[1] becomes Continue+UTurn
    BOOST_CHECK_EQUAL(result[1].instruction.type, TurnType::Continue);
    BOOST_CHECK_EQUAL(result[1].instruction.direction_modifier, DirectionModifier::UTurn);
    // intersection[0] should be unchanged
    BOOST_CHECK_EQUAL(result[0].instruction.type, TurnType::Turn);
}

BOOST_AUTO_TEST_CASE(first_lane_uturn_reversed_no_valid_sharp_right_returns_early)
{
    // reversed=true but intersection[1] is NOT SharpRight → early return
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 90.0, {TurnType::Turn, DirectionModifier::Right}}});

    auto graph = makeGraph({{0, 1, true}}, 2);
    osrm::util::guidance::LaneDataIdMap id_map;

    LaneDataVector lanes = makeLaneData({TurnLaneType::uturn});

    Intersection result =
        triviallyMatchLanesToTurns(intersection, lanes, graph, LaneDescriptionID{0}, id_map);

    // Early return at line 218 → intersection unchanged
    BOOST_CHECK_EQUAL(result[0].instruction.type, TurnType::Turn);
    BOOST_CHECK_EQUAL(result[1].instruction.type, TurnType::Turn);
}

BOOST_AUTO_TEST_CASE(first_lane_uturn_reversed_no_second_road_returns_early)
{
    // reversed=true but intersection.size() <= 1 → early return (line 214)
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}}});

    auto graph = makeGraph({{0, 1, true}}, 2);
    osrm::util::guidance::LaneDataIdMap id_map;

    LaneDataVector lanes = makeLaneData({TurnLaneType::uturn});

    Intersection result =
        triviallyMatchLanesToTurns(intersection, lanes, graph, LaneDescriptionID{0}, id_map);

    // Early return → unchanged
    BOOST_CHECK_EQUAL(result[0].instruction.type, TurnType::Turn);
}

BOOST_AUTO_TEST_CASE(first_lane_uturn_reversed_second_not_entry_allowed_returns_early)
{
    // reversed=true, intersection[1].entry_allowed=false → early return
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {false, 45.0, {TurnType::Turn, DirectionModifier::SharpRight}}});

    auto graph = makeGraph({{0, 1, true}}, 2);
    osrm::util::guidance::LaneDataIdMap id_map;

    LaneDataVector lanes = makeLaneData({TurnLaneType::uturn});

    Intersection result =
        triviallyMatchLanesToTurns(intersection, lanes, graph, LaneDescriptionID{0}, id_map);

    // Early return → unchanged
    BOOST_CHECK_EQUAL(result[0].instruction.type, TurnType::Turn);
}

BOOST_AUTO_TEST_CASE(normal_matching_loop)
{
    // Standard case: first lane is NOT uturn, normal matching
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 90.0, {TurnType::Turn, DirectionModifier::Right}},
                          {true, 180.0, {TurnType::Turn, DirectionModifier::Straight}},
                          {true, 270.0, {TurnType::Turn, DirectionModifier::Left}}});

    auto graph = makeGraph({}, 0);
    osrm::util::guidance::LaneDataIdMap id_map;

    LaneDataVector lanes =
        makeLaneData({TurnLaneType::right, TurnLaneType::straight, TurnLaneType::left});

    Intersection result =
        triviallyMatchLanesToTurns(intersection, lanes, graph, LaneDescriptionID{0}, id_map);

    // Each road should get a lane_data_id assigned
    BOOST_CHECK(result[1].lane_data_id != INVALID_LANE_DATAID);
    BOOST_CHECK(result[2].lane_data_id != INVALID_LANE_DATAID);
    BOOST_CHECK(result[3].lane_data_id != INVALID_LANE_DATAID);

    // lane_data_ids should be sequential starting from 0
    BOOST_CHECK_EQUAL(result[1].lane_data_id, LaneDataID{0});
    BOOST_CHECK_EQUAL(result[2].lane_data_id, LaneDataID{1});
    BOOST_CHECK_EQUAL(result[3].lane_data_id, LaneDataID{2});
}

BOOST_AUTO_TEST_CASE(trailing_uturn_not_reversed)
{
    // lane + 1 == lane_data.size() && lane_data.back().tag == uturn
    // edge not reversed → u_turn = 0, modifies intersection[0]
    // Loop starts at road_index=1 so intersection[0] is never processed in loop
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 90.0, {TurnType::Turn, DirectionModifier::Right}}});

    auto graph = makeGraph({{0, 1, false}}, 2);
    osrm::util::guidance::LaneDataIdMap id_map;

    // 'right' gets matched at [1], loop exits (road_index=2 >= size), uturn remains
    LaneDataVector lanes = makeLaneData({TurnLaneType::right, TurnLaneType::uturn});

    Intersection result =
        triviallyMatchLanesToTurns(intersection, lanes, graph, LaneDescriptionID{0}, id_map);

    // intersection[0] should become Continue+UTurn (uturn matched to ingress road)
    BOOST_CHECK_EQUAL(result[0].instruction.type, TurnType::Continue);
    BOOST_CHECK_EQUAL(result[0].instruction.direction_modifier, DirectionModifier::UTurn);
    // intersection[1] should have lane_data_id set
    BOOST_CHECK(result[1].lane_data_id != INVALID_LANE_DATAID);
}

BOOST_AUTO_TEST_CASE(trailing_uturn_reversed_last_road_not_sharp_left_returns_early)
{
    // reversed=true, last road is NOT SharpLeft → early return in trailing handler
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 90.0, {TurnType::Turn, DirectionModifier::Right}}});

    auto graph = makeGraph({{0, 1, true}}, 2);
    osrm::util::guidance::LaneDataIdMap id_map;

    LaneDataVector lanes = makeLaneData({TurnLaneType::right, TurnLaneType::uturn});

    Intersection result =
        triviallyMatchLanesToTurns(intersection, lanes, graph, LaneDescriptionID{0}, id_map);

    // Early return in trailing handler: intersection.back() is Right, not SharpLeft
    // intersection unchanged
    BOOST_CHECK_EQUAL(result[0].instruction.type, TurnType::Turn);
    BOOST_CHECK_EQUAL(result[1].instruction.type, TurnType::Turn);
}

BOOST_AUTO_TEST_CASE(trailing_uturn_reversed_last_road_not_entry_allowed_returns_early)
{
    // reversed=true, last road is SharpLeft but NOT entry_allowed → early return
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 90.0, {TurnType::Turn, DirectionModifier::Right}},
                          {false, 315.0, {TurnType::Turn, DirectionModifier::SharpLeft}}});

    auto graph = makeGraph({{0, 1, true}}, 2);
    osrm::util::guidance::LaneDataIdMap id_map;

    // Loop processes: road_index=1 → match right at [1], lane=1.
    // road_index=2 → entry_allowed=false → skip.
    // Loop exits. Trailing handler: reversed=true, back().entry_allowed=false → early return.
    LaneDataVector lanes = makeLaneData({TurnLaneType::right, TurnLaneType::uturn});

    Intersection result =
        triviallyMatchLanesToTurns(intersection, lanes, graph, LaneDescriptionID{0}, id_map);

    // Early return: intersection[2] unchanged
    BOOST_CHECK_EQUAL(result[2].instruction.type, TurnType::Turn);
    BOOST_CHECK_EQUAL(result[2].instruction.direction_modifier, DirectionModifier::SharpLeft);
}

BOOST_AUTO_TEST_CASE(first_lane_uturn_skips_lane_zero_normal_matching_follows)
{
    // First lane is uturn (entry allowed, not reversed) — handled by front handler
    // The matched lane is lane_data.back() which is the u-turn lane data itself for single entry
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}}});

    auto graph = makeGraph({{0, 1, false}}, 2);
    osrm::util::guidance::LaneDataIdMap id_map;

    LaneDataVector lanes = makeLaneData({TurnLaneType::uturn});

    Intersection result =
        triviallyMatchLanesToTurns(intersection, lanes, graph, LaneDescriptionID{0}, id_map);

    // intersection[0] → Continue+UTurn (matched by front handler)
    BOOST_CHECK_EQUAL(result[0].instruction.type, TurnType::Continue);
    BOOST_CHECK_EQUAL(result[0].instruction.direction_modifier, DirectionModifier::UTurn);
}

BOOST_AUTO_TEST_CASE(non_uturn_lanes_only_normal_matching)
{
    // Regular lanes, no u-turn at front or back
    Intersection intersection =
        makeIntersection({{true, 0.0, {TurnType::Turn, DirectionModifier::UTurn}},
                          {true, 180.0, {TurnType::Turn, DirectionModifier::Straight}},
                          {true, 270.0, {TurnType::Turn, DirectionModifier::Left}}});

    auto graph = makeGraph({}, 0);
    osrm::util::guidance::LaneDataIdMap id_map;

    LaneDataVector lanes = makeLaneData({TurnLaneType::straight, TurnLaneType::left});

    Intersection result =
        triviallyMatchLanesToTurns(intersection, lanes, graph, LaneDescriptionID{0}, id_map);

    // Both roads get lane_data_ids
    BOOST_CHECK(result[1].lane_data_id != INVALID_LANE_DATAID);
    BOOST_CHECK(result[2].lane_data_id != INVALID_LANE_DATAID);
    // intersection[0] unchanged (no u-turn lane data)
    BOOST_CHECK_EQUAL(result[0].instruction.type, TurnType::Turn);
}

BOOST_AUTO_TEST_SUITE_END()
