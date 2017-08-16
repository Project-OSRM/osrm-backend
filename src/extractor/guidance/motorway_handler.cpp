#include "extractor/guidance/motorway_handler.hpp"
#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/road_classification.hpp"

#include "util/bearing.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/guidance/name_announcements.hpp"

#include <limits>
#include <utility>

#include <boost/assert.hpp>

using osrm::util::angularDeviation;
using osrm::extractor::guidance::getTurnDirection;

namespace osrm
{
namespace extractor
{
namespace guidance
{
namespace
{

inline bool isMotorwayClass(EdgeID eid, const util::NodeBasedDynamicGraph &node_based_graph)
{
    auto const &classification = node_based_graph.GetEdgeData(eid).flags.road_classification;
    return classification.IsMotorwayClass() && !classification.IsLinkClass();
}
inline RoadClassification roadClass(const ConnectedRoad &road,
                                    const util::NodeBasedDynamicGraph &graph)
{
    return graph.GetEdgeData(road.eid).flags.road_classification;
}

inline bool isRampClass(EdgeID eid, const util::NodeBasedDynamicGraph &node_based_graph)
{
    return node_based_graph.GetEdgeData(eid).flags.road_classification.IsRampClass();
}

} // namespace

MotorwayHandler::MotorwayHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                                 const EdgeBasedNodeDataContainer &node_data_container,
                                 const std::vector<util::Coordinate> &coordinates,
                                 const util::NameTable &name_table,
                                 const SuffixTable &street_name_suffix_table,
                                 const IntersectionGenerator &intersection_generator)
    : IntersectionHandler(node_based_graph,
                          node_data_container,
                          coordinates,
                          name_table,
                          street_name_suffix_table,
                          intersection_generator)
{
}

bool MotorwayHandler::canProcess(const NodeID,
                                 const EdgeID via_eid,
                                 const Intersection &intersection) const
{
    // the road we come from needs to be a motorway
    if (!node_based_graph.GetEdgeData(via_eid).flags.road_classification.IsMotorwayClass())
        return false;

    // all adjacent roads that allow turns need to be motorway classes and narrow turns
    auto const is_non_motorway_class_or_not_narrow = [&](auto const &road) {
        if (road.entry_allowed && angularDeviation(road.angle, STRAIGHT_ANGLE) > 60)
            return true;

        return !node_based_graph.GetEdgeData(road.eid).flags.road_classification.IsMotorwayClass();
    };
    return !std::any_of(
        intersection.begin(), intersection.end(), is_non_motorway_class_or_not_narrow);
}

Intersection MotorwayHandler::
operator()(const NodeID, const EdgeID via_eid, Intersection intersection) const
{
    // coming from motorway
    if (isMotorwayClass(via_eid, node_based_graph))
    {
        intersection = fromMotorway(via_eid, std::move(intersection));
        std::for_each(intersection.begin(), intersection.end(), [](ConnectedRoad &road) {
            if (road.instruction.type == TurnType::OnRamp)
                road.instruction.type = TurnType::OffRamp;
        });
        return intersection;
    }
    else // coming from a ramp
    {
        return fromRamp(via_eid, std::move(intersection));
        // ramp merging straight onto motorway
    }
}

Intersection MotorwayHandler::fromMotorway(const EdgeID via_eid, Intersection intersection) const
{
    BOOST_ASSERT(isMotorwayClass(via_eid, node_based_graph));

    // continue_pos == 0 if no continue
    auto const continue_pos = findObviousTurn(via_eid, intersection);
    auto continue_angle = intersection[continue_pos].angle;

    if (continue_pos == 0)
    {
        if (intersection.countEnterable() == 1)
        {
            auto allowed = std::find_if(intersection.begin(),
                                        intersection.end(),
                                        [](auto const &road) { return road.entry_allowed; });
            auto const index = std::distance(intersection.begin(), allowed);

            intersection[index].instruction =
                getInstructionForObvious(intersection.size(), via_eid, false, intersection[index]);
        }
        else if (intersection.size() == 2)
        {
            // do not announce ramps at the end of a highway
            intersection[1].instruction = {TurnType::NoTurn,
                                           getTurnDirection(intersection[1].angle)};
        }
        else if (intersection.size() == 3)
        {
            // splitting ramp at the end of a highway
            if (intersection[1].entry_allowed && intersection[2].entry_allowed)
            {
                assignFork(via_eid, intersection[2], intersection[1]);
            }
            else
            {
                // ending in a passing ramp
                if (intersection[1].entry_allowed)
                    intersection[1].instruction = {TurnType::NoTurn,
                                                   getTurnDirection(intersection[1].angle)};
                else
                    intersection[2].instruction = {TurnType::NoTurn,
                                                   getTurnDirection(intersection[2].angle)};
            }
        }
        else if (intersection.size() == 4 &&
                 roadClass(intersection[1], node_based_graph) ==
                     roadClass(intersection[2], node_based_graph) &&
                 roadClass(intersection[2], node_based_graph) ==
                     roadClass(intersection[3], node_based_graph))
        {
            // tripple fork at the end
            assignFork(via_eid, intersection[3], intersection[2], intersection[1]);
        }

        else if (intersection.countEnterable() > 0) // check whether turns exist at all
        {
            // FALLBACK, this should hopefully never be reached
            return fallback(std::move(intersection));
        }
    }
    else
    {
        // for counting how many exiting roads are motorways that allow entry
        const auto is_motorway_entry = [this](const auto &road) {
            if (!road.entry_allowed)
                return false;

            auto const &road_class = node_based_graph.GetEdgeData(road.eid).flags.road_classification;
            return road_class.IsMotorwayClass() && !road_class.IsLinkClass();
        };

        const unsigned exiting_motorways =
            std::count_if(intersection.begin() + 1, intersection.end(), is_motorway_entry);

        if (exiting_motorways == 0)
        {
            // Ending in Ramp
            for (auto &road : intersection)
            {
                if (road.entry_allowed)
                {
                    BOOST_ASSERT(isRampClass(road.eid, node_based_graph));
                    road.instruction = TurnInstruction::SUPPRESSED(getTurnDirection(road.angle));
                }
            }
        }
        else if (exiting_motorways == 1)
        {
            // normal motorway passing some ramps or mering onto another motorway
            if (intersection.size() == 2)
            {
                BOOST_ASSERT(!isRampClass(intersection[1].eid, node_based_graph));

                intersection[1].instruction =
                    getInstructionForObvious(intersection.size(),
                                             via_eid,
                                             isThroughStreet(1, intersection),
                                             intersection[1]);
            }
            else
            {
                // Normal Highway exit or merge
                for (auto &road : intersection)
                {
                    // ignore invalid uturns/other
                    if (!road.entry_allowed)
                        continue;

                    if (road.angle == continue_angle)
                    {
                        road.instruction = getInstructionForObvious(
                            intersection.size(), via_eid, isThroughStreet(1, intersection), road);
                    }
                    else if (road.angle < continue_angle)
                    {
                        road.instruction = {isRampClass(road.eid, node_based_graph)
                                                ? TurnType::OffRamp
                                                : TurnType::Turn,
                                            (road.angle < 145) ? DirectionModifier::Right
                                                               : DirectionModifier::SlightRight};
                    }
                    else if (road.angle > continue_angle)
                    {
                        road.instruction = {isRampClass(road.eid, node_based_graph)
                                                ? TurnType::OffRamp
                                                : TurnType::Turn,
                                            (road.angle > 215) ? DirectionModifier::Left
                                                               : DirectionModifier::SlightLeft};
                    }
                }
            }
        }
        // handle motorway forks
        else if (exiting_motorways > 1)
        {
            if (exiting_motorways == 2 && intersection.size() == 2)
            {
                intersection[1].instruction =
                    getInstructionForObvious(intersection.size(),
                                             via_eid,
                                             isThroughStreet(1, intersection),
                                             intersection[1]);
                intersection[0].entry_allowed = false; // UTURN on the freeway
            }
            else if (exiting_motorways == 2)
            {
                // standard fork
                std::size_t first_valid = std::numeric_limits<std::size_t>::max(),
                            second_valid = std::numeric_limits<std::size_t>::max();
                for (std::size_t i = 0; i < intersection.size(); ++i)
                {
                    if (intersection[i].entry_allowed &&
                        isMotorwayClass(intersection[i].eid, node_based_graph))
                    {
                        if (first_valid < intersection.size())
                        {
                            second_valid = i;
                            break;
                        }
                        else
                        {
                            first_valid = i;
                        }
                    }
                }
                assignFork(via_eid, intersection[second_valid], intersection[first_valid]);
            }
            else if (exiting_motorways == 3)
            {
                // triple fork
                std::size_t first_valid = std::numeric_limits<std::size_t>::max(),
                            second_valid = std::numeric_limits<std::size_t>::max(),
                            third_valid = std::numeric_limits<std::size_t>::max();
                for (std::size_t i = 0; i < intersection.size(); ++i)
                {
                    if (intersection[i].entry_allowed &&
                        isMotorwayClass(intersection[i].eid, node_based_graph))
                    {
                        if (second_valid < intersection.size())
                        {
                            third_valid = i;
                            break;
                        }
                        else if (first_valid < intersection.size())
                        {
                            second_valid = i;
                        }
                        else
                        {
                            first_valid = i;
                        }
                    }
                }
                assignFork(via_eid,
                           intersection[third_valid],
                           intersection[second_valid],
                           intersection[first_valid]);
            }
            else
            {
                return fallback(std::move(intersection));
            }
        } // done for more than one highway exit
    }
    return intersection;
}

Intersection MotorwayHandler::fromRamp(const EdgeID via_eid, Intersection intersection) const
{
    auto const is_link_road = [&](auto const &road) {
        return node_based_graph.GetEdgeData(road.eid).flags.road_classification.IsRampClass();
    };
    auto const all_ramps = std::all_of(intersection.begin(), intersection.end(), is_link_road);

    // in case all entries are ramps
    if (all_ramps)
    {
        std::vector<std::size_t> valids;
        for (std::size_t i = 1; i < intersection.size(); ++i)
            if (intersection[i].entry_allowed)
                valids.push_back(i);

        auto const obvious = findObviousTurn(via_eid, intersection);
        if (obvious != 0)
        {
            // if only two roads exit, we are looking at a ramp forking off from a different exit
            if (valids.size() == 2)
            {
                if (valids[0] == obvious)
                {
                    intersection[valids[0]].instruction =
                        getInstructionForObvious(intersection.size(),
                                                 via_eid,
                                                 isThroughStreet(valids[0], intersection),
                                                 intersection[valids[0]]);
                    intersection[valids[1]].instruction = {
                        findBasicTurnType(via_eid, intersection[valids[1]]),
                        getTurnDirection(intersection[valids[1]].angle)};
                }
                else
                {
                    intersection[valids[1]].instruction =
                        getInstructionForObvious(intersection.size(),
                                                 via_eid,
                                                 isThroughStreet(valids[1], intersection),
                                                 intersection[valids[1]]);
                    intersection[valids[0]].instruction = {
                        findBasicTurnType(via_eid, intersection[valids[0]]),
                        getTurnDirection(intersection[valids[0]].angle)};
                }
            }
            else
            {
                // announce as basic turns
                for (std::size_t i = 1; i < intersection.size(); ++i)
                {
                    auto &road = intersection[i];
                    if (road.eid == intersection[obvious].eid)
                    {
                        road.instruction = getInstructionForObvious(
                            intersection.size(), via_eid, isThroughStreet(i, intersection), road);
                    }
                    else
                    {
                        road.instruction = {findBasicTurnType(via_eid, road),
                                            getTurnDirection(road.angle)};
                    }
                }
            }
        }
        else
        {
            // check if the combination of left/right forms a close triangle at the road. This is
            // the case if a ramp splits up just before an intersection. We don't want to consider
            // these as a fork, since we do not collapse forks.
            auto const close_triangle = [&](auto const left, auto const right) {
                auto const left_node = node_based_graph.GetTarget(intersection[left].eid);
                auto const right_node = node_based_graph.GetTarget(intersection[right].eid);

                // if the distance to the next intersection is large, we do not consider it a fork
                // pre-interseciton. On motorways these can be quite long, but they shouldn't be as
                // long as an off+on ramp.
                const constexpr auto DISTANCE_LIMIT = 100;
                auto const coordinate_at_center = coordinates[node_based_graph.GetTarget(via_eid)];

                if (util::coordinate_calculation::haversineDistance(coordinate_at_center,coordinates[left_node]) > DISTANCE_LIMIT ||
                    util::coordinate_calculation::haversineDistance(coordinate_at_center,coordinates[right_node])> DISTANCE_LIMIT)
                    return false;

                // forming a triangle
                for (auto eid : node_based_graph.GetAdjacentEdgeRange(left_node))
                {
                    if (node_based_graph.GetTarget(eid) == right_node)
                        return true;
                }
                return false;
            };

            // two non-obvious highway ramps, announce as a fork
            if (valids.size() == 2 && !close_triangle(valids[1], valids[0]))
            {
                // left road comes first, we find the right first though. Therefore we have to use
                // `1` first
                assignFork(via_eid, intersection[valids[1]], intersection[valids[0]]);
            }
            else
            {
                // announce as basic turns
                for (auto &road : intersection)
                    road.instruction = {findBasicTurnType(via_eid, road),
                                        getTurnDirection(road.angle)};
            }
        }
        return intersection;
    }

    auto num_valid_turns = intersection.countEnterable();
    // ramp straight into a motorway/ramp
    if (intersection.size() == 2 && num_valid_turns == 1)
    {
        BOOST_ASSERT(!intersection[0].entry_allowed);
        BOOST_ASSERT(isMotorwayClass(intersection[1].eid, node_based_graph));

        intersection[1].instruction = getInstructionForObvious(
            intersection.size(), via_eid, isThroughStreet(1, intersection), intersection[1]);
    }
    else if (intersection.size() == 3)
    {
        const auto &second_intersection_data = node_data_container.GetAnnotation(
            node_based_graph.GetEdgeData(intersection[2].eid).annotation_data);
        const auto &first_intersection_data = node_data_container.GetAnnotation(
            node_based_graph.GetEdgeData(intersection[1].eid).annotation_data);
        const auto first_second_same_name =
            !util::guidance::requiresNameAnnounced(second_intersection_data.name_id,
                                                   first_intersection_data.name_id,
                                                   name_table,
                                                   street_name_suffix_table);

        // merging onto a passing highway / or two ramps merging onto the same highway
        if (num_valid_turns == 1)
        {
            BOOST_ASSERT(!intersection[0].entry_allowed);
            // check order of highways
            //          4
            //     5         3
            //
            //   6              2
            //
            //     7         1
            //          0
            if (intersection[1].entry_allowed)
            {
                if (isMotorwayClass(intersection[1].eid, node_based_graph) &&
                    second_intersection_data.name_id != EMPTY_NAMEID &&
                    first_intersection_data.name_id != EMPTY_NAMEID && first_second_same_name)
                {
                    // circular order indicates a merge to the left (0-3 onto 4
                    if (angularDeviation(intersection[1].angle, STRAIGHT_ANGLE) <
                        2 * NARROW_TURN_ANGLE)
                        intersection[1].instruction = {TurnType::Merge,
                                                       DirectionModifier::SlightLeft};
                    else // fallback
                        intersection[1].instruction = {TurnType::Merge,
                                                       getTurnDirection(intersection[1].angle)};
                }
                else // passing by the end of a motorway
                {
                    intersection[1].instruction =
                        getInstructionForObvious(intersection.size(),
                                                 via_eid,
                                                 isThroughStreet(1, intersection),
                                                 intersection[1]);
                }
            }
            else
            {
                BOOST_ASSERT(intersection[2].entry_allowed);
                if (isMotorwayClass(intersection[2].eid, node_based_graph) &&
                    second_intersection_data.name_id != EMPTY_NAMEID &&
                    first_intersection_data.name_id != EMPTY_NAMEID && first_second_same_name)
                {
                    // circular order (5-0) onto 4
                    if (angularDeviation(intersection[2].angle, STRAIGHT_ANGLE) <
                        2 * NARROW_TURN_ANGLE)
                        intersection[2].instruction = {TurnType::Merge,
                                                       DirectionModifier::SlightRight};
                    else // fallback
                        intersection[2].instruction = {TurnType::Merge,
                                                       getTurnDirection(intersection[2].angle)};
                }
                else // passing the end of a highway
                {
                    intersection[2].instruction =
                        getInstructionForObvious(intersection.size(),
                                                 via_eid,
                                                 isThroughStreet(2, intersection),
                                                 intersection[2]);
                }
            }
        }
        else
        {
            BOOST_ASSERT(num_valid_turns == 2);
            // UTurn on ramps is not possible
            BOOST_ASSERT(!intersection[0].entry_allowed);
            BOOST_ASSERT(intersection[1].entry_allowed);
            BOOST_ASSERT(intersection[2].entry_allowed);
            // two motorways starting at end of ramp (fork)
            //  M       M
            //    \   /
            //      |
            //      R
            if (isMotorwayClass(intersection[1].eid, node_based_graph) &&
                isMotorwayClass(intersection[2].eid, node_based_graph))
            {
                assignFork(via_eid, intersection[2], intersection[1]);
            }
            else
            {
                // continued ramp passing motorway entry
                //      M  R
                //      M  R
                //      | /
                //      R
                if (isMotorwayClass(intersection[1].eid, node_based_graph))
                {
                    intersection[1].instruction = {TurnType::Turn, DirectionModifier::SlightRight};
                    intersection[2].instruction = {TurnType::Continue,
                                                   DirectionModifier::SlightLeft};
                }
                else
                {
                    assignFork(via_eid, intersection[2], intersection[1]);
                }
            }
        }
    }
    // On - Off Ramp on passing Motorway, Ramp onto Fork(?)
    else if (intersection.size() == 4)
    {
        bool passed_highway_entry = false;
        for (auto &road : intersection)
        {
            if (!road.entry_allowed && isMotorwayClass(road.eid, node_based_graph))
            {
                passed_highway_entry = true;
            }
            else if (isMotorwayClass(road.eid, node_based_graph))
            {
                road.instruction = {TurnType::Merge,
                                    passed_highway_entry ? DirectionModifier::SlightRight
                                                         : DirectionModifier::SlightLeft};
            }
            else
            {
                BOOST_ASSERT(isRampClass(road.eid, node_based_graph));
                road.instruction = {TurnType::OffRamp, getTurnDirection(road.angle)};
            }
        }
    }
    else
    {
        return fallback(std::move(intersection));
    }
    return intersection;
}

Intersection MotorwayHandler::fallback(Intersection intersection) const
{
    for (auto &road : intersection)
    {
        if (!road.entry_allowed)
            continue;

        const auto type =
            isMotorwayClass(road.eid, node_based_graph) ? TurnType::Merge : TurnType::Turn;

        if (type == TurnType::Turn)
        {
            if (angularDeviation(road.angle, STRAIGHT_ANGLE) < FUZZY_ANGLE_DIFFERENCE)
                road.instruction = {type, DirectionModifier::Straight};
            else
            {
                road.instruction = {type,
                                    road.angle > STRAIGHT_ANGLE ? DirectionModifier::SlightLeft
                                                                : DirectionModifier::SlightRight};
            }
        }
        else
        {
            road.instruction = {type,
                                road.angle < STRAIGHT_ANGLE ? DirectionModifier::SlightLeft
                                                            : DirectionModifier::SlightRight};
        }
    }
    return intersection;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
