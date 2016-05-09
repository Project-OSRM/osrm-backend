#include "extractor/guidance/turn_classification.hpp"

#include "util/simple_logger.hpp"

#include <cstddef>
#include <cstdint>
#include <iomanip>

namespace osrm
{
namespace extractor
{
namespace guidance
{

struct TurnPossibility
{
    TurnPossibility(bool entry_allowed, double bearing, std::uint8_t discrete_id)
        : entry_allowed(entry_allowed), bearing(std::move(bearing)),
          discrete_id(std::move(discrete_id))
    {
    }

    TurnPossibility() : entry_allowed(false), bearing(0), discrete_id(0) {}

    bool entry_allowed;
    double bearing;
    std::uint8_t discrete_id;
};

namespace
{

bool hasConflicts(const std::vector<TurnPossibility> &turns)
{
    if (turns.size() <= 1)
        return false;
    for (std::size_t pos = 0; pos < turns.size(); ++pos)
    {
        if (turns[pos].discrete_id == turns[(pos + 1) % turns.size()].discrete_id)
            return true;
    }
    return false;
}

// Fix cases with nearly identical turns. If the difference between turns is just to small to be
// visible, we will not report the angle twice
std::vector<TurnPossibility> fixIdenticalTurns(std::vector<TurnPossibility> intersection)
{
    BOOST_ASSERT(intersection.size() > 1);
    for (auto itr = intersection.begin(); itr != intersection.end(); ++itr)
    {
        const auto next = [&]() {
            auto next_itr = std::next(itr);
            if (next_itr != intersection.end())
                return next_itr;
            return intersection.begin();
        }();

        // conflict here?
        if (itr->discrete_id == next->discrete_id)
        {
            if (angularDeviation(itr->bearing, next->bearing) < 0.5) // very small angular difference
            {
                if (!itr->entry_allowed || next->entry_allowed)
                {
                    itr = intersection.erase(itr);
                }
                else
                {
                    intersection.erase(next);
                }
                if (itr == intersection.end())
                    break;
            }
        }
    }
    return intersection;
}

std::vector<TurnPossibility> fixAroundBorder(std::vector<TurnPossibility> intersection)
{
    BOOST_ASSERT(intersection.size() > 1);
    // We can solve a conflict by reporting different bearing availabilities, as long as
    // both conflicting turns are on different sides of the bearing separator.
    //
    // Consider this example:
    //      ID(0)
    //         . b
    // a ....   (ID 1)
    //         . c
    //      ID(2)
    //
    // Both b and c map to the ID 1. Due to the split, we can set ID 0 and 2. In
    // deducing the best available bearing for a turn, we can now find 0 to be closest
    // to b and 2 to be closest to c. This only works when there are no other bearings
    // close to the conflicting assignment, though.
    for (std::size_t current_index = 0; current_index < intersection.size(); ++current_index)
    {
        const auto next_index = (current_index + 1) % intersection.size();
        if (intersection[current_index].discrete_id == intersection[next_index].discrete_id)
        {
            const double border = util::guidance::BearingClass::discreteIDToAngle(
                util::guidance::BearingClass::angleToDiscreteID(
                    intersection[current_index].bearing));

            // if both are on different sides of the separation, we can check for possible
            // resolution
            if (intersection[current_index].bearing < border &&
                intersection[next_index].bearing > border)
            {
                const auto shift_angle = [](const double bearing, const double delta) {
                    auto shifted_angle = bearing + delta;
                    if (shifted_angle < 0)
                        return shifted_angle + 360.;
                    if (shifted_angle > 360)
                        return shifted_angle - 360.;
                    return shifted_angle;
                };

                // conflict resolution is possible, if both bearings are available
                const auto left_id = util::guidance::BearingClass::angleToDiscreteID(
                    shift_angle(intersection[current_index].bearing,
                                -util::guidance::BearingClass::discrete_angle_step_size));
                const auto right_id = util::guidance::BearingClass::angleToDiscreteID(
                    shift_angle(intersection[next_index].bearing,
                                util::guidance::BearingClass::discrete_angle_step_size));

                const bool resolvable = [&]() {
                    if (intersection.size() == 2)
                        return true;

                    // cannot shift to the left without generating another conflict
                    if (intersection[current_index + intersection.size() - 1].discrete_id ==
                        left_id)
                        return false;

                    // cannot shift to the right without generating another conflict
                    if (intersection[(next_index + 1) % intersection.size()].discrete_id ==
                        right_id)
                        return false;

                    return true;
                }();

                if (resolvable)
                {
                    intersection[current_index].discrete_id = left_id;
                    intersection[next_index].discrete_id = right_id;
                }
            }
        }
    }
    return intersection;
}

// return an empty set of turns, if the conflict is not possible to be handled
std::vector<TurnPossibility> handleConflicts(std::vector<TurnPossibility> intersection)
{
    intersection = fixIdenticalTurns(std::move(intersection));

    if (!hasConflicts(intersection))
        return intersection;

    intersection = fixAroundBorder(intersection);

    // if the intersection still has conflicts, we cannot handle it correctly
    if (hasConflicts(intersection))
        intersection.clear();

    return intersection;
#if 0
    const auto border = util::guidance::BearingClass::discreteIDToAngle(
        util::guidance::BearingClass::angleToDiscreteID(intersection[0].bearing));

        // at least two turns
        auto previous_id = util::guidance::BearingClass::angleToDiscreteID(turns.back().bearing);
        for (std::size_t i = 0; i < turns.size(); ++i)
        {
            if (turns[i].entry_allowed)
                entry_class.activate(turn_index);
            const auto discrete_id =
                util::guidance::BearingClass::angleToDiscreteID(turns[i].bearing);

            const auto prev_index = (i + turns.size() - 1) % turns.size();
            if (discrete_id != previous_id)
            {
                // if we go back in IDs, conflict resolution has to deal with multiple conflicts
                if (previous_id > discrete_id &&
                    previous_id != (util::guidance::BearingClass::angleToDiscreteID(
                                       turns[prev_index].bearing)))
                {
                    std::cout << "Previous ID conflict " << (int)previous_id << " "
                              << (int)discrete_id << std::endl;
                    break;
                }
                ++turn_index;
                bearing_class.addDiscreteID(discrete_id);
                previous_id = discrete_id;
            }
            else
            {
                // the previous turn was handled into a conflict. Such a conflict cannot be
                // correctly expressed.
                // We have to report a unclassified setting.
                if (util::guidance::BearingClass::angleToDiscreteID(turns[prev_index].bearing) !=
                    previous_id)
                    break;

                if (turns[i].bearing >= border && turns[prev_index].bearing < border)
                {
                    const auto shift_angle = [](const double bearing, const double delta) {
                        auto shifted_angle = bearing + delta;
                        if (shifted_angle < 0)
                            return shifted_angle + 360.;
                        if (shifted_angle > 360)
                            return shifted_angle - 360.;
                        return shifted_angle;
                    };

                    // conflict resolution is possible, if both bearings are available
                    const auto left_id = util::guidance::BearingClass::angleToDiscreteID(
                        shift_angle(turns[prev_index].bearing,
                                    -util::guidance::BearingClass::discrete_angle_step_size));
                    const auto right_id = util::guidance::BearingClass::angleToDiscreteID(
                        shift_angle(turns[i].bearing,
                                    util::guidance::BearingClass::discrete_angle_step_size));
                    if (!bearing_class.hasDiscrete(left_id) && !bearing_class.hasDiscrete(right_id))
                    {
                        bearing_class.resetDiscreteID(discrete_id);
                        bearing_class.addDiscreteID(left_id);
                        bearing_class.addDiscreteID(right_id);
                        ++turn_index;
                        previous_id = right_id;
                    }
                }
            }
        }
#endif

    return intersection;
};

} // namespace

std::pair<util::guidance::EntryClass, util::guidance::BearingClass>
classifyIntersection(NodeID nid,
                     const Intersection &intersection,
                     const util::NodeBasedDynamicGraph &node_based_graph,
                     const extractor::CompressedEdgeContainer &compressed_geometries,
                     const std::vector<extractor::QueryNode> &query_nodes)
{
    if (intersection.empty())
        return {};

    std::vector<TurnPossibility> turns;

    const auto node_coordinate = util::Coordinate(query_nodes[nid].lon, query_nodes[nid].lat);

    // generate a list of all turn angles between a base edge, the node and a current edge
    for (const auto &road : intersection)
    {
        const auto eid = road.turn.eid;
        const auto edge_coordinate = getRepresentativeCoordinate(
            nid, node_based_graph.GetTarget(eid), eid, false, compressed_geometries, query_nodes);

        const double bearing =
            util::coordinate_calculation::bearing(node_coordinate, edge_coordinate);
        turns.push_back({road.entry_allowed, bearing,
                         util::guidance::BearingClass::angleToDiscreteID(bearing)});
    }

    std::sort(turns.begin(), turns.end(),
              [](const TurnPossibility left, const TurnPossibility right) {
                  return left.bearing < right.bearing;
              });

    // check for conflicts
    const bool has_conflicts = hasConflicts(turns);
    if (has_conflicts)
    { // try to handle conflicts, if possible
        turns = handleConflicts(std::move(turns));
    }

    util::guidance::EntryClass entry_class;
    util::guidance::BearingClass bearing_class;

    // finally transfer data to the entry/bearing classes
    std::size_t number = 0;
    for (const auto turn : turns)
    {
        if (turn.entry_allowed)
            entry_class.activate(number);
        bearing_class.addDiscreteID(turn.discrete_id);
        ++number;
    }

    static std::size_t mapping_failure_count = 0;
    if (turns.empty())
    {
        ++mapping_failure_count;
        util::SimpleLogger().Write(logDEBUG)
            << "Failed to provide full turn list for intersection ( " << mapping_failure_count
            << " ) for " << intersection.size() << " roads";

        std::cout << std::endl;
        for (const auto &road : intersection)
        {
            const auto eid = road.turn.eid;
            const auto edge_coordinate =
                getRepresentativeCoordinate(nid, node_based_graph.GetTarget(eid), eid, false,
                                            compressed_geometries, query_nodes);

            const double bearing =
                util::coordinate_calculation::bearing(node_coordinate, edge_coordinate);
            std::cout << " " << bearing << "("
                      << (int)util::guidance::BearingClass::angleToDiscreteID(bearing) << ")";
        }
        std::cout << std::endl;
        std::cout << "Location of intersection: " << std::setprecision(12) << " "
                  << util::toFloating(query_nodes[nid].lat) << " "
                  << util::toFloating(query_nodes[nid].lon) << std::endl;
        return {};
    }

    return std::make_pair(entry_class, bearing_class);
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
