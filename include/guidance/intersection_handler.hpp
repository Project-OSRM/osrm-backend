#ifndef OSRM_GUIDANCE_INTERSECTION_HANDLER_HPP_
#define OSRM_GUIDANCE_INTERSECTION_HANDLER_HPP_

#include "extractor/intersection/intersection_analysis.hpp"
#include "extractor/intersection/node_based_graph_walker.hpp"
#include "extractor/name_table.hpp"
#include "extractor/suffix_table.hpp"
#include "guidance/constants.hpp"
#include "guidance/intersection.hpp"

#include "util/assert.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/guidance/name_announcements.hpp"
#include "util/node_based_graph.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace osrm::guidance
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
                        const extractor::NameTable &name_table,
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
    const extractor::NameTable &name_table;
    const extractor::SuffixTable &street_name_suffix_table;
    const extractor::intersection::NodeBasedGraphWalker
        graph_walker; // for skipping traffic signal, distances etc.

    // Decide on a basic turn types
    TurnType::Enum findBasicTurnType(const EdgeID via_edge, const ConnectedRoad &candidate) const;

    TurnType::Enum areSameClasses(const EdgeID via_edge, const ConnectedRoad &road) const;

    template <typename IntersectionType>
    inline bool IsDistinctNarrowTurn(const EdgeID via_edge,
                                     const typename IntersectionType::const_iterator candidate,
                                     const IntersectionType &intersection) const;
    template <typename IntersectionType>
    inline bool IsDistinctWideTurn(const EdgeID via_edge,
                                   const typename IntersectionType::const_iterator candidate,
                                   const IntersectionType &intersection) const;
    template <typename IntersectionType>
    inline bool IsDistinctTurn(const EdgeID via_edge,
                               const typename IntersectionType::const_iterator candidate,
                               const IntersectionType &intersection) const;

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
    std::optional<IntersectionHandler::IntersectionViewAndNode>
    getNextIntersection(const NodeID at, const EdgeID via) const;

    bool isSameName(const EdgeID source_edge_id, const EdgeID target_edge_id) const;
};

// Implementation

namespace
{

inline bool roadHasLowerClass(const util::NodeBasedEdgeData &from_data,
                              const util::NodeBasedEdgeData &to_data,
                              const util::NodeBasedEdgeData &compare_data)
{
    // Check if a road has a strictly lower category
    const auto from_classification = from_data.flags.road_classification;
    const auto to_classification = to_data.flags.road_classification;
    const auto compare_classification = compare_data.flags.road_classification;
    const auto from_lanes_number = from_classification.GetNumberOfLanes();
    const auto compare_lanes_number = compare_classification.GetNumberOfLanes();

    // 1) if the road has strictly less classification than the incoming candidate roads
    //    and has smaller number of lanes
    const auto lanes_number_reduced =
        compare_lanes_number > 0 && compare_lanes_number + 1 < from_lanes_number;
    if (strictlyLess(compare_classification, from_classification) &&
        strictlyLess(compare_classification, to_classification) && lanes_number_reduced)
    {
        return true;
    }

    // 2) if a link of the same category
    if (isLinkTo(compare_classification, from_classification) &&
        isLinkTo(compare_classification, to_classification))
    {
        return true;
    }

    return false;
}
} // namespace

