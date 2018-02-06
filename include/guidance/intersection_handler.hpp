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

    template <typename IntersectionType> // works with Intersection and IntersectionView
    inline bool IsDistinctTurn(const std::size_t index,
                               const EdgeID via_edge,
                               const IntersectionType &intersection) const;

    template <typename IntersectionType> // works with Intersection and IntersectionView
    inline bool IsDistinctContinue(const std::size_t index,
                                   const EdgeID via_edge,
                                   const IntersectionType &intersection) const;

    // Find the most obvious turn to follow. The function returns an index into the intersection
    // determining whether there is a road that can be seen as obvious turn in the presence of many
    // other possible turns. The function will consider road categories and other inputs like the
    // turn angles.
    template <typename IntersectionType> // works with Intersection and IntersectionView
    std::size_t findObviousTurn(const EdgeID via_edge, const IntersectionType &intersection) const;

    template <typename IntersectionType> // works with Intersection and IntersectionView
    std::size_t findObviousTurnOld(const EdgeID via_edge,
                                   const IntersectionType &intersection) const;

    template <typename IntersectionType> // works with Intersection and IntersectionView
    std::size_t findObviousTurnNew(const EdgeID via_edge,
                                   const IntersectionType &intersection) const;

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
using osrm::extractor::getRoadGroup;

