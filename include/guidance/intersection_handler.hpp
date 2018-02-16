#ifndef OSRM_GUIDANCE_INTERSECTION_HANDLER_HPP_
#define OSRM_GUIDANCE_INTERSECTION_HANDLER_HPP_

#include "extractor/intersection/intersection_analysis.hpp"
#include "extractor/intersection/node_based_graph_walker.hpp"
#include "extractor/suffix_table.hpp"
#include "guidance/constants.hpp"
#include "guidance/intersection.hpp"

#include "util/coordinate_calculation.hpp"
#include "util/guidance/name_announcements.hpp"
#include "util/name_table.hpp"
#include "util/node_based_graph.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include <boost/optional.hpp>

namespace osrm
{
namespace guidance
{

// Intersection handlers deal with all issues related to intersections.
// This base class provides both the interface and implementations for
// common functions.
class IntersectionHandler
{
  public:
    IntersectionHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                        const extractor::EdgeBasedNodeDataContainer &node_data_container,
                        const std::vector<util::Coordinate> &node_coordinates,
                        const extractor::CompressedEdgeContainer &compressed_geometries,
                        const extractor::RestrictionMap &node_restriction_map,
                        const std::unordered_set<NodeID> &barrier_nodes,
                        const extractor::TurnLanesIndexedArray &turn_lanes_data,
                        const util::NameTable &name_table,
                        const extractor::SuffixTable &street_name_suffix_table);

    virtual ~IntersectionHandler() = default;

    // check whether the handler can actually handle the intersection
    virtual bool
    canProcess(const NodeID nid, const EdgeID via_eid, const Intersection &intersection) const = 0;

    // handle and process the intersection
    virtual Intersection
    operator()(const NodeID nid, const EdgeID via_eid, Intersection intersection) const = 0;

  protected:
    const util::NodeBasedDynamicGraph &node_based_graph;
    const extractor::EdgeBasedNodeDataContainer &node_data_container;
    const std::vector<util::Coordinate> &node_coordinates;
    const extractor::CompressedEdgeContainer &compressed_geometries;
    const extractor::RestrictionMap &node_restriction_map;
    const std::unordered_set<NodeID> &barrier_nodes;
    const extractor::TurnLanesIndexedArray &turn_lanes_data;
    const util::NameTable &name_table;
    const extractor::SuffixTable &street_name_suffix_table;
    const extractor::intersection::NodeBasedGraphWalker
        graph_walker; // for skipping traffic signal, distances etc.

    // Decide on a basic turn types
    TurnType::Enum findBasicTurnType(const EdgeID via_edge, const ConnectedRoad &candidate) const;

    TurnType::Enum areSameClasses(const EdgeID via_edge, const ConnectedRoad &road) const;

    // Find the most obvious turn to follow. The function returns an index into the intersection
    // determining whether there is a road that can be seen as obvious turn in the presence of many
    // other possible turns. The function will consider road categories and other inputs like the
    // turn angles.
    template <typename IntersectionType> // works with Intersection and IntersectionView
    std::size_t findObviousTurn(const EdgeID via_edge, const IntersectionType &intersection) const;

    // Obvious turns can still take multiple forms. This function looks at the turn onto a road
    // candidate when coming from a via_edge and determines the best instruction to emit.
    // `through_street` indicates if the street turned onto is a through sreet (think mergees and
    // similar)
    TurnInstruction getInstructionForObvious(const std::size_t number_of_candidates,
                                             const EdgeID via_edge,
                                             const bool through_street,
                                             const ConnectedRoad &candidate) const;

    // Treating potential forks
    void assignFork(const EdgeID via_edge, ConnectedRoad &left, ConnectedRoad &right) const;
    void assignFork(const EdgeID via_edge,
                    ConnectedRoad &left,
                    ConnectedRoad &center,
                    ConnectedRoad &right) const;

    // Trivial Turns use findBasicTurnType and getTurnDirection as only criteria
    void assignTrivialTurns(const EdgeID via_eid,
                            Intersection &intersection,
                            const std::size_t begin,
                            const std::size_t end) const;

    // See `getNextIntersection`
    struct IntersectionViewAndNode final
    {
        extractor::intersection::IntersectionView intersection; // < actual intersection
        NodeID node;                                            // < node at this intersection
    };