template <typename IntersectionType> // works with Intersection and IntersectionView
inline bool
IntersectionHandler::IsDistinctNarrowTurn(const EdgeID via_edge,
                                          const typename IntersectionType::const_iterator candidate,
                                          const IntersectionType &intersection) const
{
    const auto &via_edge_data = node_based_graph.GetEdgeData(via_edge);
    const auto &via_edge_annotation =
        node_data_container.GetAnnotation(via_edge_data.annotation_data);

    const auto &candidate_data = node_based_graph.GetEdgeData(candidate->eid);
    const auto &candidate_annotation =
        node_data_container.GetAnnotation(candidate_data.annotation_data);
    auto const candidate_deviation = util::angularDeviation(candidate->angle, STRAIGHT_ANGLE);

    auto const num_lanes = [](auto const &data)
    { return data.flags.road_classification.GetNumberOfLanes(); };

    auto const lanes_number_equal = [&](auto const &compare_data)
    {
        // Check if the lanes number is the same going from the inbound edge to the compare road
        return num_lanes(compare_data) > 0 && num_lanes(compare_data) == num_lanes(via_edge_data);
    };

    // In case of narrow turns, we apply different criteria than for actual turns. In case of a
    // narrow turn, having two choices one of which is forbidden is fine. In case of a end of
    // the road turn, having two directions and not being allowed to turn onto one of them isn't
    // always as clear

    // check if the candidate road changes it's name
    auto const no_name_change_to_candidate =
        !util::guidance::requiresNameAnnounced(via_edge_annotation.name_id,
                                               candidate_annotation.name_id,
                                               name_table,
                                               street_name_suffix_table);

    // check if there are other narrow turns are not considered passing a low category or simply
    // a link of the same type as the potentially obvious turn
    auto const is_similar_turn = [&](auto const &road)
    {
        // 1. Skip the candidate road
        if (road.eid == candidate->eid)
        {
            return false;
        }

        // 2. For candidates with narrow turns don't consider not allowed entries
        if (candidate_deviation < NARROW_TURN_ANGLE && !road.entry_allowed)
        {
            return false;
        }

        auto const compare_deviation = util::angularDeviation(road.angle, STRAIGHT_ANGLE);
        auto const &compare_data = node_based_graph.GetEdgeData(road.eid);
        auto const &compare_annotation =
            node_data_container.GetAnnotation(compare_data.annotation_data);

        auto const is_lane_fork =
            num_lanes(compare_data) > 0 && num_lanes(candidate_data) == num_lanes(compare_data) &&
            num_lanes(via_edge_data) == num_lanes(candidate_data) + num_lanes(compare_data) &&
            util::angularDeviation(candidate->angle, road.angle) < GROUP_ANGLE;

        auto const compare_road_deviation_is_distinct =
            compare_deviation > DISTINCTION_RATIO * candidate_deviation &&
            std::abs(compare_deviation - candidate_deviation) > FUZZY_ANGLE_DIFFERENCE / 2.;

        const auto compare_road_deviation_is_slightly_distinct =
            compare_deviation > 0.7 * DISTINCTION_RATIO * candidate_deviation;

        // 3. Small side-roads that are marked restricted are not similar to unrestricted roads
        if (!via_edge_data.flags.restricted && !candidate_data.flags.restricted &&
            compare_data.flags.restricted && compare_road_deviation_is_distinct)
        {
            return false;
        }

        // 4. Roundabout exits with larger deviations wrt candidate roads are not similar
        if (via_edge_data.flags.roundabout == candidate_data.flags.roundabout &&
            via_edge_data.flags.roundabout != compare_data.flags.roundabout &&
            candidate_deviation < compare_deviation)
        {
            return false;
        }

        // 5. Similarity check based on name changes
        auto const name_changes_to_compare =
            util::guidance::requiresNameAnnounced(via_edge_annotation.name_id,
                                                  compare_annotation.name_id,
                                                  name_table,
                                                  street_name_suffix_table);

        if ((no_name_change_to_candidate || name_changes_to_compare) && !is_lane_fork &&
            compare_road_deviation_is_distinct)
        {
            return false;
        }

        // 6. If the road has a continuation on the opposite side of intersection
        // it can not be similar to the candidate road
        auto const opposing_turn =
            intersection.FindClosestBearing(util::bearing::reverse(road.perceived_bearing));
        auto const &opposing_data = node_based_graph.GetEdgeData(opposing_turn->eid);
        auto const &opposing_annotation =
            node_data_container.GetAnnotation(opposing_data.annotation_data);

        auto const four_or_more_ways_intersection = intersection.size() >= 4;
        auto const no_name_change_to_compare_from_opposing =
            !util::guidance::requiresNameAnnounced(opposing_annotation.name_id,
                                                   compare_annotation.name_id,
                                                   name_table,
                                                   street_name_suffix_table);

        const auto opposing_to_compare_angle =
            util::angularDeviation(road.angle, opposing_turn->angle);

        auto const opposing_to_compare_road_is_distinct =
            no_name_change_to_compare_from_opposing ||
            opposing_to_compare_angle > (STRAIGHT_ANGLE - NARROW_TURN_ANGLE);

        if (four_or_more_ways_intersection && opposing_to_compare_road_is_distinct &&
            compare_road_deviation_is_distinct)
        {
            return false;
        }

        if (four_or_more_ways_intersection && no_name_change_to_candidate &&
            name_changes_to_compare && compare_road_deviation_is_slightly_distinct &&
            no_name_change_to_compare_from_opposing &&
            opposing_to_compare_angle > STRAIGHT_ANGLE - FUZZY_ANGLE_DIFFERENCE)
        {
            return false;
        }

        if (!four_or_more_ways_intersection && no_name_change_to_candidate &&
            name_changes_to_compare && compare_road_deviation_is_distinct && !is_lane_fork &&
            opposing_to_compare_angle > STRAIGHT_ANGLE - GROUP_ANGLE)
        {
            return false;
        }

        // 7. If the inbound road has low priority, consider all distinct roads as non-similar
        auto const from_non_main_road_class =
            via_edge_data.flags.road_classification.GetPriority() >
            extractor::RoadPriorityClass::SECONDARY;

        if (from_non_main_road_class && compare_road_deviation_is_distinct)
        {
            return false;
        }

        // 8. Consider roads non-similar if the candidate road has the same number
        // of lanes and has quite small deviation from straightforward direction
        //     a=a=a + b=b=b
        //            ` c-c
        if (lanes_number_equal(candidate_data) && candidate_deviation < FUZZY_ANGLE_DIFFERENCE &&
            compare_road_deviation_is_distinct)
        {
            return false;
        }

        // 9. Priority checks
        const auto same_priority_to_candidate =
            via_edge_data.flags.road_classification.GetPriority() ==
            candidate_data.flags.road_classification.GetPriority();

        const auto compare_has_lower_class =
            candidate_data.flags.road_classification.GetPriority() <
            compare_data.flags.road_classification.GetPriority();

        const auto compare_has_higher_class =
            candidate_data.flags.road_classification.GetPriority() >
            compare_data.flags.road_classification.GetPriority() + 4;

        if (same_priority_to_candidate && compare_has_lower_class && no_name_change_to_candidate &&
            compare_road_deviation_is_slightly_distinct)
        {
            return false;
        }

        if (same_priority_to_candidate && compare_has_higher_class && no_name_change_to_candidate &&
            compare_road_deviation_is_slightly_distinct)
        {
            return false;
        }

        if (roadHasLowerClass(via_edge_data, candidate_data, compare_data))
        {
            return false;
        }

        const auto candidate_road_has_same_priority_group =
            via_edge_data.flags.road_classification.GetPriority() ==
            candidate_data.flags.road_classification.GetPriority();
        const auto compare_road_has_lower_priority_group =
            extractor::getRoadGroup(via_edge_data.flags.road_classification) <
            extractor::getRoadGroup(compare_data.flags.road_classification);
        auto const candidate_and_compare_have_different_names =
            util::guidance::requiresNameAnnounced(candidate_annotation.name_id,
                                                  compare_annotation.name_id,
                                                  name_table,
                                                  street_name_suffix_table);

        if (candidate_road_has_same_priority_group && compare_road_has_lower_priority_group &&
            candidate_and_compare_have_different_names && name_changes_to_compare)
        {
            return false;
        }

        if (candidate_road_has_same_priority_group &&
            compare_data.flags.road_classification.IsLinkClass())
        {
            return false;
        }

        return true;
    };

    return std::find_if(intersection.begin() + 1, intersection.end(), is_similar_turn) ==
           intersection.end();
}

