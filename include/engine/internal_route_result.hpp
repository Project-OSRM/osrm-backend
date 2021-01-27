#ifndef RAW_ROUTE_DATA_H
#define RAW_ROUTE_DATA_H

#include "extractor/class_data.hpp"
#include "extractor/travel_mode.hpp"

#include "guidance/turn_bearing.hpp"
#include "guidance/turn_instruction.hpp"

#include "engine/phantom_node.hpp"

#include "util/coordinate.hpp"
#include "util/guidance/entry_class.hpp"
#include "util/guidance/turn_lanes.hpp"
#include "util/integer_range.hpp"
#include "util/typedefs.hpp"

#include <vector>

namespace osrm
{
namespace engine
{

struct PathData
{
    // from edge-based-node id
    NodeID from_edge_based_node;
    // the internal OSRM id of the OSM node id that is the via node of the turn
    NodeID turn_via_node;
    // name of the street that leads to the turn
    unsigned name_id;
    // segregated edge-based node that leads to the turn
    bool is_segregated;
    // weight that is traveled on the segment until the turn is reached
    // including the turn weight, if one exists
    EdgeWeight weight_until_turn;
    // If this segment immediately preceeds a turn, then duration_of_turn
    // will contain the weight of the turn.  Otherwise it will be 0.
    EdgeWeight weight_of_turn;
    // duration that is traveled on the segment until the turn is reached,
    // including a turn if the segment preceeds one.
    EdgeWeight duration_until_turn;
    // If this segment immediately preceeds a turn, then duration_of_turn
    // will contain the duration of the turn.  Otherwise it will be 0.
    EdgeWeight duration_of_turn;
    // instruction to execute at the turn
    osrm::guidance::TurnInstruction turn_instruction;
    // turn lane data
    util::guidance::LaneTupleIdPair lane_data;
    // travel mode of the street that leads to the turn
    extractor::TravelMode travel_mode : 4;
    // user defined classed of the street that leads to the turn
    extractor::ClassData classes;
    // entry class of the turn, indicating possibility of turns
    util::guidance::EntryClass entry_class;

    // Source of the speed value on this road segment
    DatasourceID datasource_id;

    // bearing (as seen from the intersection) pre-turn
    osrm::guidance::TurnBearing pre_turn_bearing;
    // bearing (as seen from the intersection) post-turn
    osrm::guidance::TurnBearing post_turn_bearing;

    // Driving side of the turn
    bool is_left_hand_driving;
};

struct InternalRouteResult
{
    std::vector<std::vector<PathData>> unpacked_path_segments;
    std::vector<PhantomNodes> segment_end_coordinates;
    std::vector<bool> source_traversed_in_reverse;
    std::vector<bool> target_traversed_in_reverse;
    EdgeWeight shortest_path_weight = INVALID_EDGE_WEIGHT;

    bool is_valid() const { return INVALID_EDGE_WEIGHT != shortest_path_weight; }

    bool is_via_leg(const std::size_t leg) const
    {
        return (leg != unpacked_path_segments.size() - 1);
    }

    // Note: includes duration for turns, except for at start and end node.
    EdgeWeight duration() const
    {
        EdgeWeight ret{0};

        for (const auto &leg : unpacked_path_segments)
            for (const auto &segment : leg)
                ret += segment.duration_until_turn;

        return ret;
    }
};

struct InternalManyRoutesResult
{
    InternalManyRoutesResult() = default;
    InternalManyRoutesResult(InternalRouteResult route) : routes{std::move(route)} {}
    InternalManyRoutesResult(std::vector<InternalRouteResult> routes_) : routes{std::move(routes_)}
    {
    }

    std::vector<InternalRouteResult> routes;
};

inline InternalRouteResult CollapseInternalRouteResult(const InternalRouteResult &leggy_result,
                                                       const std::vector<bool> &is_waypoint)
{
    BOOST_ASSERT(leggy_result.is_valid());
    BOOST_ASSERT(is_waypoint[0]);     // first and last coords
    BOOST_ASSERT(is_waypoint.back()); // should always be waypoints
    // Nothing to collapse! return result as is
    if (leggy_result.unpacked_path_segments.size() == 1)
        return leggy_result;

    BOOST_ASSERT(leggy_result.segment_end_coordinates.size() > 1);

    InternalRouteResult collapsed;
    collapsed.shortest_path_weight = leggy_result.shortest_path_weight;
    for (auto i : util::irange<std::size_t>(0, leggy_result.unpacked_path_segments.size()))
    {
        if (is_waypoint[i])
        {
            // start another leg vector
            collapsed.unpacked_path_segments.push_back(leggy_result.unpacked_path_segments[i]);
            // save new phantom node pair
            collapsed.segment_end_coordinates.push_back(leggy_result.segment_end_coordinates[i]);
            // save data about phantom nodes
            collapsed.source_traversed_in_reverse.push_back(
                leggy_result.source_traversed_in_reverse[i]);
            collapsed.target_traversed_in_reverse.push_back(
                leggy_result.target_traversed_in_reverse[i]);
        }
        else
        // no new leg, collapse the next segment into the last leg
        {
            BOOST_ASSERT(!collapsed.unpacked_path_segments.empty());
            auto &last_segment = collapsed.unpacked_path_segments.back();
            BOOST_ASSERT(!collapsed.segment_end_coordinates.empty());
            collapsed.segment_end_coordinates.back().target_phantom =
                leggy_result.segment_end_coordinates[i].target_phantom;
            collapsed.target_traversed_in_reverse.back() =
                leggy_result.target_traversed_in_reverse[i];
            // copy path segments into current leg
            if (!leggy_result.unpacked_path_segments[i].empty())
            {
                auto old_size = last_segment.size();
                last_segment.insert(last_segment.end(),
                                    leggy_result.unpacked_path_segments[i].begin(),
                                    leggy_result.unpacked_path_segments[i].end());

                // The first segment of the unpacked path is missing the weight of the
                // source phantom.  We need to add those values back so that the total
                // edge weight is correct
                last_segment[old_size].weight_until_turn +=

                    leggy_result.source_traversed_in_reverse[i]
                        ? leggy_result.segment_end_coordinates[i].source_phantom.reverse_weight
                        : leggy_result.segment_end_coordinates[i].source_phantom.forward_weight;

                last_segment[old_size].duration_until_turn +=
                    leggy_result.source_traversed_in_reverse[i]
                        ? leggy_result.segment_end_coordinates[i].source_phantom.reverse_duration
                        : leggy_result.segment_end_coordinates[i].source_phantom.forward_duration;
            }
        }
    }

    BOOST_ASSERT(collapsed.segment_end_coordinates.size() ==
                 collapsed.unpacked_path_segments.size());
    return collapsed;
}
} // namespace engine
} // namespace osrm

#endif // RAW_ROUTE_DATA_H
