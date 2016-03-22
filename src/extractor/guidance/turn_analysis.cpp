#include "extractor/guidance/turn_analysis.hpp"

#include "util/simple_logger.hpp"
#include "util/coordinate.hpp"

#include <cstddef>
#include <limits>
#include <iomanip>

namespace osrm
{
namespace extractor
{
namespace guidance
{
// configuration of turn classification
const bool constexpr INVERT = true;

// what angle is interpreted as going straight
const double constexpr STRAIGHT_ANGLE = 180.;
// if a turn deviates this much from going straight, it will be kept straight
const double constexpr MAXIMAL_ALLOWED_NO_TURN_DEVIATION = 3.;
// angle that lies between two nearly indistinguishable roads
const double constexpr NARROW_TURN_ANGLE = 30.;
const double constexpr GROUP_ANGLE = 90;
// angle difference that can be classified as straight, if its the only narrow turn
const double constexpr FUZZY_ANGLE_DIFFERENCE = 15.;
const double constexpr DISTINCTION_RATIO = 2;
const unsigned constexpr INVALID_NAME_ID = 0;

using EdgeData = util::NodeBasedDynamicGraph::EdgeData;

ConnectedRoad::ConnectedRoad(const TurnOperation turn, const bool entry_allowed)
    : turn(turn), entry_allowed(entry_allowed)
{
}

bool requiresAnnouncement(const EdgeData &from, const EdgeData &to)
{
    return !from.IsCompatibleTo(to);
}

struct Localizer
{
    const std::vector<QueryNode> *node_info_list = nullptr;

    util::Coordinate operator()(const NodeID nid)
    {
        if (node_info_list)
        {
            return {(*node_info_list)[nid].lon, (*node_info_list)[nid].lat};
        }
        return {};
    }
};

static Localizer localizer;

TurnAnalysis::TurnAnalysis(const util::NodeBasedDynamicGraph &node_based_graph,
                           const std::vector<QueryNode> &node_info_list,
                           const RestrictionMap &restriction_map,
                           const std::unordered_set<NodeID> &barrier_nodes,
                           const CompressedEdgeContainer &compressed_edge_container,
                           const util::NameTable &name_table)
    : node_based_graph(node_based_graph), node_info_list(node_info_list),
      restriction_map(restriction_map), barrier_nodes(barrier_nodes),
      compressed_edge_container(compressed_edge_container), name_table(name_table)
{
}

// some small tool functions to simplify decisions down the line
namespace detail
{

inline FunctionalRoadClass roadClass(const ConnectedRoad &road,
                                     const util::NodeBasedDynamicGraph &graph)
{
    return graph.GetEdgeData(road.turn.eid).road_classification.road_class;
}

inline bool isMotorwayClass(FunctionalRoadClass road_class)
{
    return road_class == FunctionalRoadClass::MOTORWAY || road_class == FunctionalRoadClass::TRUNK;
}

inline bool isMotorwayClass(EdgeID eid, const util::NodeBasedDynamicGraph &node_based_graph)
{
    return isMotorwayClass(node_based_graph.GetEdgeData(eid).road_classification.road_class);
}

inline bool isRampClass(EdgeID eid, const util::NodeBasedDynamicGraph &node_based_graph)
{
    return isRampClass(node_based_graph.GetEdgeData(eid).road_classification.road_class);
}

} // namespace detail

std::vector<TurnOperation> TurnAnalysis::getTurns(const NodeID from, const EdgeID via_edge) const
{
    localizer.node_info_list = &node_info_list;
    auto intersection = getConnectedRoads(from, via_edge);

    const auto &in_edge_data = node_based_graph.GetEdgeData(via_edge);

    // main priority: roundabouts
    bool on_roundabout = in_edge_data.roundabout;
    bool can_enter_roundabout = false;
    bool can_exit_roundabout = false;
    for (const auto &road : intersection)
    {
        const auto &edge_data = node_based_graph.GetEdgeData(road.turn.eid);
        // only check actual outgoing edges
        if (edge_data.reversed)
            continue;

        if (edge_data.roundabout)
        {
            can_enter_roundabout = true;
        }
        else
        {
            can_exit_roundabout = true;
        }
    }
    if (on_roundabout || can_enter_roundabout)
    {
        intersection = handleRoundabouts(via_edge, on_roundabout, can_exit_roundabout,
                                         std::move(intersection));
    }
    else
    {
        // set initial defaults for normal turns and modifier based on angle
        intersection = setTurnTypes(from, via_edge, std::move(intersection));
        if (isMotorwayJunction(via_edge, intersection))
        {
            intersection = handleMotorwayJunction(via_edge, std::move(intersection));
        }

        else if (intersection.size() == 1)
        {
            intersection = handleOneWayTurn(std::move(intersection));
        }
        else if (intersection.size() == 2)
        {
            intersection = handleTwoWayTurn(via_edge, std::move(intersection));
        }
        else if (intersection.size() == 3)
        {
            intersection = handleThreeWayTurn(via_edge, std::move(intersection));
        }
        else
        {
            intersection = handleComplexTurn(via_edge, std::move(intersection));
        }
        // complex intersection, potentially requires conflict resolution
    }

    std::vector<TurnOperation> turns;
    for (auto road : intersection)
        if (road.entry_allowed)
            turns.emplace_back(road.turn);

    return turns;
}

inline std::size_t countValid(const std::vector<ConnectedRoad> &intersection)
{
    return std::count_if(intersection.begin(), intersection.end(), [](const ConnectedRoad &road)
                         {
                             return road.entry_allowed;
                         });
}

std::vector<ConnectedRoad>
TurnAnalysis::handleRoundabouts(const EdgeID via_edge,
                                const bool on_roundabout,
                                const bool can_exit_roundabout,
                                std::vector<ConnectedRoad> intersection) const
{
    // TODO requires differentiation between roundabouts and rotaries
    // detect via radius (get via circle through three vertices)
    NodeID node_v = node_based_graph.GetTarget(via_edge);
    if (on_roundabout)
    {
        // Shoule hopefully have only a single exit and continue
        // at least for cars. How about bikes?
        for (auto &road : intersection)
        {
            auto &turn = road.turn;
            const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);
            if (out_data.roundabout)
            {
                // TODO can forks happen in roundabouts? E.g. required lane changes
                if (1 == node_based_graph.GetDirectedOutDegree(node_v))
                {
                    // No turn possible.
                    turn.instruction = TurnInstruction::NO_TURN();
                }
                else
                {
                    turn.instruction =
                        TurnInstruction::REMAIN_ROUNDABOUT(getTurnDirection(turn.angle));
                }
            }
            else
            {
                turn.instruction = TurnInstruction::EXIT_ROUNDABOUT(getTurnDirection(turn.angle));
            }
        }
        return intersection;
    }
    else
    {
        for (auto &road : intersection)
        {
            if (!road.entry_allowed)
                continue;
            auto &turn = road.turn;
            const auto &out_data = node_based_graph.GetEdgeData(turn.eid);
            if (out_data.roundabout)
            {
                turn.instruction = TurnInstruction::ENTER_ROUNDABOUT(getTurnDirection(turn.angle));
                if (can_exit_roundabout)
                {
                    if (turn.instruction.type == TurnType::EnterRotary)
                        turn.instruction.type = TurnType::EnterRotaryAtExit;
                    if (turn.instruction.type == TurnType::EnterRoundabout)
                        turn.instruction.type = TurnType::EnterRoundaboutAtExit;
                }
            }
            else
            {
                turn.instruction = {TurnType::EnterAndExitRoundabout, getTurnDirection(turn.angle)};
            }
        }
        return intersection;
    }
}

std::vector<ConnectedRoad>
TurnAnalysis::fallbackTurnAssignmentMotorway(std::vector<ConnectedRoad> intersection) const
{
    for (auto &road : intersection)
    {
        const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);

        util::SimpleLogger().Write(logWARNING)
            << "road: " << road.toString() << " Name: " << out_data.name_id
            << " Road Class: " << (int)out_data.road_classification.road_class
            << " At: " << localizer(node_based_graph.GetTarget(road.turn.eid));

        if (!road.entry_allowed)
            continue;

        const auto type = detail::isMotorwayClass(out_data.road_classification.road_class)
                              ? TurnType::Merge
                              : TurnType::Turn;
        if (angularDeviation(road.turn.angle, STRAIGHT_ANGLE) < FUZZY_ANGLE_DIFFERENCE)
            road.turn.instruction = {type, DirectionModifier::Straight};
        else
        {
            road.turn.instruction = {type,
                                     road.turn.angle > STRAIGHT_ANGLE
                                         ? DirectionModifier::SlightLeft
                                         : DirectionModifier::SlightRight};
        }
    }
    return intersection;
}

std::vector<ConnectedRoad>
TurnAnalysis::handleFromMotorway(const EdgeID via_edge,
                                 std::vector<ConnectedRoad> intersection) const
{
    const auto &in_data = node_based_graph.GetEdgeData(via_edge);
    BOOST_ASSERT(detail::isMotorwayClass(in_data.road_classification.road_class));

    const auto countExitingMotorways = [this](const std::vector<ConnectedRoad> &intersection)
    {
        unsigned count = 0;
        for (const auto &road : intersection)
        {
            if (road.entry_allowed && detail::isMotorwayClass(road.turn.eid, node_based_graph))
                ++count;
        }
        return count;
    };

    // find the angle that continues on our current highway
    const auto getContinueAngle = [this, in_data](const std::vector<ConnectedRoad> &intersection)
    {
        for (const auto &road : intersection)
        {
            const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);
            if (road.turn.angle != 0 && in_data.name_id == out_data.name_id &&
                in_data.name_id != 0 &&
                detail::isMotorwayClass(out_data.road_classification.road_class))
                return road.turn.angle;
        }
        return intersection[0].turn.angle;
    };