template <typename IntersectionType>
inline bool
IntersectionHandler::IsDistinctWideTurn(const EdgeID via_edge,
                                        const typename IntersectionType::const_iterator candidate,
                                        const IntersectionType &intersection) const
{
    const auto &via_edge_data = node_based_graph.GetEdgeData(via_edge);
    const auto &candidate_data = node_based_graph.GetEdgeData(candidate->eid);
    auto const candidate_deviation = util::angularDeviation(candidate->angle, STRAIGHT_ANGLE);

    // Deviation is larger than NARROW_TURN_ANGLE0 here for the candidate
    // check if there is any turn, that might look just as obvious, even though it might not
    // be allowed. Entry-allowed isn't considered a valid distinction criterion here
    auto const is_similar_turn = [&](auto const &road)
    {
        // 1. Skip over our candidate
        if (road.eid == candidate->eid)
            return false;

        // we do not consider roads of far lesser category to be more obvious
        const auto &compare_data = node_based_graph.GetEdgeData(road.eid);
        const auto compare_deviation = util::angularDeviation(road.angle, STRAIGHT_ANGLE);
        const auto is_compare_straight =
            getTurnDirection(road.angle) == DirectionModifier::Straight;

        // 2. Don't consider similarity if a compare road is non-straight and has lower class
        if (!is_compare_straight && roadHasLowerClass(via_edge_data, candidate_data, compare_data))
        {
            return false;
        }

        // 3. If the turn is much stronger, we are also fine (note that we do not have to check
        // absolutes, since candidate is at least > NARROW_TURN_ANGLE)
        auto const compare_road_deviation_is_distinct =
            compare_deviation > DISTINCTION_RATIO * candidate_deviation;

        if (compare_road_deviation_is_distinct)
        {
            return false;
        }

        // 4. If initial and adjusted bearings are quite different then check deviations
        // computed in the vicinity of the intersection point based in initial bearings:
        // road is not similar to candidate if a road-to-candidate is not a straight direction
        // and road has distinctive deviation.
        if (util::angularDeviation(intersection[0].initial_bearing,
                                   intersection[0].perceived_bearing) > FUZZY_ANGLE_DIFFERENCE)
        {
            using osrm::util::bearing::reverse;
            using osrm::util::bearing::angleBetween;
            using osrm::util::angularDeviation;

            const auto via_edge_initial_bearing = reverse(intersection[0].initial_bearing);
            const auto candidate_deviation_initial = angularDeviation(
                angleBetween(via_edge_initial_bearing, candidate->initial_bearing), STRAIGHT_ANGLE);
            const auto road_deviation_initial = angularDeviation(
                angleBetween(via_edge_initial_bearing, road.initial_bearing), STRAIGHT_ANGLE);
            const auto road_to_candidate_angle =
                angleBetween(reverse(road.initial_bearing), candidate->initial_bearing);
            const auto is_straight_road_to_candidate =
                getTurnDirection(road_to_candidate_angle) == DirectionModifier::Straight;

            if (!is_straight_road_to_candidate &&
                road_deviation_initial > DISTINCTION_RATIO * candidate_deviation_initial)
            {
                return false;
            }
        }

        return true;
    };

    return std::find_if(intersection.begin() + 1, intersection.end(), is_similar_turn) ==
           intersection.end();
}

