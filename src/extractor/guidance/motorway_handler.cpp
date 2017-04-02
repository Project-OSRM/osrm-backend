#include "extractor/guidance/motorway_handler.hpp"
#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/road_classification.hpp"

#include "util/bearing.hpp"
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
    return node_based_graph.GetEdgeData(eid).road_classification.IsMotorwayClass();
}
inline RoadClassification roadClass(const ConnectedRoad &road,
                                    const util::NodeBasedDynamicGraph &graph)
{
    return graph.GetEdgeData(road.eid).road_classification;
}

inline bool isRampClass(EdgeID eid, const util::NodeBasedDynamicGraph &node_based_graph)
{
    return node_based_graph.GetEdgeData(eid).road_classification.IsRampClass();
}

} // namespace

MotorwayHandler::MotorwayHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                                 const std::vector<util::Coordinate> &coordinates,
                                 const util::NameTable &name_table,
                                 const SuffixTable &street_name_suffix_table,
                                 const IntersectionGenerator &intersection_generator)
    : IntersectionHandler(node_based_graph,
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
    bool has_motorway = false;
    bool has_normal_roads = false;

    for (const auto &road : intersection)
    {
        // not merging or forking?
        if (road.entry_allowed && angularDeviation(road.angle, STRAIGHT_ANGLE) > 60)
            return false;
        else if (isMotorwayClass(road.eid, node_based_graph))
        {
            if (road.entry_allowed)
                has_motorway = true;
        }
        else if (!isRampClass(road.eid, node_based_graph))
            has_normal_roads = true;
    }

    if (has_normal_roads)
        return false;

    return has_motorway || isMotorwayClass(via_eid, node_based_graph);
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
    const auto &in_data = node_based_graph.GetEdgeData(via_eid);
    BOOST_ASSERT(isMotorwayClass(via_eid, node_based_graph));

    const auto countExitingMotorways = [this](const Intersection &intersection) {
        unsigned count = 0;
        for (const auto &road : intersection)
        {
            if (road.entry_allowed && isMotorwayClass(road.eid, node_based_graph))
                ++count;
        }
        return count;
    };

    // find the angle that continues on our current highway
    const auto getContinueAngle = [this, in_data](const Intersection &intersection) {
        for (const auto &road : intersection)
        {
            const auto &out_data = node_based_graph.GetEdgeData(road.eid);

            const auto same_name = !util::guidance::requiresNameAnnounced(
                in_data.name_id, out_data.name_id, name_table, street_name_suffix_table);

            if (road.angle != 0 && in_data.name_id != EMPTY_NAMEID &&
                out_data.name_id != EMPTY_NAMEID && same_name &&
                isMotorwayClass(road.eid, node_based_graph))
                return road.angle;
        }
        return intersection[0].angle;
    };

    const auto getMostLikelyContinue = [this, in_data](const Intersection &intersection) {
        double angle = intersection[0].angle;
        double best = 180;
        for (const auto &road : intersection)
        {
            if (isMotorwayClass(road.eid, node_based_graph) &&
                angularDeviation(road.angle, STRAIGHT_ANGLE) < best)
            {
                best = angularDeviation(road.angle, STRAIGHT_ANGLE);
                angle = road.angle;
            }
        }
        return angle;
    };

    const auto findBestContinue = [&]() {
        const double continue_angle = getContinueAngle(intersection);
        if (continue_angle != intersection[0].angle)
            return continue_angle;
        else
            return getMostLikelyContinue(intersection);
    };

    // find continue angle
    const double continue_angle = findBestContinue();
    // highway does not continue and has no obvious choice
    if (continue_angle == intersection[0].angle)
    {
        if (intersection.size() == 2)
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
        const unsigned exiting_motorways = countExitingMotorways(intersection);

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
        const auto &second_intersection_data = node_based_graph.GetEdgeData(intersection[2].eid);
        const auto &first_intersection_data = node_based_graph.GetEdgeData(intersection[1].eid);
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