    const auto getMostLikelyContinue =
        [this, in_data](const std::vector<ConnectedRoad> &intersection)
    {
        double angle = intersection[0].turn.angle;
        double best = 180;
        for (const auto &road : intersection)
        {
            const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);
            if (detail::isMotorwayClass(out_data.road_classification.road_class) &&
                angularDeviation(road.turn.angle, STRAIGHT_ANGLE) < best)
            {
                best = angularDeviation(road.turn.angle, STRAIGHT_ANGLE);
                angle = road.turn.angle;
            }
        }
        return angle;
    };

    const auto findBestContinue = [&]()
    {
        const double continue_angle = getContinueAngle(intersection);
        if (continue_angle != intersection[0].turn.angle)
            return continue_angle;
        else
            return getMostLikelyContinue(intersection);
    };

    // find continue angle
    const double continue_angle = findBestContinue();

    // highway does not continue and has no obvious choice
    if (continue_angle == intersection[0].turn.angle)
    {
        if (intersection.size() == 2)
        {
            // do not announce ramps at the end of a highway
            intersection[1].turn.instruction = {TurnType::NoTurn,
                                                getTurnDirection(intersection[1].turn.angle)};
        }
        else if (intersection.size() == 3)
        {
            // splitting ramp at the end of a highway
            if (intersection[1].entry_allowed && intersection[2].entry_allowed)
            {
                assignFork(via_edge, intersection[2], intersection[1]);
            }
            else
            {
                // ending in a passing ramp
                if (intersection[1].entry_allowed)
                    intersection[1].turn.instruction = {
                        TurnType::NoTurn, getTurnDirection(intersection[1].turn.angle)};
                else
                    intersection[2].turn.instruction = {
                        TurnType::NoTurn, getTurnDirection(intersection[2].turn.angle)};
            }
        }
        else if (intersection.size() == 4 &&
                 detail::roadClass(intersection[1], node_based_graph) ==
                     detail::roadClass(intersection[2], node_based_graph) &&
                 detail::roadClass(intersection[2], node_based_graph) ==
                     detail::roadClass(intersection[3], node_based_graph))
        {
            // tripple fork at the end
            assignFork(via_edge, intersection[3], intersection[2], intersection[1]);
        }
        else if (countValid(intersection) > 0) // check whether turns exist at all
        {
            // FALLBACK, this should hopefully never be reached
            auto coord = localizer(node_based_graph.GetTarget(via_edge));
            util::SimpleLogger().Write(logWARNING)
                << "Fallback reached from motorway at " << std::setprecision(12)
                << toFloating(coord.lat) << " " << toFloating(coord.lon) << ", no continue angle, "
                << intersection.size() << " roads, " << countValid(intersection) << " valid ones.";
            fallbackTurnAssignmentMotorway(intersection);
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
                    BOOST_ASSERT(detail::isRampClass(road.turn.eid, node_based_graph));
                    road.turn.instruction =
                        TurnInstruction::SUPPRESSED(getTurnDirection(road.turn.angle));
                }
            }
        }
        else if (exiting_motorways == 1)
        {
            // normal motorway passing some ramps or mering onto another motorway
            if (intersection.size() == 2)
            {
                BOOST_ASSERT(!detail::isRampClass(intersection[1].turn.eid, node_based_graph));

                intersection[1].turn.instruction =
                    getInstructionForObvious(intersection.size(), via_edge, intersection[1]);
            }
            else
            {
                // continue on the same highway
                bool continues = (getContinueAngle(intersection) != intersection[0].turn.angle);
                // Normal Highway exit or merge
                for (auto &road : intersection)
                {
                    // ignore invalid uturns/other
                    if (!road.entry_allowed)
                        continue;

                    if (road.turn.angle == continue_angle)
                    {
                        if (continues)
                            road.turn.instruction =
                                TurnInstruction::SUPPRESSED(DirectionModifier::Straight);
                        else // TODO handle turn direction correctly
                            road.turn.instruction = {TurnType::Merge, DirectionModifier::Straight};
                    }
                    else if (road.turn.angle < continue_angle)
                    {
                        road.turn.instruction = {
                            detail::isRampClass(road.turn.eid, node_based_graph) ? TurnType::Ramp
                                                                                 : TurnType::Turn,
                            (road.turn.angle < 145) ? DirectionModifier::Right
                                                    : DirectionModifier::SlightRight};
                    }
                    else if (road.turn.angle > continue_angle)
                    {
                        road.turn.instruction = {
                            detail::isRampClass(road.turn.eid, node_based_graph) ? TurnType::Ramp
                                                                                 : TurnType::Turn,
                            (road.turn.angle > 215) ? DirectionModifier::Left
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
                intersection[1].turn.instruction =
                    getInstructionForObvious(intersection.size(), via_edge, intersection[1]);
                util::SimpleLogger().Write(logWARNING)
                    << "Disabled U-Turn on a freeway at "
                    << localizer(node_based_graph.GetTarget(via_edge));
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
                        detail::isMotorwayClass(intersection[i].turn.eid, node_based_graph))
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
                assignFork(via_edge, intersection[second_valid], intersection[first_valid]);
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
                        detail::isMotorwayClass(intersection[i].turn.eid, node_based_graph))
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
                assignFork(via_edge, intersection[third_valid], intersection[second_valid],
                           intersection[first_valid]);
            }
            else
            {
                auto coord = localizer(node_based_graph.GetTarget(via_edge));
                util::SimpleLogger().Write(logWARNING)
                    << "Found motorway junction with more than "
                       "2 exiting motorways or additional ramps at " << std::setprecision(12)
                    << toFloating(coord.lat) << " " << toFloating(coord.lon);
                fallbackTurnAssignmentMotorway(intersection);
            }
        } // done for more than one highway exit
    }
    return intersection;
}

std::vector<ConnectedRoad>
TurnAnalysis::handleMotorwayRamp(const EdgeID via_edge,
                                 std::vector<ConnectedRoad> intersection) const
{
    auto num_valid_turns = countValid(intersection);
    // ramp straight into a motorway/ramp
    if (intersection.size() == 2 && num_valid_turns == 1)
    {
        BOOST_ASSERT(!intersection[0].entry_allowed);
        BOOST_ASSERT(detail::isMotorwayClass(intersection[1].turn.eid, node_based_graph));

        intersection[1].turn.instruction =
            getInstructionForObvious(intersection.size(), via_edge, intersection[1]);
    }
    else if (intersection.size() == 3)
    {
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
                if (detail::isMotorwayClass(intersection[1].turn.eid, node_based_graph))
                {
                    // circular order indicates a merge to the left (0-3 onto 4
                    if (angularDeviation(intersection[1].turn.angle, STRAIGHT_ANGLE) <
                        NARROW_TURN_ANGLE)
                        intersection[1].turn.instruction = {TurnType::Merge,
                                                            DirectionModifier::SlightLeft};
                    else // fallback
                        intersection[1].turn.instruction = {
                            TurnType::Merge, getTurnDirection(intersection[1].turn.angle)};
                }
                else // passing by the end of a motorway
                    intersection[1].turn.instruction =
                        getInstructionForObvious(intersection.size(), via_edge, intersection[1]);
            }
            else
            {
                BOOST_ASSERT(intersection[2].entry_allowed);
                if (detail::isMotorwayClass(intersection[2].turn.eid, node_based_graph))
                {
                    // circular order (5-0) onto 4
                    if (angularDeviation(intersection[2].turn.angle, STRAIGHT_ANGLE) <
                        NARROW_TURN_ANGLE)
                        intersection[2].turn.instruction = {TurnType::Merge,
                                                            DirectionModifier::SlightRight};
                    else // fallback
                        intersection[2].turn.instruction = {
                            TurnType::Merge, getTurnDirection(intersection[2].turn.angle)};
                }
                else // passing the end of a highway
                    intersection[1].turn.instruction =
                        getInstructionForObvious(intersection.size(), via_edge, intersection[1]);
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
            if (detail::isMotorwayClass(intersection[1].turn.eid, node_based_graph) &&
                detail::isMotorwayClass(intersection[2].turn.eid, node_based_graph))
            {
                assignFork(via_edge, intersection[2], intersection[1]);
            }
            else
            {
                // continued ramp passing motorway entry
                //      M  R
                //      M  R
                //      | /
                //      R
                if (detail::isMotorwayClass(node_based_graph.GetEdgeData(intersection[1].turn.eid)
                                                .road_classification.road_class))
                {
                    intersection[1].turn.instruction = {TurnType::Merge,
                                                        DirectionModifier::SlightRight};
                    intersection[2].turn.instruction = {TurnType::Fork,
                                                        DirectionModifier::SlightLeft};
                }
                else
                {
                    intersection[1].turn.instruction = {TurnType::Fork,
                                                        DirectionModifier::SlightRight};
                    intersection[2].turn.instruction = {TurnType::Merge,
                                                        DirectionModifier::SlightLeft};
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
            const auto &edge_data = node_based_graph.GetEdgeData(road.turn.eid);
            if (!road.entry_allowed &&
                detail::isMotorwayClass(edge_data.road_classification.road_class))
            {
                passed_highway_entry = true;
            }
            else if (detail::isMotorwayClass(edge_data.road_classification.road_class))
            {
                road.turn.instruction = {TurnType::Merge,
                                         passed_highway_entry ? DirectionModifier::SlightRight
                                                              : DirectionModifier::SlightLeft};
            }
            else
            {
                BOOST_ASSERT(isRampClass(edge_data.road_classification.road_class));
                road.turn.instruction = {TurnType::Ramp, getTurnDirection(road.turn.angle)};
            }
        }
    }
    else
    { // FALLBACK, hopefully this should never been reached
        util::SimpleLogger().Write(logWARNING) << "Reached fallback on motorway ramp with "
                                               << intersection.size() << " roads and "
                                               << countValid(intersection) << " valid turns.";
        fallbackTurnAssignmentMotorway(intersection);
    }
    return intersection;
}

std::vector<ConnectedRoad>
TurnAnalysis::handleMotorwayJunction(const EdgeID via_edge,
                                     std::vector<ConnectedRoad> intersection) const
{
    // BOOST_ASSERT(!intersection[0].entry_allowed); //This fails due to @themarex handling of dead
    // end
    // streets
    const auto &in_data = node_based_graph.GetEdgeData(via_edge);

    // coming from motorway
    if (detail::isMotorwayClass(in_data.road_classification.road_class))
    {
        return handleFromMotorway(via_edge, std::move(intersection));
    }
    else // coming from a ramp
    {
        return handleMotorwayRamp(via_edge, std::move(intersection));
        // ramp merging straight onto motorway
    }
}

bool TurnAnalysis::isMotorwayJunction(const EdgeID via_edge,
                                      const std::vector<ConnectedRoad> &intersection) const
{
    bool has_motorway = false;
    bool has_normal_roads = false;

    for (const auto &road : intersection)
    {
        const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);
        // not merging or forking?
        if ((angularDeviation(road.turn.angle, 0) > 35 &&
             angularDeviation(road.turn.angle, 180) > 35) ||
            (road.entry_allowed && angularDeviation(road.turn.angle, 0) < 35))
            return false;
        else if (out_data.road_classification.road_class == FunctionalRoadClass::MOTORWAY ||
                 out_data.road_classification.road_class == FunctionalRoadClass::TRUNK)
        {
            if (road.entry_allowed)
                has_motorway = true;
        }
        else if (!isRampClass(out_data.road_classification.road_class))
            has_normal_roads = true;
    }

    if (has_normal_roads)
        return false;

    const auto &in_data = node_based_graph.GetEdgeData(via_edge);
    return has_motorway ||
           in_data.road_classification.road_class == FunctionalRoadClass::MOTORWAY ||
           in_data.road_classification.road_class == FunctionalRoadClass::TRUNK;
}