template <typename IntersectionType>
inline bool
IntersectionHandler::IsDistinctTurn(const EdgeID via_edge,
                                    const typename IntersectionType::const_iterator candidate,
                                    const IntersectionType &intersection) const
{
    auto const candidate_deviation = util::angularDeviation(candidate->angle, STRAIGHT_ANGLE);

    if (candidate_deviation < GROUP_ANGLE)
    {
        return IsDistinctNarrowTurn(via_edge, candidate, intersection);
    }

    return IsDistinctWideTurn(via_edge, candidate, intersection);
}

// Impl.
template <typename IntersectionType> // works with Intersection and IntersectionView
std::size_t IntersectionHandler::findObviousTurn(const EdgeID via_edge,
                                                 const IntersectionType &intersection) const
{

    // no obvious road
    if (intersection.size() == 1)
        return 0;

    // a single non u-turn is obvious
    if (intersection.size() == 2)
        return 1;

    // the way we are coming from
    auto const &via_edge_data = node_based_graph.GetEdgeData(via_edge);
    auto const &via_edge_annotation =
        node_data_container.GetAnnotation(via_edge_data.annotation_data);

    // implement a filter, taking out all roads of lower class or different names
    auto const continues_on_name_with_higher_class = [&](auto const &road)
    {
        // it needs to be possible to enter the road
        if (!road.entry_allowed)
            return true;

        // to continue on a name, we need to have one first
        if (via_edge_annotation.name_id == EMPTY_NAMEID &&
            !via_edge_data.flags.road_classification.IsLowPriorityRoadClass())
            return true;

        // and we cannot yloose it (roads loosing their name will be handled after this check
        // here)
        auto const &road_data = node_based_graph.GetEdgeData(road.eid);
        const auto &road_annotation = node_data_container.GetAnnotation(road_data.annotation_data);
        if (road_annotation.name_id == EMPTY_NAMEID &&
            !road_data.flags.road_classification.IsLowPriorityRoadClass())
            return true;

        // if not both of the entries are empty, we do not consider this a continue
        if ((via_edge_annotation.name_id == EMPTY_NAMEID) ^
            (road_annotation.name_id == EMPTY_NAMEID))
            return true;

        // the priority can only stay the same or increase. We don't consider a
        // primary->residential
        // or residential->service as a continuing road
        if (strictlyLess(road_data.flags.road_classification,
                         via_edge_data.flags.road_classification))
            return true;

        // filter out link classes to our current class, since they should only be connectivity
        if (isLinkTo(road_data.flags.road_classification, via_edge_data.flags.road_classification))
            return true;

        // most expensive check last (since we filter, we check whether the name changes
        return util::guidance::requiresNameAnnounced(via_edge_annotation.name_id,
                                                     road_annotation.name_id,
                                                     name_table,
                                                     street_name_suffix_table);
    };

    // check if the current road continues at a given index
    auto const road_continues_itr =
        intersection.findClosestTurn(STRAIGHT_ANGLE, continues_on_name_with_higher_class);

    // this check is not part of the main conditions, so that if the turn looks obvious from all
    // other perspectives, a mode change will not result in different classification
    auto const to_index_if_valid = [&](auto const iterator) -> std::size_t
    {
        auto const &from_data = node_based_graph.GetEdgeData(via_edge);
        auto const &to_data = node_based_graph.GetEdgeData(iterator->eid);

        if (from_data.flags.roundabout != to_data.flags.roundabout)
            return 0;

        return std::distance(intersection.begin(), iterator);
    };

    // in case the continuing road is distinct, we prefer continuing on the current road.
    // Only if continue does not exist or we are not distinct, we look for other possible candidates
    if (road_continues_itr != intersection.end() &&
        IsDistinctTurn(via_edge, road_continues_itr, intersection))
    {
        return to_index_if_valid(road_continues_itr);
    }

    // The road doesn't continue in an obvious fashion. At least we see the start of a new road
    // here, which might be more obvious than (for example) a turning road of the same name. The
    // next goal is to find a road which is going more or less straight, but is also a matching
    // category. So if we are on a primary that has an alley right ahead, the alley will not
    // quality. But if primary goes straight onto secondary / turns left into primary. We would
    // consider the secondary a candidate.

    // opposed to before, we do not care about name changes, again: this is a filter, so internal
    // false/true will be negated for selection
    auto const valid_of_higher_or_same_category = [&](auto const &road)
    {
        if (!road.entry_allowed)
            return true;

        auto const &road_data = node_based_graph.GetEdgeData(road.eid);
        if (strictlyLess(road_data.flags.road_classification,
                         via_edge_data.flags.road_classification))
            return true;

        if (isLinkTo(road_data.flags.road_classification, via_edge_data.flags.road_classification))
            return true;

        return false;
    };

    // check for roads that allow entry only
    auto const straightmost_turn_itr =
        intersection.findClosestTurn(STRAIGHT_ANGLE, valid_of_higher_or_same_category);

    if (straightmost_turn_itr != intersection.end() &&
        IsDistinctTurn(via_edge, straightmost_turn_itr, intersection))
    {
        return to_index_if_valid(straightmost_turn_itr);
    }

    // we cannot find a turn of same or higher priority, so we check if any straightmost turn could
    // be obvious. We only consider somewhat narrow turns for these cases though
    auto const straightmost_valid = intersection.findClosestTurn(
        STRAIGHT_ANGLE, [&](auto const &road) { return !road.entry_allowed; });
    // no valid turns
    if (straightmost_valid == intersection.end())
        return 0;

    auto const non_sharp_turns = intersection.Count(
        [&](auto const &road) { return util::angularDeviation(road.angle, STRAIGHT_ANGLE) <= 90; });
    auto const straight_is_only_non_sharp =
        (util::angularDeviation(straightmost_valid->angle, STRAIGHT_ANGLE) <= 90) &&
        (non_sharp_turns == 1);

    if ((straightmost_valid != straightmost_turn_itr) &&
        (util::angularDeviation(STRAIGHT_ANGLE, straightmost_valid->angle) <= GROUP_ANGLE ||
         straight_is_only_non_sharp) &&
        !node_based_graph.GetEdgeData(straightmost_valid->eid)
             .flags.road_classification.IsLowPriorityRoadClass() &&
        IsDistinctTurn(via_edge, straightmost_valid, intersection))
    {
        return to_index_if_valid(straightmost_valid);
    }

    // special case handling for motorways, for which nearly narrow / only allowed turns are
    // always obvious
    if (node_based_graph.GetEdgeData(straightmost_valid->eid)
            .flags.road_classification.IsMotorwayClass() &&
        util::angularDeviation(straightmost_valid->angle, STRAIGHT_ANGLE) <= GROUP_ANGLE &&
        intersection.countEnterable() == 1)
    {
        return to_index_if_valid(straightmost_valid);
    }

    // Special case handling for roads splitting up, all the same name (exactly the same)
    const auto all_roads_have_same_name =
        std::all_of(intersection.begin(),
                    intersection.end(),
                    [id = via_edge_annotation.name_id, this](auto const &road)
                    {
                        auto const data_id = node_based_graph.GetEdgeData(road.eid).annotation_data;
                        auto const name_id = node_data_container.GetAnnotation(data_id).name_id;
                        return (name_id != EMPTY_NAMEID) && (name_id == id);
                    });

    if (intersection.size() == 3 && all_roads_have_same_name &&
        intersection.countEnterable() == 1 &&
        // ensure that we do not lookt at a end of the road turn in a segregated intersection
        (util::angularDeviation(intersection[1].angle, 90) > NARROW_TURN_ANGLE ||
         util::angularDeviation(intersection[2].angle, 270) > NARROW_TURN_ANGLE))
    {
        return to_index_if_valid(straightmost_valid);
    }

    return 0;
}

} // namespace osrm::guidance

#endif /*OSRM_GUIDANCE_INTERSECTION_HANDLER_HPP_*/
