#include "extractor/guidance/sliproad_handler.hpp"
#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/intersection_scenario_three_way.hpp"
#include "extractor/guidance/toolkit.hpp"

#include "util/guidance/toolkit.hpp"

#include <limits>
#include <utility>

#include <boost/assert.hpp>

using EdgeData = osrm::util::NodeBasedDynamicGraph::EdgeData;
using osrm::util::guidance::getTurnDirection;
using osrm::util::guidance::angularDeviation;

namespace osrm
{
namespace extractor
{
namespace guidance
{

SliproadHandler::SliproadHandler(const IntersectionGenerator &intersection_generator,
                                 const util::NodeBasedDynamicGraph &node_based_graph,
                                 const std::vector<QueryNode> &node_info_list,
                                 const util::NameTable &name_table,
                                 const SuffixTable &street_name_suffix_table)
    : IntersectionHandler(node_based_graph,
                          node_info_list,
                          name_table,
                          street_name_suffix_table,
                          intersection_generator)
{
}

// included for interface reasons only
bool SliproadHandler::canProcess(const NodeID /*nid*/,
                                 const EdgeID /*via_eid*/,
                                 const Intersection & /*intersection*/) const
{
    return true;
}

Intersection SliproadHandler::
operator()(const NodeID, const EdgeID source_edge_id, Intersection intersection) const
{
    auto intersection_node_id = node_based_graph.GetTarget(source_edge_id);

    // if there is no turn, there is no sliproad
    if (intersection.size() <= 2)
        return intersection;

    const auto findNextIntersectionForRoad =
        [&](const NodeID at_node, const ConnectedRoad &road, NodeID &output_node) {
            auto intersection = intersection_generator(at_node, road.turn.eid);
            auto in_edge = road.turn.eid;
            // skip over traffic lights
            while (intersection.size() == 2)
            {
                const auto node = node_based_graph.GetTarget(in_edge);
                in_edge = intersection[1].turn.eid;
                output_node = node_based_graph.GetTarget(in_edge);
                intersection = intersection_generator(node, in_edge);
            }
            return intersection;
        };

    const std::size_t obvious_turn_index = [&]() -> std::size_t {
        const auto index = findObviousTurn(source_edge_id, intersection);
        if (index != 0)
            return index;
        else if (intersection.size() == 3 &&
                 intersection[1].turn.instruction.type == TurnType::Fork)
        {
            // Forks themselves do not contain a `obvious` turn index. If we look at a fork that has
            // a one-sided sliproad, however, the non-sliproad can be considered `obvious`. Here we
            // assume that this could be the case and check for a potential sliproad/non-sliproad
            // situation.
            NodeID intersection_node_one, intersection_node_two;
            const auto intersection_following_index_one = findNextIntersectionForRoad(
                intersection_node_id, intersection[1], intersection_node_one);
            const auto intersection_following_index_two = findNextIntersectionForRoad(
                intersection_node_id, intersection[2], intersection_node_two);

            // In case of loops at the end of the road, we will arrive back at the intersection
            // itself. If that is the case, the road is obviously not a sliproad.

            // a sliproad has to enter a road without choice
            const auto couldBeSliproad = [](const Intersection &intersection) {
                if (intersection.size() != 3)
                    return false;
                if ((intersection[1].entry_allowed && intersection[2].entry_allowed) ||
                    intersection[0].entry_allowed)
                    return false;
                return true;
            };

            if (couldBeSliproad(intersection_following_index_one) &&
                intersection_node_id != intersection_node_two)
                return 2;
            else if (couldBeSliproad(intersection_following_index_two) &&
                     intersection_node_id != intersection_node_one)
                return 1;
            else
                return 0;
        }
        else
            return 0;
    }();

    if (obvious_turn_index == 0)
        return intersection;

    const auto &next_road = intersection[obvious_turn_index];

    const auto linkTest = [this, next_road](const ConnectedRoad &road) {
        return !node_based_graph.GetEdgeData(road.turn.eid).roundabout && road.entry_allowed &&
               angularDeviation(road.turn.angle, STRAIGHT_ANGLE) <= 2 * NARROW_TURN_ANGLE &&
               !hasRoundaboutType(road.turn.instruction) &&
               angularDeviation(next_road.turn.angle, road.turn.angle) >
                   std::numeric_limits<double>::epsilon();
    };

    bool hasNarrow =
        std::find_if(intersection.begin(), intersection.end(), linkTest) != intersection.end();
    if (!hasNarrow)
        return intersection;

    const auto source_edge_data = node_based_graph.GetEdgeData(source_edge_id);

    // check whether the continue road is valid
    const auto check_valid = [this, source_edge_data](const ConnectedRoad &road) {
        const auto road_edge_data = node_based_graph.GetEdgeData(road.turn.eid);
        // Test to see if the source edge and the one we're looking at are the same road
        return road_edge_data.road_classification == source_edge_data.road_classification &&
               road_edge_data.name_id != EMPTY_NAMEID &&
               road_edge_data.name_id == source_edge_data.name_id && road.entry_allowed;
    };

    if (!check_valid(next_road))
        return intersection;

    // Threshold check, if the intersection is too far away, don't bother continuing
    const auto &next_road_data = node_based_graph.GetEdgeData(next_road.turn.eid);
    if (next_road_data.distance > MAX_SLIPROAD_THRESHOLD)
    {
        return intersection;
    }
    auto next_intersection_node = node_based_graph.GetTarget(next_road.turn.eid);

    const auto next_road_next_intersection =
        findNextIntersectionForRoad(intersection_node_id, next_road, next_intersection_node);

    // If we are at a traffic loop at the end of a road, don't consider it a sliproad
    if (intersection_node_id == next_intersection_node)
        return intersection;

    std::unordered_set<NameID> target_road_names;

    for (const auto &road : next_road_next_intersection)
    {
        const auto &target_data = node_based_graph.GetEdgeData(road.turn.eid);
        target_road_names.insert(target_data.name_id);
    }

    for (auto &road : intersection)
    {
        if (linkTest(road))
        {
            EdgeID candidate_in = road.turn.eid;
            const auto target_intersection = [&](NodeID node) {
                auto intersection = intersection_generator(node, candidate_in);
                // skip over traffic lights
                if (intersection.size() == 2)
                {
                    node = node_based_graph.GetTarget(candidate_in);
                    candidate_in = intersection[1].turn.eid;
                    intersection = intersection_generator(node, candidate_in);
                }
                return intersection;
            }(intersection_node_id);

            for (const auto &candidate_road : target_intersection)
            {
                const auto &candidate_data = node_based_graph.GetEdgeData(candidate_road.turn.eid);
                if (target_road_names.count(candidate_data.name_id) > 0)
                {
                    if (node_based_graph.GetTarget(candidate_road.turn.eid) ==
                        next_intersection_node)
                    {
                        road.turn.instruction.type = TurnType::Sliproad;
                        break;
                    }
                    else
                    {
                        const auto skip_traffic_light_intersection = intersection_generator(
                            node_based_graph.GetTarget(candidate_in), candidate_road.turn.eid);
                        if (skip_traffic_light_intersection.size() == 2 &&
                            node_based_graph.GetTarget(
                                skip_traffic_light_intersection[1].turn.eid) ==
                                next_intersection_node)
                        {

                            road.turn.instruction.type = TurnType::Sliproad;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (next_road.turn.instruction.type == TurnType::Fork)
    {
        const auto &next_data = node_based_graph.GetEdgeData(next_road.turn.eid);
        if (next_data.name_id == source_edge_data.name_id)
        {
            if (angularDeviation(next_road.turn.angle, STRAIGHT_ANGLE) < 5)
                intersection[obvious_turn_index].turn.instruction.type = TurnType::Suppressed;
            else
                intersection[obvious_turn_index].turn.instruction.type = TurnType::Continue;
            intersection[obvious_turn_index].turn.instruction.direction_modifier =
                getTurnDirection(intersection[obvious_turn_index].turn.angle);
        }
        else if (next_data.name_id != EMPTY_NAMEID)
        {
            intersection[obvious_turn_index].turn.instruction.type = TurnType::NewName;
            intersection[obvious_turn_index].turn.instruction.direction_modifier =
                getTurnDirection(intersection[obvious_turn_index].turn.angle);
        }
        else
        {
            intersection[obvious_turn_index].turn.instruction.type = TurnType::Suppressed;
        }
    }

    return intersection;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