TurnType TurnAnalysis::findBasicTurnType(const EdgeID via_edge, const ConnectedRoad &road) const
{

    const auto &in_data = node_based_graph.GetEdgeData(via_edge);
    const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);

    bool on_ramp = isRampClass(in_data.road_classification.road_class);

    bool onto_ramp = isRampClass(out_data.road_classification.road_class);

    if (!on_ramp && onto_ramp)
        return TurnType::Ramp;

    if (in_data.name_id == out_data.name_id && in_data.name_id != INVALID_NAME_ID)
    {
        return TurnType::Continue;
    }

    return TurnType::Turn;
}

TurnInstruction TurnAnalysis::getInstructionForObvious(const std::size_t num_roads,
                                                       const EdgeID via_edge,
                                                       const ConnectedRoad &road) const
{
    const auto type = findBasicTurnType(via_edge, road);
    if (type == TurnType::Ramp)
    {
        return {TurnType::Ramp, getTurnDirection(road.turn.angle)};
    }

    if (angularDeviation(road.turn.angle, 0) < 0.01)
    {
        return {TurnType::Turn, DirectionModifier::UTurn};
    }
    if (type == TurnType::Turn)
    {
        const auto &in_data = node_based_graph.GetEdgeData(via_edge);
        const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);
        if (in_data.name_id != out_data.name_id &&
            requiresNameAnnounced(name_table.GetNameForID(in_data.name_id),
                                  name_table.GetNameForID(out_data.name_id)))
            return {TurnType::NewName, getTurnDirection(road.turn.angle)};
        else
            return {TurnType::Suppressed, getTurnDirection(road.turn.angle)};
    }
    BOOST_ASSERT(type == TurnType::Continue);
    if (num_roads > 2)
    {
        return {TurnType::Suppressed, getTurnDirection(road.turn.angle)};
    }
    else
    {
        return {TurnType::NoTurn, getTurnDirection(road.turn.angle)};
    }
}

std::vector<ConnectedRoad>
TurnAnalysis::handleOneWayTurn(std::vector<ConnectedRoad> intersection) const
{
    BOOST_ASSERT(intersection[0].turn.angle < 0.001);
    return intersection;
}

std::vector<ConnectedRoad>
TurnAnalysis::handleTwoWayTurn(const EdgeID via_edge, std::vector<ConnectedRoad> intersection) const
{
    BOOST_ASSERT(intersection[0].turn.angle < 0.001);
    intersection[1].turn.instruction =
        getInstructionForObvious(intersection.size(), via_edge, intersection[1]);

    if (intersection[1].turn.instruction.type == TurnType::Suppressed)
        intersection[1].turn.instruction.type = TurnType::NoTurn;

    return intersection;
}