    // Skips over artificial intersections i.e. traffic lights, barriers etc.
    // Returns the next non-artificial intersection and its node in the node based
    // graph if an intersection could be found or none otherwise.
    //
    //  a ... tl ... b .. c
    //               .
    //               .
    //               d
    //
    //  ^ at
    //     ^ via
    //
    // For this scenario returns intersection at `b` and `b`.
    boost::optional<IntersectionHandler::IntersectionViewAndNode>
    getNextIntersection(const NodeID at, const EdgeID via) const;

    bool isSameName(const EdgeID source_edge_id, const EdgeID target_edge_id) const;
};

// Impl.

template <typename IntersectionType> // works with Intersection and IntersectionView
std::size_t IntersectionHandler::findObviousTurn(const EdgeID via_edge,
                                                 const IntersectionType &intersection) const
{
    using Road = typename IntersectionType::value_type;
    using osrm::util::angularDeviation;

    // no obvious road
    if (intersection.size() == 1)
        return 0;

    // a single non u-turn is obvious
    if (intersection.size() == 2)
        return 1;

    const auto &in_way_edge = node_based_graph.GetEdgeData(via_edge);
    const auto &in_way_data = node_data_container.GetAnnotation(in_way_edge.annotation_data);

    // the strategy for picking the most obvious turn involves deciding between
    // an overall best candidate and a best candidate that shares the same name
    // as the in road, i.e. a continue road
    std::size_t best_option = 0;
    double best_option_deviation = 180;
    std::size_t best_continue = 0;
    double best_continue_deviation = 180;

    /* helper functions */
    const auto IsContinueRoad = [&](const extractor::NodeBasedEdgeAnnotation &way_data) {
        return !util::guidance::requiresNameAnnounced(
            in_way_data.name_id, way_data.name_id, name_table, street_name_suffix_table);
    };
    auto sameOrHigherPriority = [&](const auto &way_data) {
        return way_data.flags.road_classification.GetPriority() <=
               in_way_edge.flags.road_classification.GetPriority();
    };
    auto IsLowPriority = [](const auto &way_data) {
        return way_data.flags.road_classification.IsLowPriorityRoadClass();
    };
    // These two Compare functions are used for sifting out best option and continue
    // candidates by evaluating all the ways in an intersection by what they share
    // with the in way. Ideal candidates are of similar road class with the in way
    // and are require relatively straight turns.
    const auto RoadCompare = [&](const auto &lhs, const auto &rhs) {
        const auto &lhs_edge = node_based_graph.GetEdgeData(lhs.eid);
        const auto &rhs_edge = node_based_graph.GetEdgeData(rhs.eid);
        const auto lhs_deviation = angularDeviation(lhs.angle, STRAIGHT_ANGLE);
        const auto rhs_deviation = angularDeviation(rhs.angle, STRAIGHT_ANGLE);

        const bool rhs_same_classification =
            rhs_edge.flags.road_classification == in_way_edge.flags.road_classification;
        const bool lhs_same_classification =
            lhs_edge.flags.road_classification == in_way_edge.flags.road_classification;
        const bool rhs_same_or_higher_priority = sameOrHigherPriority(rhs_edge);
        const bool rhs_low_priority = IsLowPriority(rhs_edge);
        const bool lhs_same_or_higher_priority = sameOrHigherPriority(lhs_edge);
        const bool lhs_low_priority = IsLowPriority(lhs_edge);
        auto left_tie = std::tie(lhs.entry_allowed,
                                 lhs_same_or_higher_priority,
                                 rhs_low_priority,
                                 rhs_deviation,
                                 lhs_same_classification);
        auto right_tie = std::tie(rhs.entry_allowed,
                                  rhs_same_or_higher_priority,
                                  lhs_low_priority,
                                  lhs_deviation,
                                  rhs_same_classification);
        return left_tie > right_tie;
    };
    const auto RoadCompareSameName = [&](const auto &lhs, const auto &rhs) {
        const auto &lhs_data = node_data_container.GetAnnotation(
            node_based_graph.GetEdgeData(lhs.eid).annotation_data);
        const auto &rhs_data = node_data_container.GetAnnotation(
            node_based_graph.GetEdgeData(rhs.eid).annotation_data);
        const auto lhs_continues = IsContinueRoad(lhs_data);
        const auto rhs_continues = IsContinueRoad(rhs_data);
        const auto left_tie = std::tie(lhs.entry_allowed, lhs_continues);
        const auto right_tie = std::tie(rhs.entry_allowed, rhs_continues);
        return left_tie > right_tie || (left_tie == right_tie && RoadCompare(lhs, rhs));
    };

    auto best_option_it = std::min_element(begin(intersection), end(intersection), RoadCompare);

    // min element should only return end() when vector is empty
    BOOST_ASSERT(best_option_it != end(intersection));

    best_option = std::distance(begin(intersection), best_option_it);
    best_option_deviation = angularDeviation(intersection[best_option].angle, STRAIGHT_ANGLE);
    const auto &best_option_edge = node_based_graph.GetEdgeData(intersection[best_option].eid);
    const auto &best_option_data =
        node_data_container.GetAnnotation(best_option_edge.annotation_data);

    // Unless the in way is also low priority, it is generally undesirable to
    // indicate that a low priority road is obvious
    if (IsLowPriority(best_option_edge) &&
        best_option_edge.flags.road_classification != in_way_edge.flags.road_classification)
    {
        best_option = 0;
        best_option_deviation = 180;
    }

    // double check if the way with the lowest deviation from straight is still be better choice
    const auto straightest = intersection.findClosestTurn(STRAIGHT_ANGLE);
    if (straightest != best_option_it)
    {
        const auto &straightest_edge = node_based_graph.GetEdgeData(straightest->eid);
        double straightest_data_deviation = angularDeviation(straightest->angle, STRAIGHT_ANGLE);
        const auto deviation_diff =
            std::abs(best_option_deviation - straightest_data_deviation) > FUZZY_ANGLE_DIFFERENCE;
        const auto not_ramp_class = !straightest_edge.flags.road_classification.IsRampClass();
        const auto not_link_class = !straightest_edge.flags.road_classification.IsLinkClass();
        if (deviation_diff && !IsLowPriority(straightest_edge) && not_ramp_class &&
            not_link_class && !IsContinueRoad(best_option_data))
        {
            best_option = std::distance(begin(intersection), straightest);
            best_option_deviation =
                angularDeviation(intersection[best_option].angle, STRAIGHT_ANGLE);
        }
    }

    // No non-low priority roads? Declare no obvious turn
    if (best_option == 0)
        return 0;

    auto best_continue_it =
        std::min_element(begin(intersection), end(intersection), RoadCompareSameName);
    const auto best_continue_edge = node_based_graph.GetEdgeData(best_continue_it->eid);
    const auto best_continue_data =
        node_data_container.GetAnnotation(best_continue_edge.annotation_data);
    if (IsContinueRoad(best_continue_data) ||
        (in_way_data.name_id == EMPTY_NAMEID && best_continue_data.name_id == EMPTY_NAMEID))
    {
        best_continue = std::distance(begin(intersection), best_continue_it);
        best_continue_deviation =
            angularDeviation(intersection[best_continue].angle, STRAIGHT_ANGLE);
    }

    // if the best angle is going straight but the road is turning, declare no obvious turn
    if (0 != best_continue && best_option != best_continue &&
        best_option_deviation < MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
        best_continue_edge.flags.road_classification == best_option_edge.flags.road_classification)
    {
        return 0;
    }

    // get a count of number of ways from that intersection that qualify to have
    // continue instruction because they share a name with the approaching way
    const std::int64_t continue_count =
        count_if(++begin(intersection), end(intersection), [&](const auto &way) {
            return IsContinueRoad(node_data_container.GetAnnotation(
                node_based_graph.GetEdgeData(way.eid).annotation_data));
        });
    const std::int64_t continue_count_valid =
        count_if(++begin(intersection), end(intersection), [&](const auto &way) {
            return IsContinueRoad(node_data_container.GetAnnotation(
                       node_based_graph.GetEdgeData(way.eid).annotation_data)) &&
                   way.entry_allowed;
        });

    // checks if continue candidates are sharp turns
    const bool all_continues_are_narrow = [&]() {
        return std::count_if(begin(intersection), end(intersection), [&](const Road &road) {
                   const auto &road_data = node_data_container.GetAnnotation(
                       node_based_graph.GetEdgeData(road.eid).annotation_data);
                   const double &road_angle = angularDeviation(road.angle, STRAIGHT_ANGLE);
                   return IsContinueRoad(road_data) && (road_angle < NARROW_TURN_ANGLE);
               }) == continue_count;
    }();

    // return true if the best_option candidate is more promising than the best_continue candidate
    // otherwise return false, the best_continue candidate is more promising
    const auto best_over_best_continue = [&]() {
        // no continue road exists
        if (best_continue == 0)
            return true;

        // we have multiple continues and not all are narrow. This suggests that
        // the continue candidates are ambiguous
        if (!all_continues_are_narrow && (continue_count >= 2 && intersection.size() >= 4))
            return true;

        // if the best continue is not narrow and we also have at least 2 possible choices, the
        // intersection size does not matter anymore
        if (continue_count_valid >= 2 && best_continue_deviation >= 2 * NARROW_TURN_ANGLE)
            return true;

        // continue data now most certainly exists
        const auto &continue_edge = node_based_graph.GetEdgeData(intersection[best_continue].eid);

        // best_continue is obvious by road class
        if (obviousByRoadClass(in_way_edge.flags.road_classification,
                               continue_edge.flags.road_classification,
                               best_option_edge.flags.road_classification))
            return false;

        // best_option is obvious by road class
        if (obviousByRoadClass(in_way_edge.flags.road_classification,
                               best_option_edge.flags.road_classification,
                               continue_edge.flags.road_classification))
            return true;

        // the best_option deviation is very straight and not a ramp
        if (best_option_deviation < best_continue_deviation &&
            best_option_deviation < FUZZY_ANGLE_DIFFERENCE &&
            !best_option_edge.flags.road_classification.IsRampClass())
            return true;

        // the continue road is of a lower priority, while the road continues on the same priority
        // with a better angle
        if (best_option_deviation < best_continue_deviation &&
            in_way_edge.flags.road_classification == best_option_edge.flags.road_classification &&
            continue_edge.flags.road_classification.GetPriority() >
                best_option_edge.flags.road_classification.GetPriority())
            return true;

        return false;
    }();

    // check whether we turn onto a oneway through street. These typically happen at the end of
    // roads and might not seem obvious, since it isn't always as visible that you cannot turn
    // left/right. To be on the safe side, we announce these as non-obvious
    const auto turns_onto_through_street = [&](const auto &road) {
        // find edge opposite to the one we are checking (in-road)
        const auto in_through_candidate =
            intersection.FindClosestBearing(util::bearing::reverse(road.perceived_bearing));

        const auto &in_edge = node_based_graph.GetEdgeData(in_through_candidate->eid);
        const auto &out_edge = node_based_graph.GetEdgeData(road.eid);

        // by asking for the same class, we ensure that we do not overrule obvious by road-class
        // decisions
        const auto same_class =
            in_edge.flags.road_classification == out_edge.flags.road_classification;

        // only if the entry is allowed for one of the two, but not the other, we need to check.
        // Otherwise other handlers do it better
        const bool is_oneway = !in_through_candidate->entry_allowed && road.entry_allowed;

        const bool not_roundabout = !(in_edge.flags.roundabout || in_edge.flags.circular ||
                                      out_edge.flags.roundabout || out_edge.flags.circular);

        // for the purpose of this check, we do not care about low-priority roads (parking lots,
        // mostly). Since we postulate both classes to be the same, checking one of the two is
        // enough
        const bool not_low_priority = !in_edge.flags.road_classification.IsLowPriorityRoadClass();

        const auto in_deviation = angularDeviation(in_through_candidate->angle, STRAIGHT_ANGLE);
        const auto out_deviaiton = angularDeviation(road.angle, STRAIGHT_ANGLE);
        // in case the deviation isn't considerably lower for the road we are turning onto,
        // consider it non-obvious. The threshold here requires a slight (60) vs sharp (120)
        // degree variation, at lest (120/60 == 2)
        return is_oneway && same_class && not_roundabout && not_low_priority &&
               (in_deviation / (std::max(out_deviaiton, 0.5)) <= 2);
    };

    if (best_over_best_continue)
    {
        // Find left/right deviation
        // skipping over service roads
        const std::size_t left_index = [&]() {
            const auto index_candidate = (best_option + 1) % intersection.size();
            if (index_candidate == 0)
                return index_candidate;
            const auto &candidate_edge =
                node_based_graph.GetEdgeData(intersection[index_candidate].eid);
            if (obviousByRoadClass(in_way_edge.flags.road_classification,
                                   best_option_edge.flags.road_classification,
                                   candidate_edge.flags.road_classification))
                return (index_candidate + 1) % intersection.size();
            else
                return index_candidate;

        }();
        const auto right_index = [&]() {
            BOOST_ASSERT(best_option > 0);
            const auto index_candidate = best_option - 1;
            if (index_candidate == 0)
                return index_candidate;
            const auto &candidate_edge =
                node_based_graph.GetEdgeData(intersection[index_candidate].eid);
            if (obviousByRoadClass(in_way_edge.flags.road_classification,
                                   best_option_edge.flags.road_classification,
                                   candidate_edge.flags.road_classification))
                return index_candidate - 1;
            else
                return index_candidate;
        }();

        const double left_deviation =
            angularDeviation(intersection[left_index].angle, STRAIGHT_ANGLE);
        const double right_deviation =
            angularDeviation(intersection[right_index].angle, STRAIGHT_ANGLE);

        // return best_option candidate if it is nearly straight and distinct from the nearest other
        // out way
        if (best_option_deviation < MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
            std::min(left_deviation, right_deviation) > FUZZY_ANGLE_DIFFERENCE)
            return best_option;

        const auto &left_edge = node_based_graph.GetEdgeData(intersection[left_index].eid);
        const auto &right_edge = node_based_graph.GetEdgeData(intersection[right_index].eid);

        const bool obvious_to_left =
            left_index == 0 || obviousByRoadClass(in_way_edge.flags.road_classification,
                                                  best_option_edge.flags.road_classification,
                                                  left_edge.flags.road_classification);
        const bool obvious_to_right =
            right_index == 0 || obviousByRoadClass(in_way_edge.flags.road_classification,
                                                   best_option_edge.flags.road_classification,
                                                   right_edge.flags.road_classification);

        // if the best_option turn isn't narrow, but there is a nearly straight turn, we don't
        // consider the turn obvious
        const auto check_narrow = [&intersection, best_option_deviation](const std::size_t index) {
            return angularDeviation(intersection[index].angle, STRAIGHT_ANGLE) <=
                       FUZZY_ANGLE_DIFFERENCE &&
                   (best_option_deviation > NARROW_TURN_ANGLE || intersection[index].entry_allowed);
        };

        // other narrow turns?
        if (check_narrow(right_index) && !obvious_to_right)
            return 0;

        if (check_narrow(left_index) && !obvious_to_left)
            return 0;

        // we are turning onto a through street (possibly at the end of the road). Ensure that we
        // announce a turn, if it isn't a slight merge
        if (turns_onto_through_street(intersection[best_option]))
            return 0;

        // checks if a given way in the intersection is distinct enough from the best_option
        // candidate
        const auto isDistinct = [&](const std::size_t index, const double deviation) {
            /*
               If the neighbor is not possible to enter, we allow for a lower
               distinction rate. If the road category is smaller, its also adjusted. Only
               roads of the same priority require the full distinction ratio.
             */
            const auto &best_option_edge =
                node_based_graph.GetEdgeData(intersection[best_option].eid);
            const auto adjusted_distinction_ratio = [&]() {
                // obviousness by road classes
                if (in_way_edge.flags.road_classification ==
                        best_option_edge.flags.road_classification &&
                    best_option_edge.flags.road_classification.GetPriority() <
                        node_based_graph.GetEdgeData(intersection[index].eid)
                            .flags.road_classification.GetPriority())
                    return 0.8 * DISTINCTION_RATIO;
                // if road classes are the same, we use the full ratio
                else
                    return DISTINCTION_RATIO;
            }();
            return index == 0 || deviation / best_option_deviation >= adjusted_distinction_ratio ||
                   (deviation <= NARROW_TURN_ANGLE && !intersection[index].entry_allowed);
        };

        const bool distinct_to_left = isDistinct(left_index, left_deviation);
        const bool distinct_to_right = isDistinct(right_index, right_deviation);
        // Well distinct turn that is nearly straight
        if ((distinct_to_left || obvious_to_left) && (distinct_to_right || obvious_to_right))
            return best_option;
    }
    else
    {
        const auto &continue_edge = node_based_graph.GetEdgeData(intersection[best_continue].eid);
        const auto &continue_data =
            node_data_container.GetAnnotation(continue_edge.annotation_data);
        if (std::abs(best_continue_deviation) < 1)
            return best_continue;

        // we are turning onto a through street (possibly at the end of the road). Ensure that we
        // announce a turn, if it isn't a slight merge
        if (turns_onto_through_street(intersection[best_continue]))
            return 0;

        // check if any other similar best continues exist
        std::size_t i, last = intersection.size();
        for (i = 1; i < last; ++i)
        {
            if (i == best_continue || !intersection[i].entry_allowed)
                continue;

            const auto &turn_edge = node_based_graph.GetEdgeData(intersection[i].eid);
            const auto &turn_data = node_data_container.GetAnnotation(turn_edge.annotation_data);
            const bool is_obvious_by_road_class =
                obviousByRoadClass(in_way_edge.flags.road_classification,
                                   continue_edge.flags.road_classification,
                                   turn_edge.flags.road_classification);

            // if the main road is obvious by class, we ignore the current road as a potential
            // prevention of obviousness
            if (is_obvious_by_road_class)
                continue;

            // continuation could be grouped with a straight turn and the turning road is a ramp
            if (turn_edge.flags.road_classification.IsRampClass() &&
                best_continue_deviation < GROUP_ANGLE &&
                !continue_edge.flags.road_classification.IsRampClass())
                continue;

            // perfectly straight turns prevent obviousness
            const auto turn_deviation = angularDeviation(intersection[i].angle, STRAIGHT_ANGLE);
            if (turn_deviation < FUZZY_ANGLE_DIFFERENCE)
                return 0;

            const auto deviation_ratio = turn_deviation / best_continue_deviation;

            // in comparison to normal deviations, a continue road can offer a smaller distinction
            // ratio. Other roads close to the turn angle are not as obvious, if one road continues.
            if (deviation_ratio < DISTINCTION_RATIO / 1.5)
                return 0;

            /* in comparison to another continuing road, we need a better distinction. This prevents
               situations where the turn is probably less obvious. An example are places that have a
               road with the same name entering/exiting:

                       d
                      /
                     /
               a -- b
                     \
                      \
                       c
            */

            const auto same_name = !util::guidance::requiresNameAnnounced(
                turn_data.name_id, continue_data.name_id, name_table, street_name_suffix_table);

            if (same_name && deviation_ratio < 1.5 * DISTINCTION_RATIO)
                return 0;
        }

        // Segregated intersections can result in us finding an obvious turn, even though its only
        // obvious due to a very short segment in between. So if the segment coming in is very
        // short, we check the previous intersection for other continues in the opposite bearing.
        const auto node_at_intersection = node_based_graph.GetTarget(via_edge);

        const double constexpr MAX_COLLAPSE_DISTANCE = 30;
        const auto distance_at_u_turn = intersection[0].segment_length;
        if (distance_at_u_turn < MAX_COLLAPSE_DISTANCE)
        {
            // this request here actually goes against the direction of the ingoing edgeid. This can
            // even reverse the direction. Since we don't want to compute actual turns but simply
            // try to find whether there is a turn going to the opposite direction of our obvious
            // turn, this should be alright.
            const auto previous_intersection = [&]() -> extractor::intersection::IntersectionView {
                const auto parameters = extractor::intersection::skipDegreeTwoNodes(
                    node_based_graph, {node_at_intersection, intersection[0].eid});
                if (node_based_graph.GetTarget(parameters.edge) == node_at_intersection)
                    return {};

                return extractor::intersection::getConnectedRoads<false>(node_based_graph,
                                                                         node_data_container,
                                                                         node_coordinates,
                                                                         compressed_geometries,
                                                                         node_restriction_map,
                                                                         barrier_nodes,
                                                                         turn_lanes_data,
                                                                         parameters);
            }();

            if (!previous_intersection.empty())
            {
                const auto continue_road = intersection[best_continue];
                for (const auto &comparison_road : previous_intersection)
                {
                    // since we look at the intersection in the wrong direction, a similar angle
                    // actually represents a near 180 degree different in bearings between the two
                    // roads. So if there is a road that is enterable in the opposite direction just
                    // prior, a turn is not obvious
                    const auto &turn_edge_data = node_based_graph.GetEdgeData(comparison_road.eid);
                    const auto &turn_data =
                        node_data_container.GetAnnotation(turn_edge_data.annotation_data);
                    if (angularDeviation(comparison_road.angle, STRAIGHT_ANGLE) > GROUP_ANGLE &&
                        angularDeviation(comparison_road.angle, continue_road.angle) <
                            FUZZY_ANGLE_DIFFERENCE &&
                        !turn_edge_data.reversed && continue_data.CanCombineWith(turn_data))
                        return 0;
                }
            }
        }

        return best_continue;
    }

    return 0;
}

} // namespace guidance
} // namespace osrm

#endif /*OSRM_GUIDANCE_INTERSECTION_HANDLER_HPP_*/