template <typename IntersectionType> // works with Intersection and IntersectionView
inline bool IntersectionHandler::IsDistinctTurn(const std::size_t index,
                                                const EdgeID via_edge,
                                                const IntersectionType &intersection) const
{
    // for comparing road categories
    const auto &via_edge_data = node_based_graph.GetEdgeData(via_edge);
    const auto &candidate = intersection[index];
    const auto &candidate_data = node_based_graph.GetEdgeData(candidate.eid);

    auto const num_lanes = [](auto const &data) {
        return data.flags.road_classification.GetNumberOfLanes();
    };

    auto const override_class_by_lanes = [&](auto const &compare_data) {
        // sometimes roads of same size are tagged strangely within a neighborhood, combining
        // primary roads with residential roads. If the road with can be deducted from lanes, we
        // can override such a classification
        if (num_lanes(compare_data) > 0 && num_lanes(via_edge_data) > 0)
        {
            // check if via-edge has more than one additional lane, relative to the compare data
            if (num_lanes(via_edge_data) - num_lanes(compare_data) > 1)
                return true;
        }
        return false;
    };

    // check if a road is distinct to the obvious turn candidate in its road class. This is the case
    // only if we pass by a lower road category class or a link to the same category
    auto const distinct_by_class = [&](auto const &road) {
        auto const &compare_data = node_based_graph.GetEdgeData(road.eid);

        // passing a road of a stricly lower category (e.g. residential driving past driveway,
        // primary road passing a residential road) but also exiting a freeway onto a primary in the
        // presence of an alley
        if (strictlyLess(compare_data.flags.road_classification,
                         via_edge_data.flags.road_classification) &&
            strictlyLess(compare_data.flags.road_classification,
                         candidate_data.flags.road_classification) &&
            override_class_by_lanes(compare_data))
        {
            return true;
        }

        // passing by a link of the same category
        if (isLinkTo(compare_data.flags.road_classification,
                     via_edge_data.flags.road_classification) &&
            isLinkTo(compare_data.flags.road_classification,
                     candidate_data.flags.road_classification))
            return true;

        // staying on the same road class, encountering a road that is a severe change in class
        // (residential-> motorway_link) is considered a fair distinction
        if (compare_data.flags.road_classification.IsLinkClass() &&
            (via_edge_data.flags.road_classification.GetPriority() ==
             candidate_data.flags.road_classification.GetPriority()) &&
            (std::abs(static_cast<int>(getRoadGroup(via_edge_data.flags.road_classification)) -
                      static_cast<int>(getRoadGroup(compare_data.flags.road_classification))) >
             4) &&
            override_class_by_lanes(compare_data))
        {
            return true;
        }

        return false;
    };

    // in case of narrow turns, we apply different criteria than for actual turns. In case of a
    // narrow turn, having two choices one of which is forbidden is fine. In case of a end of the
    // road turn, having two directions and not being allowed to turn onto one of them isn't always
    // as clear
    auto const candidate_deviation = util::angularDeviation(candidate.angle, STRAIGHT_ANGLE);

    const auto &via_edge_annotation =
        node_data_container.GetAnnotation(via_edge_data.annotation_data);
    const auto &candidate_annotation =
        node_data_container.GetAnnotation(candidate_data.annotation_data);

    const auto constexpr max_narrow_deviation = GROUP_ANGLE;
    // on cases where the candidate deviation is in a narrow range, we can consider the deviaiton of
    // other turns as a distinction criteria
    //
    //             c
    //           *
    //         *
    //        b - d
    //        |
    //        a
    // for example can be considered obvious as goig straight, while
    //
    //             c
    //  d        *
    //     *   *
    //        b
    //        |
    //        a
    // should err on the side of caution (when only comparing deviations)

    if (candidate_deviation <= max_narrow_deviation)
    {
        // check if the candidate changes it's name
        auto const candidate_changes_name =
            util::guidance::requiresNameAnnounced(via_edge_annotation.name_id,
                                                  candidate_annotation.name_id,
                                                  name_table,
                                                  street_name_suffix_table);

        // check if there are other narrow turns are not considered passing a low category or simply
        // a link of the same type as the potentially obvious turn
        auto const is_similar_turn = [&](auto const &road) {
            // skip over our candidate
            if (road.eid == candidate.eid)
                return false;

            // since we have a narrow turn, we only care for roads allowing entry
            if (candidate_deviation < NARROW_TURN_ANGLE && !road.entry_allowed)
            {
                return false;
            }

            // detect link roads in segregated intersections
            if (!road.entry_allowed && (intersection.size() == 5) &&
                (std::count_if(intersection.begin(), intersection.end(), [](auto const &road) {
                     return road.entry_allowed;
                 }) <= 2))
            {
                // if we are on a link road and all other turns form a 4 way intersection, the
                // angular differences of all other turns need to be near 90 degrees
                bool all_close_to_90 = true;
                for (std::size_t i = 1; i < 3; ++i)
                {
                    auto const deviation =
                        util::angularDeviation(intersection[i].angle, intersection[i + 1].angle);
                    if (deviation < 75 || deviation > 105)
                    {
                        all_close_to_90 = false;
                        break;
                    }
                }
                if (all_close_to_90)
                {
                    return false;
                }
            }

            auto const compare_deviation = util::angularDeviation(road.angle, STRAIGHT_ANGLE);
            auto const &compare_data = node_based_graph.GetEdgeData(road.eid);
            auto const &compare_annotation =
                node_data_container.GetAnnotation(compare_data.annotation_data);

            // in the states, many small side-roads are marked restricted. We could consider them
            // driveways. Passing by one of these should always be obvious
            if (candidate_deviation < NARROW_TURN_ANGLE &&
                (compare_deviation > 1.5 * candidate_deviation) && compare_data.flags.restricted &&
                !via_edge_data.flags.restricted && !candidate_data.flags.restricted)
            {
                return false;
            }

            // if we see a roundabout that is a larger turn, we do not consider it similar. This is
            // related to throughabouts which often are slightly curved on exits:
            //              |
            // - a          d -
            //    \` e f ` /
            //     b - - c
            if (compare_data.flags.roundabout != via_edge_data.flags.roundabout &&
                via_edge_data.flags.roundabout == candidate_data.flags.roundabout &&
                candidate_deviation < compare_deviation)
                return false;

            // to find whether a continuing road is turning, we need to check if it is an actual
            // turn, a segregated intersection

            auto const opposing_turn =
                intersection.FindClosestBearing(util::bearing::reverse(road.perceived_bearing));
            auto const opposing_data = node_based_graph.GetEdgeData(opposing_turn->eid);
            // Check for a situation like:
            //
            //     a         a
            //     a         a
            // a a + b b     + b b
            //     c        ac
            //     c      a  c
            //
            // opposed to
            //
            //     a
            //     a
            // a a + b b
            //     a
            //     a
            auto const name_changes_onto_compare =
                util::guidance::requiresNameAnnounced(via_edge_annotation.name_id,
                                                      compare_annotation.name_id,
                                                      name_table,
                                                      street_name_suffix_table);
            auto const opposing_name =
                node_data_container.GetAnnotation(opposing_data.annotation_data).name_id;
            auto const name_changes_onto_compare_from_opposing =
                util::guidance::requiresNameAnnounced(opposing_name,
                                                      compare_annotation.name_id,
                                                      name_table,
                                                      street_name_suffix_table);

            // check if the continuing road takes a turn, and we are turning off it. This is
            // required, sicne we could end up announcing `follow X for 2 miles` and if `X` turns,
            // we would be inclined to do the turn as well, if it isn't crazy (like a sharp turn)
            auto const continue_turns = (via_edge_annotation.name_id != EMPTY_NAMEID) &&
                                        !name_changes_onto_compare &&
                                        (util::angularDeviation(road.angle, opposing_turn->angle) <
                                             (STRAIGHT_ANGLE - NARROW_TURN_ANGLE) &&
                                         name_changes_onto_compare_from_opposing) &&
                                        util::angularDeviation(road.angle, 0) > NARROW_TURN_ANGLE;

            auto const continuing_road_takes_a_turn = candidate_changes_name && continue_turns;
            // at least a relative and a maximum difference, if the road name does not turn.
            // Since we can announce `stay on X for 2 miles, we need to ensure that we announce
            // turns off it (even if straight). Otherwise people might follow X further than they
            // should
            // For roads splitting with the same name, we ask for a larger difference.
            auto const minimum_angle_difference = FUZZY_ANGLE_DIFFERENCE;
            /*
                (via_edge_annotation.name_id != EMPTY_NAMEID && !candidate_changes_name &&
                 !name_changes_onto_compare)
                    ? NARROW_TURN_ANGLE
                    : FUZZY_ANGLE_DIFFERENCE;
            */

            // if a turn angle isn't remotely forward, we don't consider a deviation to be distinct
            // auto const both_turns_go_into_same_direction =
            //     (candidate.angle >= STRAIGHT_ANGLE) ==
            //     (road.angle >= STRAIGHT_ANGLE); // are both turns to the left?
            auto const roads_deviation_is_distinct =
                compare_deviation / std::max(0.1, candidate_deviation) > DISTINCTION_RATIO &&
                std::abs(compare_deviation - candidate_deviation) > minimum_angle_difference;

            auto const continue_is_main_class =
                via_edge_data.flags.road_classification.GetPriority() <=
                extractor::RoadPriorityClass::SECONDARY;
            if ((!continuing_road_takes_a_turn || !continue_is_main_class) &&
                roads_deviation_is_distinct)
            {
                return false;
            }

            // in case of slight turns, there can be exits that are also very narrow. If they are on
            // a new lane though, we accept smaller distinction angles
            //
            // a - - - b - - - - c
            //            ` ` ` `d
            //
            // A narrow exit lane can be present, but still be distinct from the road
            if (num_lanes(via_edge_data) > 0 &&
                num_lanes(candidate_data) == num_lanes(via_edge_data))
            {
                if (compare_deviation > candidate_deviation &&
                    candidate_deviation <= FUZZY_ANGLE_DIFFERENCE &&
                    (compare_deviation - candidate_deviation) > 0.5 * FUZZY_ANGLE_DIFFERENCE)
                {
                    // very slight angle going straight on the exact same number of lanes as coming
                    // in, one turn branching off in a slight angle with additional lanes
                    return false;
                }
            }

            // when crossing an intersection of a similar road category, lower deviations can also
            // make sense
            // crossing a compare road
            auto const crossing_compare =
                !name_changes_onto_compare_from_opposing &&
                (util::angularDeviation(opposing_turn->angle, road.angle) >
                 STRAIGHT_ANGLE - FUZZY_ANGLE_DIFFERENCE) &&
                name_changes_onto_compare;

            // in case of a continuing road of higher road class, we accept quite a bit loweer
            // distinction
            auto const compare_has_lower_class =
                (candidate_data.flags.road_classification.GetPriority() ==
                 via_edge_data.flags.road_classification.GetPriority()) &&
                (candidate_data.flags.road_classification.GetPriority() <
                 compare_data.flags.road_classification.GetPriority());

            // for something like a tertiary link, we skip over tertiary, secondary_link, secondary,
            // primary_link and require at least a primary road
            auto const compare_has_way_higher_class =
                (candidate_data.flags.road_classification.GetPriority() ==
                 via_edge_data.flags.road_classification.GetPriority()) &&
                (std::abs(static_cast<std::int32_t>(
                              candidate_data.flags.road_classification.GetPriority()) -
                          static_cast<std::int32_t>(
                              compare_data.flags.road_classification.GetPriority())) > 4);

            if (!candidate_changes_name && !continuing_road_takes_a_turn &&
                (compare_has_lower_class || compare_has_way_higher_class || crossing_compare) &&
                compare_deviation / std::max(0.1, candidate_deviation) > 0.7 * DISTINCTION_RATIO)
            {
                return false;
            }

            // since the angle and allowed match, we compare road categories. Passing a low priority
            // road allows us to consider it non obvious
            if (distinct_by_class(road))
            {
                return false;
            }

            // switching the general road class within a turn is not a likely maneuver. We consider
            // a turn distinct enough (given it's straight/narrow continue), if it's road class
            // differs from other turns (and is of a lesser category)
            if ((getRoadGroup(via_edge_data.flags.road_classification) !=
                 getRoadGroup(compare_data.flags.road_classification)) &&
                (via_edge_data.flags.road_classification.GetPriority() ==
                 candidate_data.flags.road_classification.GetPriority()))
                return false;

            return true;
        };

        auto const itr =
            std::find_if(intersection.begin() + 1, intersection.end(), is_similar_turn);
        return itr == intersection.end();
    }
    else
    {
        // deviation is larger than NARROW_TURN_ANGLE0 here for the candidate

        // check if there is any turn, that might look just as obvious, even though it might not be
        // allowed. Entry-allowed isn't considered a valid distinction criterion here
        auto const is_similar_turn = [&](auto const &road) {
            // skip over our candidate
            if (road.eid == candidate.eid)
                return false;

            // we do not consider roads of far lesser category to be more obvious
            const auto &compare_data = node_based_graph.GetEdgeData(road.eid);
            /*
            if (strictlyLess(compare_data.flags.road_classification,
            candidate_data.flags.road_classification))
            {
                std::cout << "Road class is strictly less" << std::endl;
                return false;
            }
            */

            // if the class is just not on the same level
            if (distinct_by_class(road) && !override_class_by_lanes(compare_data))
            {
                return false;
            }

            // just as above,  switching the general road class within a turn is not a likely
            // maneuver. We consider
            // a turn distinct enough (given it's straight/narrow continue), if it's road class
            // differs from other turns. However, the difference in angles between the two needs to
            // be reasonable as well. When coming down to tertiary and less, road groups are more or
            // less random
            if (util::angularDeviation(road.angle, candidate.angle) < 100 &&
                via_edge_data.flags.road_classification.GetPriority() <=
                    extractor::RoadPriorityClass::SECONDARY &&
                ((getRoadGroup(via_edge_data.flags.road_classification) !=
                  getRoadGroup(compare_data.flags.road_classification)) &&
                 (via_edge_data.flags.road_classification.GetPriority() ==
                  candidate_data.flags.road_classification.GetPriority())) &&
                !override_class_by_lanes(compare_data) &&
                (via_edge_data.flags.road_classification.GetPriority() !=
                 extractor::RoadPriorityClass::UNCLASSIFIED) &&
                (compare_data.flags.road_classification.GetPriority() !=
                 extractor::RoadPriorityClass::UNCLASSIFIED))
            {
                return false;
            }

            // if the turn is much stronger, we are also fine (note that we do not have to check
            // absolutes, since candidate is at least > NARROW_TURN_ANGLE
            const auto compare_deviation = util::angularDeviation(road.angle, STRAIGHT_ANGLE);
            if (compare_deviation / candidate_deviation > DISTINCTION_RATIO)
            {
                return false;
            }

            return true;
        };

        return std::find_if(intersection.begin() + 1, intersection.end(), is_similar_turn) ==
               intersection.end();
    }
}