std::vector<ConnectedRoad>
TurnAnalysis::handleThreeWayTurn(const EdgeID via_edge,
                                 std::vector<ConnectedRoad> intersection) const
{
    BOOST_ASSERT(intersection[0].turn.angle < 0.001);
    const auto isObviousOfTwo = [](const ConnectedRoad road, const ConnectedRoad other)
    {
        return (angularDeviation(road.turn.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
                angularDeviation(other.turn.angle, STRAIGHT_ANGLE) > 85) ||
               (angularDeviation(road.turn.angle, STRAIGHT_ANGLE) <
                std::numeric_limits<double>::epsilon()) ||
               (angularDeviation(other.turn.angle, STRAIGHT_ANGLE) /
                    angularDeviation(road.turn.angle, STRAIGHT_ANGLE) >
                1.4);
    };

    /* Two nearly straight turns -> FORK
                OOOOOOO
              /
       IIIIII
              \
                OOOOOOO
    */
    if (angularDeviation(intersection[1].turn.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
        angularDeviation(intersection[2].turn.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE)
    {
        if (intersection[1].entry_allowed && intersection[2].entry_allowed)
        {
            const auto left_class = node_based_graph.GetEdgeData(intersection[2].turn.eid)
                                        .road_classification.road_class;
            const auto right_class = node_based_graph.GetEdgeData(intersection[1].turn.eid)
                                         .road_classification.road_class;
            if (canBeSeenAsFork(left_class, right_class))
                assignFork(via_edge, intersection[2], intersection[1]);
            else if (getPriority(left_class) > getPriority(right_class))
            {
                intersection[1].turn.instruction =
                    getInstructionForObvious(intersection.size(), via_edge, intersection[1]);
                intersection[2].turn.instruction = {findBasicTurnType(via_edge, intersection[2]),
                                                    DirectionModifier::SlightLeft};
            }
            else
            {
                intersection[2].turn.instruction =
                    getInstructionForObvious(intersection.size(), via_edge, intersection[2]);
                intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                    DirectionModifier::SlightRight};
            }
        }
        else
        {
            if (intersection[1].entry_allowed)
                intersection[1].turn.instruction =
                    getInstructionForObvious(intersection.size(), via_edge, intersection[1]);
            if (intersection[2].entry_allowed)
                intersection[2].turn.instruction =
                    getInstructionForObvious(intersection.size(), via_edge, intersection[2]);
        }
    }
    /*  T Intersection

      OOOOOOO T OOOOOOOO
              I
              I
              I
    */
    else if (angularDeviation(intersection[1].turn.angle, 90) < NARROW_TURN_ANGLE &&
             angularDeviation(intersection[2].turn.angle, 270) < NARROW_TURN_ANGLE &&
             angularDeviation(intersection[1].turn.angle, intersection[2].turn.angle) >
                 NARROW_TURN_ANGLE)
    {
        if (intersection[1].entry_allowed)
        {
            if (TurnType::Ramp != findBasicTurnType(via_edge, intersection[1]))
                intersection[1].turn.instruction = {TurnType::EndOfRoad, DirectionModifier::Right};
            else
                intersection[1].turn.instruction = {TurnType::Ramp, DirectionModifier::Right};
        }
        if (intersection[2].entry_allowed)
        {
            if (TurnType::Ramp != findBasicTurnType(via_edge, intersection[2]))

                intersection[2].turn.instruction = {TurnType::EndOfRoad, DirectionModifier::Left};
            else
                intersection[2].turn.instruction = {TurnType::Ramp, DirectionModifier::Left};
        }
    }
    /* T Intersection, Cross left
              O
              O
              O
     IIIIIIII - OOOOOOOOOO
    */
    else if (angularDeviation(intersection[1].turn.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
             angularDeviation(intersection[2].turn.angle, 270) < NARROW_TURN_ANGLE &&
             angularDeviation(intersection[1].turn.angle, intersection[2].turn.angle) >
                 NARROW_TURN_ANGLE)
    {
        if (intersection[1].entry_allowed)
        {
            if (TurnType::Ramp != findBasicTurnType(via_edge, intersection[1]))
                intersection[1].turn.instruction =
                    getInstructionForObvious(intersection.size(), via_edge, intersection[1]);
            else
                intersection[1].turn.instruction = {TurnType::Ramp, DirectionModifier::Straight};
        }
        if (intersection[2].entry_allowed)
        {
            intersection[2].turn.instruction = {findBasicTurnType(via_edge, intersection[2]),
                                                DirectionModifier::Left};
        }
    }
    /* T Intersection, Cross right

     IIIIIIII T OOOOOOOOOO
              O
              O
              O
    */
    else if (angularDeviation(intersection[2].turn.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
             angularDeviation(intersection[1].turn.angle, 90) < NARROW_TURN_ANGLE &&
             angularDeviation(intersection[1].turn.angle, intersection[2].turn.angle) >
                 NARROW_TURN_ANGLE)
    {
        if (intersection[2].entry_allowed)
            intersection[2].turn.instruction =
                getInstructionForObvious(intersection.size(), via_edge, intersection[2]);
        if (intersection[1].entry_allowed)
            intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                DirectionModifier::Right};
    }
    // merge onto a through street
    else if (INVALID_NAME_ID != node_based_graph.GetEdgeData(intersection[1].turn.eid).name_id &&
             node_based_graph.GetEdgeData(intersection[1].turn.eid).name_id ==
                 node_based_graph.GetEdgeData(intersection[2].turn.eid).name_id)
    {
        const auto findTurn = [isObviousOfTwo](const ConnectedRoad turn, const ConnectedRoad other)
                                  -> TurnInstruction
        {
            return {isObviousOfTwo(turn, other) ? TurnType::Merge : TurnType::Turn,
                    getTurnDirection(turn.turn.angle)};
        };
        intersection[1].turn.instruction = findTurn(intersection[1], intersection[2]);
        intersection[2].turn.instruction = findTurn(intersection[2], intersection[1]);
    }

    // other street merges from the left
    else if (INVALID_NAME_ID != node_based_graph.GetEdgeData(via_edge).name_id &&
             node_based_graph.GetEdgeData(via_edge).name_id ==
                 node_based_graph.GetEdgeData(intersection[1].turn.eid).name_id)
    {
        if (isObviousOfTwo(intersection[1], intersection[2]))
        {
            intersection[1].turn.instruction =
                TurnInstruction::SUPPRESSED(DirectionModifier::Straight);
        }
        else
        {
            intersection[1].turn.instruction = {TurnType::Continue,
                                                getTurnDirection(intersection[1].turn.angle)};
        }
        intersection[2].turn.instruction = {TurnType::Turn,
                                            getTurnDirection(intersection[2].turn.angle)};
    }
    // other street merges from the right
    else if (INVALID_NAME_ID != node_based_graph.GetEdgeData(via_edge).name_id &&
             node_based_graph.GetEdgeData(via_edge).name_id ==
                 node_based_graph.GetEdgeData(intersection[2].turn.eid).name_id)
    {
        if (isObviousOfTwo(intersection[2], intersection[1]))
        {
            intersection[2].turn.instruction =
                TurnInstruction::SUPPRESSED(DirectionModifier::Straight);
        }
        else
        {
            intersection[2].turn.instruction = {TurnType::Continue,
                                                getTurnDirection(intersection[2].turn.angle)};
        }
        intersection[1].turn.instruction = {TurnType::Turn,
                                            getTurnDirection(intersection[1].turn.angle)};
    }
    else
    {
        const unsigned in_name_id = node_based_graph.GetEdgeData(via_edge).name_id;
        const unsigned out_names[2] = {
            node_based_graph.GetEdgeData(intersection[1].turn.eid).name_id,
            node_based_graph.GetEdgeData(intersection[2].turn.eid).name_id};
        if (isObviousOfTwo(intersection[1], intersection[2]))
        {
            intersection[1].turn.instruction = {
                (in_name_id != INVALID_NAME_ID || out_names[0] != INVALID_NAME_ID)
                    ? TurnType::NewName
                    : TurnType::NoTurn,
                getTurnDirection(intersection[1].turn.angle)};
        }
        else
        {
            intersection[1].turn.instruction = {TurnType::Turn,
                                                getTurnDirection(intersection[1].turn.angle)};
        }

        if (isObviousOfTwo(intersection[2], intersection[1]))
        {
            intersection[2].turn.instruction = {
                (in_name_id != INVALID_NAME_ID || out_names[1] != INVALID_NAME_ID)
                    ? TurnType::NewName
                    : TurnType::NoTurn,
                getTurnDirection(intersection[2].turn.angle)};
        }
        else
        {
            intersection[2].turn.instruction = {TurnType::Turn,
                                                getTurnDirection(intersection[2].turn.angle)};
        }
    }
    // unnamed intersections or basic three way turn

    // remain at basic turns
    // TODO handle obviousness, Handle Merges
    return intersection;
}

void TurnAnalysis::handleDistinctConflict(const EdgeID via_edge,
                                          ConnectedRoad &left,
                                          ConnectedRoad &right) const
{
    // single turn of both is valid (don't change the valid one)
    // or multiple identical angles -> bad OSM intersection
    if ((!left.entry_allowed || !right.entry_allowed) || (left.turn.angle == right.turn.angle))
    {
        if (left.entry_allowed)
            left.turn.instruction = {findBasicTurnType(via_edge, left),
                                     getTurnDirection(left.turn.angle)};
        if (right.entry_allowed)
            right.turn.instruction = {findBasicTurnType(via_edge, right),
                                      getTurnDirection(right.turn.angle)};
        return;
    }

    if (getTurnDirection(left.turn.angle) == DirectionModifier::Straight ||
        getTurnDirection(left.turn.angle) == DirectionModifier::SlightLeft ||
        getTurnDirection(right.turn.angle) == DirectionModifier::SlightRight)
    {
        const auto left_class =
            node_based_graph.GetEdgeData(left.turn.eid).road_classification.road_class;
        const auto right_class =
            node_based_graph.GetEdgeData(right.turn.eid).road_classification.road_class;
        if (canBeSeenAsFork(left_class, right_class))
            assignFork(via_edge, left, right);
        else if (getPriority(left_class) > getPriority(right_class))
        {
            // FIXME this should possibly know about the actual roads?
            right.turn.instruction = getInstructionForObvious(4, via_edge, right);
            left.turn.instruction = {findBasicTurnType(via_edge, left),
                                     DirectionModifier::SlightLeft};
        }
        else
        {
            // FIXME this should possibly know about the actual roads?
            left.turn.instruction = getInstructionForObvious(4, via_edge, left);
            right.turn.instruction = {findBasicTurnType(via_edge, right),
                                      DirectionModifier::SlightRight};
        }
    }

    const auto left_type = findBasicTurnType(via_edge, left);
    const auto right_type = findBasicTurnType(via_edge, right);
    // Two Right Turns
    if (angularDeviation(left.turn.angle, 90) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
    {
        // Keep left perfect, shift right
        left.turn.instruction = {left_type, DirectionModifier::Right};
        right.turn.instruction = {right_type, DirectionModifier::SharpRight};
        return;
    }
    if (angularDeviation(right.turn.angle, 90) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
    {
        // Keep Right perfect, shift left
        left.turn.instruction = {left_type, DirectionModifier::SlightRight};
        right.turn.instruction = {right_type, DirectionModifier::Right};
        return;
    }
    // Two Right Turns
    if (angularDeviation(left.turn.angle, 270) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
    {
        // Keep left perfect, shift right
        left.turn.instruction = {left_type, DirectionModifier::Left};
        right.turn.instruction = {right_type, DirectionModifier::SlightLeft};
        return;
    }
    if (angularDeviation(right.turn.angle, 270) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
    {
        // Keep Right perfect, shift left
        left.turn.instruction = {left_type, DirectionModifier::SharpLeft};
        right.turn.instruction = {right_type, DirectionModifier::Left};
        return;
    }
    // Both turns?
    if (TurnType::Ramp != left_type && TurnType::Ramp != right_type)
    {
        if (left.turn.angle < STRAIGHT_ANGLE)
        {
            left.turn.instruction = {TurnType::FirstTurn, getTurnDirection(left.turn.angle)};
            right.turn.instruction = {TurnType::SecondTurn, getTurnDirection(right.turn.angle)};
        }
        else
        {
            left.turn.instruction = {TurnType::SecondTurn, getTurnDirection(left.turn.angle)};
            right.turn.instruction = {TurnType::FirstTurn, getTurnDirection(right.turn.angle)};
        }
        return;
    }
    // Shift the lesser penalty
    if (getTurnDirection(left.turn.angle) == DirectionModifier::SharpLeft)
    {
        left.turn.instruction = {left_type, DirectionModifier::SharpLeft};
        right.turn.instruction = {right_type, DirectionModifier::Left};
        return;
    }
    if (getTurnDirection(right.turn.angle) == DirectionModifier::SharpRight)
    {
        left.turn.instruction = {left_type, DirectionModifier::Right};
        right.turn.instruction = {right_type, DirectionModifier::SharpRight};
        return;
    }

    if (getTurnDirection(left.turn.angle) == DirectionModifier::Right)
    {
        if (angularDeviation(left.turn.angle, 90) > angularDeviation(right.turn.angle, 90))
        {
            left.turn.instruction = {left_type, DirectionModifier::SlightRight};
            right.turn.instruction = {right_type, DirectionModifier::Right};
        }
        else
        {
            left.turn.instruction = {left_type, DirectionModifier::Right};
            right.turn.instruction = {right_type, DirectionModifier::SharpRight};
        }
    }
    else
    {
        if (angularDeviation(left.turn.angle, 270) > angularDeviation(right.turn.angle, 270))
        {
            left.turn.instruction = {left_type, DirectionModifier::SharpLeft};
            right.turn.instruction = {right_type, DirectionModifier::Left};
        }
        else
        {
            left.turn.instruction = {left_type, DirectionModifier::Left};
            right.turn.instruction = {right_type, DirectionModifier::SlightLeft};
        }
    }
}

std::vector<ConnectedRoad>
TurnAnalysis::handleComplexTurn(const EdgeID via_edge,
                                std::vector<ConnectedRoad> intersection) const
{
    static int fallback_count = 0;
    const std::size_t obvious_index = findObviousTurn(via_edge, intersection);
    const auto fork_range = findFork(intersection);
    std::size_t straightmost_turn = 0;
    double straightmost_deviation = 180;
    for (std::size_t i = 0; i < intersection.size(); ++i)
    {
        const double deviation = angularDeviation(intersection[i].turn.angle, STRAIGHT_ANGLE);
        if (deviation < straightmost_deviation)
        {
            straightmost_deviation = deviation;
            straightmost_turn = i;
        }
    }

    if (obvious_index != 0)
    {
        intersection[obvious_index].turn.instruction =
            getInstructionForObvious(intersection.size(), via_edge, intersection[obvious_index]);

        // assign left/right turns
        intersection = assignLeftTurns(via_edge, std::move(intersection), obvious_index + 1);
        intersection = assignRightTurns(via_edge, std::move(intersection), obvious_index);
    }
    else if (fork_range.first != 0 && fork_range.second - fork_range.first <= 2) // found fork
    {
        if (fork_range.second - fork_range.first == 1)
        {
            auto &left = intersection[fork_range.second];
            auto &right = intersection[fork_range.first];
            const auto left_class =
                node_based_graph.GetEdgeData(left.turn.eid).road_classification.road_class;
            const auto right_class =
                node_based_graph.GetEdgeData(right.turn.eid).road_classification.road_class;
            if (canBeSeenAsFork(left_class, right_class))
                assignFork(via_edge, left, right);
            else if (getPriority(left_class) > getPriority(right_class))
            {
                right.turn.instruction =
                    getInstructionForObvious(intersection.size(), via_edge, right);
                left.turn.instruction = {findBasicTurnType(via_edge, left),
                                         DirectionModifier::SlightLeft};
            }
            else
            {
                left.turn.instruction =
                    getInstructionForObvious(intersection.size(), via_edge, left);
                right.turn.instruction = {findBasicTurnType(via_edge, right),
                                          DirectionModifier::SlightRight};
            }
        }
        else if (fork_range.second - fork_range.second == 2)
        {
            assignFork(via_edge, intersection[fork_range.second],
                       intersection[fork_range.first + 1], intersection[fork_range.first]);
        }
        // assign left/right turns
        intersection = assignLeftTurns(via_edge, std::move(intersection), fork_range.second + 1);
        intersection = assignRightTurns(via_edge, std::move(intersection), fork_range.first);
    }
    else if (straightmost_deviation < FUZZY_ANGLE_DIFFERENCE &&
             !intersection[straightmost_turn].entry_allowed)
    {
        // invalid straight turn
        intersection = assignLeftTurns(via_edge, std::move(intersection), straightmost_turn + 1);
        intersection = assignRightTurns(via_edge, std::move(intersection), straightmost_turn);
    }
    // no straight turn
    else if (intersection[straightmost_turn].turn.angle > 180)
    {
        // at most three turns on either side
        intersection = assignLeftTurns(via_edge, std::move(intersection), straightmost_turn);
        intersection = assignRightTurns(via_edge, std::move(intersection), straightmost_turn);
    }
    else if (intersection[straightmost_turn].turn.angle < 180)
    {
        intersection = assignLeftTurns(via_edge, std::move(intersection), straightmost_turn + 1);
        intersection = assignRightTurns(via_edge, std::move(intersection), straightmost_turn + 1);
    }
    else
    {
        if (fallback_count++ < 10)
        {
            const auto coord = localizer(node_based_graph.GetTarget(via_edge));
            util::SimpleLogger().Write(logWARNING)
                << "Resolved to keep fallback on complex turn assignment at "
                << std::setprecision(12) << toFloating(coord.lat) << " " << toFloating(coord.lon)
                << "Straightmost: " << straightmost_turn;
            ;
            for (const auto &road : intersection)
            {
                const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);
                util::SimpleLogger().Write(logWARNING)
                    << "road: " << road.toString() << " Name: " << out_data.name_id
                    << " Road Class: " << (int)out_data.road_classification.road_class
                    << " At: " << localizer(node_based_graph.GetTarget(road.turn.eid));
            }
        }
    }
    return intersection;
}

// Sets basic turn types as fallback for otherwise unhandled turns
std::vector<ConnectedRoad> TurnAnalysis::setTurnTypes(const NodeID from,
                                                      const EdgeID via_edge,
                                                      std::vector<ConnectedRoad> intersection) const
{
    for (auto &road : intersection)
    {
        if (!road.entry_allowed)
            continue;

        const EdgeID onto_edge = road.turn.eid;
        const NodeID to_node = node_based_graph.GetTarget(onto_edge);

        road.turn.instruction = (from == to_node)
                                    ? TurnInstruction{TurnType::Turn, DirectionModifier::UTurn}
                                    : TurnInstruction{findBasicTurnType(via_edge, road),
                                                      getTurnDirection(road.turn.angle)};
    }
    return intersection;
}

//                                               a
//                                               |
//                                               |
//                                               v
// For an intersection from_node --via_edi--> turn_node ----> c
//                                               ^
//                                               |
//                                               |
//                                               b
// This functions returns _all_ turns as if the graph was undirected.
// That means we not only get (from_node, turn_node, c) in the above example
// but also (from_node, turn_node, a), (from_node, turn_node, b). These turns are
// marked as invalid and only needed for intersection classification.
std::vector<ConnectedRoad> TurnAnalysis::getConnectedRoads(const NodeID from_node,
                                                           const EdgeID via_eid) const
{
    std::vector<ConnectedRoad> intersection;
    const NodeID turn_node = node_based_graph.GetTarget(via_eid);
    const NodeID only_restriction_to_node =
        restriction_map.CheckForEmanatingIsOnlyTurn(from_node, turn_node);
    const bool is_barrier_node = barrier_nodes.find(turn_node) != barrier_nodes.end();

    bool has_uturn_edge = false;
    for (const EdgeID onto_edge : node_based_graph.GetAdjacentEdgeRange(turn_node))
    {
        BOOST_ASSERT(onto_edge != SPECIAL_EDGEID);
        const NodeID to_node = node_based_graph.GetTarget(onto_edge);

        bool turn_is_valid =
            // reverse edges are never valid turns because the resulting turn would look like this:
            // from_node --via_edge--> turn_node <--onto_edge-- to_node
            // however we need this for capture intersection shape for incoming one-ways
            !node_based_graph.GetEdgeData(onto_edge).reversed &&
            // we are not turning over a barrier
            (!is_barrier_node || from_node == to_node) &&
            // We are at an only_-restriction but not at the right turn.
            (only_restriction_to_node == SPECIAL_NODEID || to_node == only_restriction_to_node) &&
            // the turn is not restricted
            !restriction_map.CheckIfTurnIsRestricted(from_node, turn_node, to_node);

        auto angle = 0.;
        if (from_node == to_node)
        {
            if (turn_is_valid && !is_barrier_node)
            {
                // we only add u-turns for dead-end streets.
                if (node_based_graph.GetOutDegree(turn_node) > 1)
                {
                    auto number_of_emmiting_bidirectional_edges = 0;
                    for (auto edge : node_based_graph.GetAdjacentEdgeRange(turn_node))
                    {
                        auto target = node_based_graph.GetTarget(edge);
                        auto reverse_edge = node_based_graph.FindEdge(target, turn_node);
                        BOOST_ASSERT(reverse_edge != SPECIAL_EDGEID);
                        if (!node_based_graph.GetEdgeData(reverse_edge).reversed)
                        {
                            ++number_of_emmiting_bidirectional_edges;
                        }
                    }
                    // is a dead-end
                    turn_is_valid = number_of_emmiting_bidirectional_edges <= 1;
                }
            }
            has_uturn_edge = true;
            BOOST_ASSERT(angle >= 0. && angle < std::numeric_limits<double>::epsilon());
        }
        else
        {
            // unpack first node of second segment if packed
            const auto first_coordinate = getRepresentativeCoordinate(
                from_node, turn_node, via_eid, INVERT, compressed_edge_container, node_info_list);
            const auto third_coordinate = getRepresentativeCoordinate(
                turn_node, to_node, onto_edge, !INVERT, compressed_edge_container, node_info_list);
            angle = util::coordinate_calculation::computeAngle(
                first_coordinate, node_info_list[turn_node], third_coordinate);
            if (angle < std::numeric_limits<double>::epsilon())
                has_uturn_edge = true;
        }

        intersection.push_back(ConnectedRoad(
            TurnOperation{onto_edge, angle, {TurnType::Invalid, DirectionModifier::UTurn}},
            turn_is_valid));
    }

    // We hit the case of a street leading into nothing-ness. Since the code here assumes that this
    // will
    // never happen we add an artificial invalid uturn in this case.
    if (!has_uturn_edge)
    {
        intersection.push_back(
            {TurnOperation{via_eid, 0., {TurnType::Invalid, DirectionModifier::UTurn}}, false});
    }

    const auto ByAngle = [](const ConnectedRoad &first, const ConnectedRoad second)
    {
        return first.turn.angle < second.turn.angle;
    };
    std::sort(std::begin(intersection), std::end(intersection), ByAngle);

    BOOST_ASSERT(intersection[0].turn.angle >= 0. &&
                 intersection[0].turn.angle < std::numeric_limits<double>::epsilon());

    return mergeSegregatedRoads(std::move(intersection));
}

/*
 * Segregated Roads often merge onto a single intersection.
 * While technically representing different roads, they are
 * often looked at as a single road.
 * Due to the merging, turn Angles seem off, wenn we compute them from the
 * initial positions.
 *
 *         b<b<b<b(1)<b<b<b
 * aaaaa-b
 *         b>b>b>b(2)>b>b>b
 *
 * Would be seen as a slight turn going fro a to (2). A Sharp turn going from
 * (1) to (2).
 *
 * In cases like these, we megre this segregated roads into a single road to
 * end up with a case like:
 *
 * aaaaa-bbbbbb
 *
 * for the turn representation.
 * Anything containing the first u-turn in a merge affects all other angles
 * and is handled separately from all others.
 */
std::vector<ConnectedRoad>
TurnAnalysis::mergeSegregatedRoads(std::vector<ConnectedRoad> intersection) const
{
    const auto getRight = [&](std::size_t index)
    {
        return (index + intersection.size() - 1) % intersection.size();
    };

    const auto mergable = [&](std::size_t first, std::size_t second) -> bool
    {
        const auto &first_data = node_based_graph.GetEdgeData(intersection[first].turn.eid);
        const auto &second_data = node_based_graph.GetEdgeData(intersection[second].turn.eid);

        return first_data.name_id != INVALID_NAME_ID && first_data.name_id == second_data.name_id &&
               !first_data.roundabout && !second_data.roundabout &&
               first_data.travel_mode == second_data.travel_mode &&
               first_data.road_classification == second_data.road_classification &&
               // compatible threshold
               angularDeviation(intersection[first].turn.angle, intersection[second].turn.angle) <
                   60 &&
               first_data.reversed != second_data.reversed;
    };

    const auto merge = [](const ConnectedRoad &first, const ConnectedRoad &second) -> ConnectedRoad
    {
        if (!first.entry_allowed)
        {
            ConnectedRoad result = second;
            result.turn.angle = (first.turn.angle + second.turn.angle) / 2;
            if (first.turn.angle - second.turn.angle > 180)
                result.turn.angle += 180;
            if (result.turn.angle > 360)
                result.turn.angle -= 360;

            return result;
        }
        else
        {
            BOOST_ASSERT(!second.entry_allowed);
            ConnectedRoad result = first;
            result.turn.angle = (first.turn.angle + second.turn.angle) / 2;

            if (first.turn.angle - second.turn.angle > 180)
                result.turn.angle += 180;
            if (result.turn.angle > 360)
                result.turn.angle -= 360;

            return result;
        }
    };
    if (intersection.size() == 1)
        return intersection;

    // check for merges including the basic u-turn
    // these result in an adjustment of all other angles
    if (mergable(0, intersection.size() - 1))
    {
        // std::cout << "First merge" << std::endl;
        const double correction_factor =
            (360 - intersection[intersection.size() - 1].turn.angle) / 2;
        for (std::size_t i = 1; i + 1 < intersection.size(); ++i)
            intersection[i].turn.angle += correction_factor;
        intersection[0] = merge(intersection.front(), intersection.back());
        intersection[0].turn.angle = 0;
        intersection.pop_back();
    }
    else if (mergable(0, 1))
    {
        // std::cout << "First merge" << std::endl;
        const double correction_factor = (intersection[1].turn.angle) / 2;
        for (std::size_t i = 2; i < intersection.size(); ++i)
            intersection[i].turn.angle += correction_factor;
        intersection[0] = merge(intersection[0], intersection[1]);
        intersection[0].turn.angle = 0;
        intersection.erase(intersection.begin() + 1);
    }

    // a merge including the first u-turn requres an adjustment of the turn angles
    // therefore these are handled prior to this step
    for (std::size_t index = 2; index < intersection.size(); ++index)
    {
        if (mergable(index, getRight(index)))
        {
            intersection[getRight(index)] =
                merge(intersection[getRight(index)], intersection[index]);
            intersection.erase(intersection.begin() + index);
            --index;
        }
    }

    const auto ByAngle = [](const ConnectedRoad &first, const ConnectedRoad second)
    {
        return first.turn.angle < second.turn.angle;
    };
    std::sort(std::begin(intersection), std::end(intersection), ByAngle);
    return intersection;
}

void TurnAnalysis::assignFork(const EdgeID via_edge,
                              ConnectedRoad &left,
                              ConnectedRoad &right) const
{
    const auto &in_data = node_based_graph.GetEdgeData(via_edge);
    const bool low_priority_left = isLowPriorityRoadClass(
        node_based_graph.GetEdgeData(left.turn.eid).road_classification.road_class);
    const bool low_priority_right = isLowPriorityRoadClass(
        node_based_graph.GetEdgeData(right.turn.eid).road_classification.road_class);
    if ((angularDeviation(left.turn.angle, STRAIGHT_ANGLE) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
         angularDeviation(right.turn.angle, STRAIGHT_ANGLE) > FUZZY_ANGLE_DIFFERENCE))
    {
        // left side is actually straight
        const auto &out_data = node_based_graph.GetEdgeData(left.turn.eid);
        if (requiresAnnouncement(in_data, out_data))
        {
            if (low_priority_right && !low_priority_left)
            {
                left.turn.instruction = getInstructionForObvious(3, via_edge, left);
                right.turn.instruction = {findBasicTurnType(via_edge, right),
                                          DirectionModifier::SlightRight};
            }
            else
            {
                if (low_priority_left && !low_priority_right)
                {
                    left.turn.instruction = {findBasicTurnType(via_edge, left),
                                             DirectionModifier::SlightLeft};
                    right.turn.instruction = {findBasicTurnType(via_edge, right),
                                              DirectionModifier::SlightRight};
                }
                else
                {
                    left.turn.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
                    right.turn.instruction = {TurnType::Fork, DirectionModifier::SlightRight};
                }
            }
        }
        else
        {
            left.turn.instruction = {TurnType::Suppressed, DirectionModifier::Straight};
            right.turn.instruction = {findBasicTurnType(via_edge, right),
                                      DirectionModifier::SlightRight};
        }
    }
    else if (angularDeviation(right.turn.angle, STRAIGHT_ANGLE) <
                 MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
             angularDeviation(left.turn.angle, STRAIGHT_ANGLE) > FUZZY_ANGLE_DIFFERENCE)
    {
        // right side is actually straight
        const auto &out_data = node_based_graph.GetEdgeData(right.turn.eid);
        if (angularDeviation(right.turn.angle, STRAIGHT_ANGLE) <
                MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
            angularDeviation(left.turn.angle, STRAIGHT_ANGLE) > FUZZY_ANGLE_DIFFERENCE)
        {
            if (requiresAnnouncement(in_data, out_data))
            {
                if (low_priority_left && !low_priority_right)
                {
                    left.turn.instruction = {findBasicTurnType(via_edge, left),
                                             DirectionModifier::SlightLeft};
                    right.turn.instruction = getInstructionForObvious(3, via_edge, right);
                }
                else
                {
                    if (low_priority_right && !low_priority_left)
                    {
                        left.turn.instruction = {findBasicTurnType(via_edge, left),
                                                 DirectionModifier::SlightLeft};
                        right.turn.instruction = {findBasicTurnType(via_edge, right),
                                                  DirectionModifier::SlightRight};
                    }
                    else
                    {
                        right.turn.instruction = {TurnType::Fork, DirectionModifier::SlightRight};
                        left.turn.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
                    }
                }
            }
            else
            {
                right.turn.instruction = {TurnType::Suppressed, DirectionModifier::Straight};
                left.turn.instruction = {findBasicTurnType(via_edge, left),
                                         DirectionModifier::SlightLeft};
            }
        }
    }
    else
    {
        // left side of fork
        if (low_priority_right && !low_priority_left)
            left.turn.instruction = {TurnType::Suppressed, DirectionModifier::SlightLeft};
        else
        {
            if (low_priority_left && !low_priority_right)
                left.turn.instruction = {TurnType::Turn, DirectionModifier::SlightLeft};
            else
                left.turn.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
        }

        // right side of fork
        if (low_priority_left && !low_priority_right)
            right.turn.instruction = {TurnType::Suppressed, DirectionModifier::SlightLeft};
        else
        {
            if (low_priority_right && !low_priority_left)
                right.turn.instruction = {TurnType::Turn, DirectionModifier::SlightRight};
            else
                right.turn.instruction = {TurnType::Fork, DirectionModifier::SlightRight};
        }
    }
}

void TurnAnalysis::assignFork(const EdgeID via_edge,
                              ConnectedRoad &left,
                              ConnectedRoad &center,
                              ConnectedRoad &right) const
{
    // TODO handle low priority road classes in a reasonable way
    if (left.entry_allowed && center.entry_allowed && right.entry_allowed)
    {
        left.turn.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
        if (angularDeviation(center.turn.angle, 180) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
        {
            const auto &in_data = node_based_graph.GetEdgeData(via_edge);
            const auto &out_data = node_based_graph.GetEdgeData(center.turn.eid);
            if (requiresAnnouncement(in_data, out_data))
            {
                center.turn.instruction = {TurnType::Fork, DirectionModifier::Straight};
            }
            else
            {
                center.turn.instruction = {TurnType::Suppressed, DirectionModifier::Straight};
            }
        }
        else
        {
            center.turn.instruction = {TurnType::Fork, DirectionModifier::Straight};
        }
        right.turn.instruction = {TurnType::Fork, DirectionModifier::SlightRight};
    }
    else if (left.entry_allowed)
    {
        if (right.entry_allowed)
            assignFork(via_edge, left, right);
        else if (center.entry_allowed)
            assignFork(via_edge, left, center);
        else
            left.turn.instruction = {findBasicTurnType(via_edge, left),
                                     getTurnDirection(left.turn.angle)};
    }
    else if (right.entry_allowed)
    {
        if (center.entry_allowed)
            assignFork(via_edge, center, right);
        else
            right.turn.instruction = {findBasicTurnType(via_edge, right),
                                      getTurnDirection(right.turn.angle)};
    }
    else
    {
        if (center.entry_allowed)
            center.turn.instruction = {findBasicTurnType(via_edge, center),
                                       getTurnDirection(center.turn.angle)};
    }
}

std::size_t TurnAnalysis::findObviousTurn(const EdgeID via_edge,
                                          const std::vector<ConnectedRoad> &intersection) const
{
    // no obvious road
    if (intersection.size() == 1)
        return 0;

    // a single non u-turn is obvious
    if (intersection.size() == 2)
        return 1;

    // at least three roads
    std::size_t best = 0;
    double best_deviation = 180;

    std::size_t best_continue = 0;
    double best_continue_deviation = 180;

    const EdgeData &in_data = node_based_graph.GetEdgeData(via_edge);
    for (std::size_t i = 1; i < intersection.size(); ++i)
    {
        const double deviation = angularDeviation(intersection[i].turn.angle, STRAIGHT_ANGLE);
        if (intersection[i].entry_allowed && deviation < best_deviation)
        {
            best_deviation = deviation;
            best = i;
        }

        const auto out_data = node_based_graph.GetEdgeData(intersection[i].turn.eid);
        if (intersection[i].entry_allowed && out_data.name_id == in_data.name_id &&
            deviation < best_continue_deviation)
        {
            best_continue_deviation = deviation;
            best_continue = i;
        }
    }

    if (best == 0)
        return 0;

    if (best_deviation >= 2 * NARROW_TURN_ANGLE)
        return 0;

    // TODO incorporate road class in decision
    if (best != 0 && best_deviation < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
    {
        return best;
    }

    // has no obvious continued road
    if (best_continue == 0 || true)
    {
        // Find left/right deviation
        const double left_deviation = angularDeviation(
            intersection[(best + 1) % intersection.size()].turn.angle, STRAIGHT_ANGLE);
        const double right_deviation =
            angularDeviation(intersection[best - 1].turn.angle, STRAIGHT_ANGLE);

        if (best_deviation < MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
            std::min(left_deviation, right_deviation) > FUZZY_ANGLE_DIFFERENCE)
            return best;

        // other narrow turns?
        if (angularDeviation(intersection[best - 1].turn.angle, STRAIGHT_ANGLE) <=
            FUZZY_ANGLE_DIFFERENCE)
            return 0;
        if (angularDeviation(intersection[(best + 1) % intersection.size()].turn.angle,
                             STRAIGHT_ANGLE) <= FUZZY_ANGLE_DIFFERENCE)
            return 0;

        // Well distinct turn that is nearly straight
        if (left_deviation / best_deviation >= DISTINCTION_RATIO &&
            right_deviation / best_deviation >= DISTINCTION_RATIO)
        {
            return best;
        }
    }

    return 0; // no obvious turn
}

std::pair<std::size_t, std::size_t>
TurnAnalysis::findFork(const std::vector<ConnectedRoad> &intersection) const
{

    std::size_t best = 0;
    double best_deviation = 180;

    // TODO handle road classes
    for (std::size_t i = 1; i < intersection.size(); ++i)
    {
        const double deviation = angularDeviation(intersection[i].turn.angle, STRAIGHT_ANGLE);
        if (intersection[i].entry_allowed && deviation < best_deviation)
        {
            best_deviation = deviation;
            best = i;
        }
    }

    if (best_deviation <= NARROW_TURN_ANGLE)
    {
        std::size_t left = best, right = best;
        while (left + 1 < intersection.size() &&
               angularDeviation(intersection[left].turn.angle, intersection[left + 1].turn.angle) <
                   NARROW_TURN_ANGLE)
            ++left;
        while (right > 1 &&
               angularDeviation(intersection[right].turn.angle,
                                intersection[right - 1].turn.angle) < NARROW_TURN_ANGLE)
            --right;

        // TODO check whether 2*NARROW_TURN is too large
        if (right < left &&
            angularDeviation(intersection[left].turn.angle,
                             intersection[(left + 1) % intersection.size()].turn.angle) >=
                2 * NARROW_TURN_ANGLE &&
            angularDeviation(intersection[right].turn.angle, intersection[right - 1].turn.angle) >=
                2 * NARROW_TURN_ANGLE)
            return std::make_pair(right, left);
    }
    return std::make_pair(0llu, 0llu);
}

// Can only assign three turns
std::vector<ConnectedRoad> TurnAnalysis::assignLeftTurns(const EdgeID via_edge,
                                                         std::vector<ConnectedRoad> intersection,
                                                         const std::size_t starting_at) const
{
    const auto count_valid = [&intersection, starting_at]()
    {
        std::size_t count = 0;
        for (std::size_t i = starting_at; i < intersection.size(); ++i)
            if (intersection[i].entry_allowed)
                ++count;
        return count;
    };
    if (starting_at == intersection.size() || count_valid() == 0)
        return intersection;
    // handle single turn
    if (intersection.size() - starting_at == 1)
    {
        if (!intersection[starting_at].entry_allowed)
            return intersection;

        if (angularDeviation(intersection[starting_at].turn.angle, STRAIGHT_ANGLE) >
                NARROW_TURN_ANGLE &&
            angularDeviation(intersection[starting_at].turn.angle, 0) > NARROW_TURN_ANGLE)
        {
            // assign left turn
            intersection[starting_at].turn.instruction = {
                findBasicTurnType(via_edge, intersection[starting_at]), DirectionModifier::Left};
        }
        else if (angularDeviation(intersection[starting_at].turn.angle, STRAIGHT_ANGLE) <=
                 NARROW_TURN_ANGLE)
        {
            intersection[starting_at].turn.instruction = {
                findBasicTurnType(via_edge, intersection[starting_at]),
                DirectionModifier::SlightLeft};
        }
        else
        {
            intersection[starting_at].turn.instruction = {
                findBasicTurnType(via_edge, intersection[starting_at]),
                DirectionModifier::SharpLeft};
        }
    }
    // two turns on at the side
    else if (intersection.size() - starting_at == 2)
    {
        const auto first_direction = getTurnDirection(intersection[starting_at].turn.angle);
        const auto second_direction = getTurnDirection(intersection[starting_at + 1].turn.angle);
        if (first_direction == second_direction)
        {
            // conflict
            handleDistinctConflict(via_edge, intersection[starting_at + 1],
                                   intersection[starting_at]);
        }
        else
        {
            intersection[starting_at].turn.instruction = {
                findBasicTurnType(via_edge, intersection[starting_at]), first_direction};
            intersection[starting_at + 1].turn.instruction = {
                findBasicTurnType(via_edge, intersection[starting_at + 1]), second_direction};
        }
    }
    else if (intersection.size() - starting_at == 3)
    {
        const auto first_direction = getTurnDirection(intersection[starting_at].turn.angle);
        const auto second_direction = getTurnDirection(intersection[starting_at + 1].turn.angle);
        const auto third_direction = getTurnDirection(intersection[starting_at + 2].turn.angle);
        if (first_direction != second_direction && second_direction != third_direction)
        {
            // implies first != third, based on the angles and clockwise order
            if (intersection[starting_at].entry_allowed)
                intersection[starting_at].turn.instruction = {
                    findBasicTurnType(via_edge, intersection[starting_at]), first_direction};
            if (intersection[starting_at + 1].entry_allowed)
                intersection[starting_at + 1].turn.instruction = {
                    findBasicTurnType(via_edge, intersection[starting_at + 1]), second_direction};
            if (intersection[starting_at + 2].entry_allowed)
                intersection[starting_at + 2].turn.instruction = {
                    findBasicTurnType(via_edge, intersection[starting_at + 2]), second_direction};
        }
        else if (2 >= (intersection[starting_at].entry_allowed +
                       intersection[starting_at + 1].entry_allowed +
                       intersection[starting_at + 2].entry_allowed))
        {
            // at least one invalid turn
            if (!intersection[starting_at].entry_allowed)
            {
                handleDistinctConflict(via_edge, intersection[starting_at + 2],
                                       intersection[starting_at + 1]);
            }
            else if (!intersection[starting_at + 1].entry_allowed)
            {
                handleDistinctConflict(via_edge, intersection[starting_at + 2],
                                       intersection[starting_at]);
            }
            else
            {
                handleDistinctConflict(via_edge, intersection[starting_at + 1],
                                       intersection[starting_at]);
            }
        }
        else if (intersection[starting_at].entry_allowed &&
                 intersection[starting_at + 1].entry_allowed &&
                 intersection[starting_at + 2].entry_allowed &&
                 angularDeviation(intersection[starting_at].turn.angle,
                                  intersection[starting_at + 1].turn.angle) >= NARROW_TURN_ANGLE &&
                 angularDeviation(intersection[starting_at + 1].turn.angle,
                                  intersection[starting_at + 2].turn.angle) >= NARROW_TURN_ANGLE)
        {
            intersection[starting_at].turn.instruction = {
                findBasicTurnType(via_edge, intersection[starting_at]),
                DirectionModifier::SlightLeft};
            intersection[starting_at + 1].turn.instruction = {
                findBasicTurnType(via_edge, intersection[starting_at + 1]),
                DirectionModifier::Left};
            intersection[starting_at + 2].turn.instruction = {
                findBasicTurnType(via_edge, intersection[starting_at + 2]),
                DirectionModifier::SharpLeft};
        }
        else if (intersection[starting_at].entry_allowed &&
                 intersection[starting_at + 1].entry_allowed &&
                 intersection[starting_at + 2].entry_allowed &&
                 ((first_direction == second_direction && second_direction == third_direction) ||
                  (third_direction == second_direction &&
                   angularDeviation(intersection[starting_at].turn.angle,
                                    intersection[starting_at + 1].turn.angle) < GROUP_ANGLE) ||
                  (second_direction == first_direction &&
                   angularDeviation(intersection[starting_at + 1].turn.angle,
                                    intersection[starting_at + 2].turn.angle) < GROUP_ANGLE)))
        {
            intersection[starting_at].turn.instruction = {
                detail::isRampClass(intersection[starting_at].turn.eid, node_based_graph)
                    ? FirstRamp
                    : FirstTurn,
                second_direction};
            intersection[starting_at + 1].turn.instruction = {
                detail::isRampClass(intersection[starting_at + 1].turn.eid, node_based_graph)
                    ? SecondRamp
                    : SecondTurn,
                second_direction};
            intersection[starting_at + 2].turn.instruction = {
                detail::isRampClass(intersection[starting_at + 2].turn.eid, node_based_graph)
                    ? ThirdRamp
                    : ThirdTurn,
                second_direction};
        }
        else if (intersection[starting_at].entry_allowed &&
                 intersection[starting_at + 1].entry_allowed &&
                 intersection[starting_at + 2].entry_allowed &&
                 ((third_direction == second_direction &&
                   angularDeviation(intersection[starting_at].turn.angle,
                                    intersection[starting_at + 1].turn.angle) >= GROUP_ANGLE) ||
                  (second_direction == first_direction &&
                   angularDeviation(intersection[starting_at + 1].turn.angle,
                                    intersection[starting_at + 2].turn.angle) >= GROUP_ANGLE)))
        {
            // conflict one side with an additional very sharp turn
            if (angularDeviation(intersection[starting_at + 1].turn.angle,
                                 intersection[starting_at + 2].turn.angle) >= GROUP_ANGLE)
            {
                handleDistinctConflict(via_edge, intersection[starting_at + 1],
                                       intersection[starting_at]);
                intersection[starting_at + 2].turn.instruction = {
                    findBasicTurnType(via_edge, intersection[starting_at + 2]), third_direction};
            }
            else
            {
                intersection[starting_at].turn.instruction = {
                    findBasicTurnType(via_edge, intersection[starting_at]), first_direction};
                handleDistinctConflict(via_edge, intersection[starting_at + 2],
                                       intersection[starting_at + 1]);
            }
        }

        else if ((first_direction == second_direction &&
                  intersection[starting_at].entry_allowed !=
                      intersection[starting_at + 1].entry_allowed) ||
                 (second_direction == third_direction &&
                  intersection[starting_at + 1].entry_allowed !=
                      intersection[starting_at + 2].entry_allowed))
        {
            // no conflict, due to conflict being restricted to valid/invalid
            if (intersection[starting_at].entry_allowed)
                intersection[starting_at].turn.instruction = {
                    findBasicTurnType(via_edge, intersection[starting_at]), first_direction};
            if (intersection[starting_at + 1].entry_allowed)
                intersection[starting_at + 1].turn.instruction = {
                    findBasicTurnType(via_edge, intersection[starting_at + 1]), second_direction};
            if (intersection[starting_at + 2].entry_allowed)
                intersection[starting_at + 2].turn.instruction = {
                    findBasicTurnType(via_edge, intersection[starting_at + 2]), third_direction};
        }
        else
        {
            auto coord = localizer(node_based_graph.GetTarget(via_edge));
            util::SimpleLogger().Write(logWARNING)
                << "Reached fallback for left turns, size 3: " << std::setprecision(12)
                << toFloating(coord.lat) << " " << toFloating(coord.lon);
            for (const auto road : intersection)
            {
                const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);
                util::SimpleLogger().Write(logWARNING)
                    << "\troad: " << road.toString() << " Name: " << out_data.name_id
                    << " Road Class: " << (int)out_data.road_classification.road_class
                    << " At: " << localizer(node_based_graph.GetTarget(road.turn.eid));
            }

            for (std::size_t i = starting_at; i < intersection.size(); ++i)
                if (intersection[i].entry_allowed)
                    intersection[i].turn.instruction = {
                        findBasicTurnType(via_edge, intersection[i]),
                        getTurnDirection(intersection[i].turn.angle)};
        }
    }
    else if (intersection.size() - starting_at == 4)
    {
        if (intersection[starting_at].entry_allowed)
            intersection[starting_at].turn.instruction = {
                detail::isRampClass(intersection[starting_at].turn.eid, node_based_graph)
                    ? FirstRamp
                    : FirstTurn,
                DirectionModifier::Left};
        if (intersection[starting_at + 1].entry_allowed)
            intersection[starting_at + 1].turn.instruction = {
                detail::isRampClass(intersection[starting_at + 1].turn.eid, node_based_graph)
                    ? SecondRamp
                    : SecondTurn,
                DirectionModifier::Left};
        if (intersection[starting_at + 2].entry_allowed)
            intersection[starting_at + 2].turn.instruction = {
                detail::isRampClass(intersection[starting_at + 2].turn.eid, node_based_graph)
                    ? ThirdRamp
                    : ThirdTurn,
                DirectionModifier::Left};
        if (intersection[starting_at + 3].entry_allowed)
            intersection[starting_at + 3].turn.instruction = {
                detail::isRampClass(intersection[starting_at + 3].turn.eid, node_based_graph)
                    ? FourthRamp
                    : FourthTurn,
                DirectionModifier::Left};
    }
    else
    {
        for (auto &road : intersection)
        {
            if (!road.entry_allowed)
                continue;
            road.turn.instruction = {detail::isRampClass(road.turn.eid, node_based_graph) ? Ramp
                                                                                          : Turn,
                                     getTurnDirection(road.turn.angle)};
        }
        /*
        auto coord = localizer(node_based_graph.GetTarget(via_edge));
        util::SimpleLogger().Write(logWARNING)
            << "Reached fallback for left turns (" << starting_at << ") " << std::setprecision(12)
            << toFloating(coord.lat) << " " << toFloating(coord.lon);
        for (const auto road : intersection)
        {
            const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);
            util::SimpleLogger().Write(logWARNING)
                << "\troad: " << road.toString() << " Name: " << out_data.name_id
                << " Road Class: " << (int)out_data.road_classification.road_class
                << " At: " << localizer(node_based_graph.GetTarget(road.turn.eid));
        }
        */
    }
    return intersection;
}

// can only assign three turns
std::vector<ConnectedRoad> TurnAnalysis::assignRightTurns(const EdgeID via_edge,
                                                          std::vector<ConnectedRoad> intersection,
                                                          const std::size_t up_to) const
{
    BOOST_ASSERT(up_to <= intersection.size());
    const auto count_valid = [&intersection, up_to]()
    {
        std::size_t count = 0;
        for (std::size_t i = 1; i < up_to; ++i)
            if (intersection[i].entry_allowed)
                ++count;
        return count;
    };
    if (up_to <= 1 || count_valid() == 0)
        return intersection;
    // handle single turn
    if (up_to == 2)
    {
        if (angularDeviation(intersection[1].turn.angle, STRAIGHT_ANGLE) > NARROW_TURN_ANGLE &&
            angularDeviation(intersection[1].turn.angle, 0) > NARROW_TURN_ANGLE)
        {
            // assign left turn
            intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                DirectionModifier::Right};
        }
        else if (angularDeviation(intersection[1].turn.angle, STRAIGHT_ANGLE) <= NARROW_TURN_ANGLE)
        {
            intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                DirectionModifier::SlightRight};
        }
        else
        {
            intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                DirectionModifier::SharpRight};
        }
    }
    else if (up_to == 3)
    {
        const auto first_direction = getTurnDirection(intersection[1].turn.angle);
        const auto second_direction = getTurnDirection(intersection[2].turn.angle);
        if (first_direction == second_direction)
        {
            // conflict
            handleDistinctConflict(via_edge, intersection[2], intersection[1]);
        }
        else
        {
            intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                first_direction};
            intersection[2].turn.instruction = {findBasicTurnType(via_edge, intersection[2]),
                                                second_direction};
        }
    }
    else if (up_to == 4)
    {
        const auto first_direction = getTurnDirection(intersection[1].turn.angle);
        const auto second_direction = getTurnDirection(intersection[2].turn.angle);
        const auto third_direction = getTurnDirection(intersection[3].turn.angle);
        if (first_direction != second_direction && second_direction != third_direction)
        {
            if (intersection[1].entry_allowed)
                intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                    first_direction};
            if (intersection[2].entry_allowed)
                intersection[2].turn.instruction = {findBasicTurnType(via_edge, intersection[2]),
                                                    second_direction};
            if (intersection[3].entry_allowed)
                intersection[3].turn.instruction = {findBasicTurnType(via_edge, intersection[3]),
                                                    third_direction};
        }
        else if (2 >= (intersection[1].entry_allowed + intersection[2].entry_allowed +
                       intersection[3].entry_allowed))
        {
            // at least a single invalid
            if (!intersection[3].entry_allowed)
            {
                handleDistinctConflict(via_edge, intersection[2], intersection[1]);
            }
            else if (!intersection[1].entry_allowed)
            {
                handleDistinctConflict(via_edge, intersection[3], intersection[2]);
            }
            else // handles one-valid as well as two valid (1,3)
            {
                handleDistinctConflict(via_edge, intersection[3], intersection[1]);
            }
        }

        else if (intersection[1].entry_allowed && intersection[2].entry_allowed &&
                 intersection[3].entry_allowed &&
                 angularDeviation(intersection[1].turn.angle, intersection[2].turn.angle) >=
                     NARROW_TURN_ANGLE &&
                 angularDeviation(intersection[2].turn.angle, intersection[3].turn.angle) >=
                     NARROW_TURN_ANGLE)
        {
            intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                DirectionModifier::SharpRight};
            intersection[2].turn.instruction = {findBasicTurnType(via_edge, intersection[2]),
                                                DirectionModifier::Right};
            intersection[3].turn.instruction = {findBasicTurnType(via_edge, intersection[3]),
                                                DirectionModifier::SlightRight};
        }
        else if (intersection[1].entry_allowed && intersection[2].entry_allowed &&
                 intersection[3].entry_allowed &&
                 ((first_direction == second_direction && second_direction == third_direction) ||
                  (first_direction == second_direction &&
                   angularDeviation(intersection[2].turn.angle, intersection[3].turn.angle) <
                       GROUP_ANGLE) ||
                  (second_direction == third_direction &&
                   angularDeviation(intersection[1].turn.angle, intersection[2].turn.angle) <
                       GROUP_ANGLE)))
        {
            intersection[1].turn.instruction = {
                detail::isRampClass(intersection[1].turn.eid, node_based_graph) ? ThirdRamp
                                                                                : ThirdTurn,
                second_direction};
            intersection[2].turn.instruction = {
                detail::isRampClass(intersection[2].turn.eid, node_based_graph) ? SecondRamp
                                                                                : SecondTurn,
                second_direction};
            intersection[3].turn.instruction = {
                detail::isRampClass(intersection[3].turn.eid, node_based_graph) ? FirstRamp
                                                                                : FirstTurn,
                second_direction};
        }
        else if (intersection[1].entry_allowed && intersection[2].entry_allowed &&
                 intersection[3].entry_allowed &&
                 ((first_direction == second_direction &&
                   angularDeviation(intersection[2].turn.angle, intersection[3].turn.angle) >=
                       GROUP_ANGLE) ||
                  (second_direction == third_direction &&
                   angularDeviation(intersection[1].turn.angle, intersection[2].turn.angle) >=
                       GROUP_ANGLE)))
        {
            if (angularDeviation(intersection[2].turn.angle, intersection[3].turn.angle) >=
                GROUP_ANGLE)
            {
                handleDistinctConflict(via_edge, intersection[2], intersection[1]);
                intersection[3].turn.instruction = {findBasicTurnType(via_edge, intersection[3]),
                                                    third_direction};
            }
            else
            {
                intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                    first_direction};
                handleDistinctConflict(via_edge, intersection[3], intersection[2]);
            }
        }
        else if ((first_direction == second_direction &&
                  intersection[1].entry_allowed != intersection[2].entry_allowed) ||
                 (second_direction == third_direction &&
                  intersection[2].entry_allowed != intersection[3].entry_allowed))
        {
            if (intersection[1].entry_allowed)
                intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                    first_direction};
            if (intersection[2].entry_allowed)
                intersection[2].turn.instruction = {findBasicTurnType(via_edge, intersection[2]),
                                                    second_direction};
            if (intersection[3].entry_allowed)
                intersection[3].turn.instruction = {findBasicTurnType(via_edge, intersection[3]),
                                                    third_direction};
        }
        else
        {
            auto coord = localizer(node_based_graph.GetTarget(via_edge));
            util::SimpleLogger().Write(logWARNING)
                << "Reached fallback for right turns, size 3: " << std::setprecision(12)
                << toFloating(coord.lat) << " " << toFloating(coord.lon)
                << " Valids: " << (intersection[1].entry_allowed + intersection[2].entry_allowed +
                                   intersection[3].entry_allowed);
            for (const auto road : intersection)
            {
                const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);
                util::SimpleLogger().Write(logWARNING)
                    << "\troad: " << road.toString() << " Name: " << out_data.name_id
                    << " Road Class: " << (int)out_data.road_classification.road_class
                    << " At: " << localizer(node_based_graph.GetTarget(road.turn.eid));
            }

            for (std::size_t i = 1; i < up_to; ++i)
                if (intersection[i].entry_allowed)
                    intersection[i].turn.instruction = {
                        findBasicTurnType(via_edge, intersection[i]),
                        getTurnDirection(intersection[i].turn.angle)};
        }
    }
    else if (up_to == 5)
    {
        if (intersection[4].entry_allowed)
            intersection[4].turn.instruction = {
                detail::isRampClass(intersection[4].turn.eid, node_based_graph) ? FirstRamp
                                                                                : FirstTurn,
                DirectionModifier::Right};
        if (intersection[3].entry_allowed)
            intersection[3].turn.instruction = {
                detail::isRampClass(intersection[3].turn.eid, node_based_graph) ? SecondRamp
                                                                                : SecondTurn,
                DirectionModifier::Right};
        if (intersection[2].entry_allowed)
            intersection[2].turn.instruction = {
                detail::isRampClass(intersection[2].turn.eid, node_based_graph) ? ThirdRamp
                                                                                : ThirdTurn,
                DirectionModifier::Right};
        if (intersection[1].entry_allowed)
            intersection[1].turn.instruction = {
                detail::isRampClass(intersection[1].turn.eid, node_based_graph) ? FourthRamp
                                                                                : FourthTurn,
                DirectionModifier::Right};
    }
    else
    {
        for (std::size_t i = 1; i < up_to; ++i)
        {
            auto &road = intersection[i];
            if (!road.entry_allowed)
                continue;
            road.turn.instruction = {detail::isRampClass(road.turn.eid, node_based_graph) ? Ramp
                                                                                          : Turn,
                                     getTurnDirection(road.turn.angle)};
        }

        /*
        auto coord = localizer(node_based_graph.GetTarget(via_edge));
        util::SimpleLogger().Write(logWARNING)
            << "Reached fallback for right turns (" << up_to << ") " << std::setprecision(12)
            << toFloating(coord.lat) << " " << toFloating(coord.lon);
        for (const auto road : intersection)
        {
            const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);
            util::SimpleLogger().Write(logWARNING)
                << "\troad: " << road.toString() << " Name: " << out_data.name_id
                << " Road Class: " << (int)out_data.road_classification.road_class
                << " At: " << localizer(node_based_graph.GetTarget(road.turn.eid));
        }
        */
    }
    return intersection;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