template <typename IntersectionType> // works with Intersection and IntersectionView
inline bool IntersectionHandler::IsDistinctContinue(const std::size_t index,
                                                    const EdgeID via_edge,
                                                    const IntersectionType &intersection) const
{
    // if its good enough for a turn, it's good enough for a continue
    if (IsDistinctTurn(index, via_edge, intersection))
        return true;

    auto const in_classification = node_based_graph.GetEdgeData(via_edge).flags.road_classification;
    auto const continue_classification =
        node_based_graph.GetEdgeData(intersection[index].eid).flags.road_classification;

    // nearly straight on the same road type
    if (in_classification.GetPriority() == continue_classification.GetPriority() &&
        util::angularDeviation(intersection[index].angle, STRAIGHT_ANGLE) <
            MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
        return true;

    return false;
}

// Impl.
template <typename IntersectionType> // works with Intersection and IntersectionView
std::size_t IntersectionHandler::findObviousTurn(const EdgeID via_edge,
                                                 const IntersectionType &intersection) const
{
    auto obvious_old = findObviousTurnOld(via_edge, intersection);
    auto obvious_new = findObviousTurnNew(via_edge, intersection);
    // if (obvious_new != obvious_old)
    // {
    //     std::cout << "via_edge==" << via_edge << "   old " << obvious_old << " new " <<
    //     obvious_new
    //               << "\n";
    //     BOOST_ASSERT(false);
    // }
    (void)obvious_old;
    return obvious_new;
}

template <typename IntersectionType> // works with Intersection and IntersectionView
std::size_t IntersectionHandler::findObviousTurnNew(const EdgeID via_edge,
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
    auto const continues_on_name_with_higher_class = [&](auto const &road) {
        // it needs to be possible to enter the road
        if (!road.entry_allowed)
            return true;

        // to continue on a name, we need to have one first
        if (via_edge_annotation.name_id == EMPTY_NAMEID &&
            !via_edge_data.flags.road_classification.IsLowPriorityRoadClass())
            return true;

        // and we cannot yloose it (roads loosing their name will be handled after this check here)
        auto const &road_data = node_based_graph.GetEdgeData(road.eid);
        const auto &road_annotation = node_data_container.GetAnnotation(road_data.annotation_data);
        if (road_annotation.name_id == EMPTY_NAMEID &&
            !road_data.flags.road_classification.IsLowPriorityRoadClass())
            return true;

        // if not both of the entries are empty, we do not consider this a continue
        if ((via_edge_annotation.name_id == EMPTY_NAMEID) ^
            (road_annotation.name_id == EMPTY_NAMEID))
            return true;

        // the priority can only stay the same or increase. We don't consider a primary->residential
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
    auto const to_index_if_valid = [&](auto const iterator) -> std::size_t {
        auto const &from_data = node_based_graph.GetEdgeData(via_edge);
        auto const &to_data = node_based_graph.GetEdgeData(iterator->eid);

        if (from_data.flags.roundabout != to_data.flags.roundabout)
            return 0;

        auto const from_mode =
            node_data_container.GetAnnotation(from_data.annotation_data).travel_mode;
        auto const to_mode = node_data_container.GetAnnotation(to_data.annotation_data).travel_mode;

        if (from_mode == to_mode)
            return std::distance(intersection.begin(), iterator);
        else
            return 0;
    };

    // in case the continuing road is distinct, we prefer continuing on the current road. Only if
    // continue does not exist or we are not distinct, we look for other possible candidates
    if (road_continues_itr != intersection.end() &&
        IsDistinctContinue(
            std::distance(intersection.begin(), road_continues_itr), via_edge, intersection))
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
    auto const valid_of_higher_or_same_category = [&](auto const &road) {
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
        IsDistinctTurn(
            std::distance(intersection.begin(), straightmost_turn_itr), via_edge, intersection))
    {
        return to_index_if_valid(straightmost_turn_itr);
    }

    auto const valid_turn = [&](auto const &road) { return !road.entry_allowed; };

    // we cannot find a turn of same or higher priority, so we check if any straightmost turn could
    // be obvious. We only consider somewhat narrow turns for these cases though
    auto const straightmost_valid = intersection.findClosestTurn(STRAIGHT_ANGLE, valid_turn);
    // no valid turns
    if (straightmost_valid == intersection.end())
        return 0;

    auto const non_sharp_turns = intersection.Count(
        [&](auto const &road) { return util::angularDeviation(road.angle, STRAIGHT_ANGLE) <= 90; });
    auto const straight_is_only_non_sharp =
        (util::angularDeviation(straightmost_valid->angle, STRAIGHT_ANGLE) <= 90) &&
        (non_sharp_turns == 1);

    if ((straightmost_valid != straightmost_turn_itr) &&
        (straightmost_valid != intersection.end()) &&
        (util::angularDeviation(STRAIGHT_ANGLE, straightmost_valid->angle) <= GROUP_ANGLE ||
         straight_is_only_non_sharp) &&
        !node_based_graph.GetEdgeData(straightmost_valid->eid)
             .flags.road_classification.IsLowPriorityRoadClass() &&
        IsDistinctTurn(
            std::distance(intersection.begin(), straightmost_valid), via_edge, intersection))
    {
        return to_index_if_valid(straightmost_valid);
    }

    // special case handling for motorways, for which nearly narrow / only allowed turns are always
    // obvious
    if (node_based_graph.GetEdgeData(straightmost_valid->eid)
            .flags.road_classification.IsMotorwayClass() &&
        util::angularDeviation(straightmost_valid->angle, STRAIGHT_ANGLE) <= GROUP_ANGLE &&
        intersection.countEnterable() == 1)
    {
        return to_index_if_valid(straightmost_valid);
    }

    // Special case handling for roads splitting up, all the same name (exactly the same)
    if (intersection.size() == 3 &&
        std::all_of(intersection.begin(),
                    intersection.end(),
                    [ id = via_edge_annotation.name_id, this ](auto const &road) {
                        auto const data_id = node_based_graph.GetEdgeData(road.eid).annotation_data;
                        auto const name_id = node_data_container.GetAnnotation(data_id).name_id;
                        return (name_id != EMPTY_NAMEID) && (name_id == id);
                    }) &&
        intersection.countEnterable() == 1 &&
        // ensure that we do not lookt at a end of the road turn in a segregated intersection
        (util::angularDeviation(intersection[1].angle, 90) > NARROW_TURN_ANGLE ||
         util::angularDeviation(intersection[2].angle, 270) > NARROW_TURN_ANGLE))
    {
        return to_index_if_valid(straightmost_valid);
    }

    return 0;
}

template <typename IntersectionType> // works with Intersection and IntersectionView
std::size_t IntersectionHandler::findObviousTurnOld(const EdgeID via_edge,
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
