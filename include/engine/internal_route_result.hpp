#ifndef RAW_ROUTE_DATA_H
#define RAW_ROUTE_DATA_H

#include "engine/phantom_node.hpp"
#include "extractor/travel_mode.hpp"
#include "extractor/turn_instructions.hpp"
#include "util/typedefs.hpp"

#include "osrm/coordinate.hpp"

#include <vector>

struct PathData
{
    PathData()
        : node(SPECIAL_NODEID), name_id(INVALID_EDGE_WEIGHT), segment_duration(INVALID_EDGE_WEIGHT),
          turn_instruction(TurnInstruction::NoTurn), travel_mode(TRAVEL_MODE_INACCESSIBLE)
    {
    }

    PathData(NodeID node,
             unsigned name_id,
             TurnInstruction turn_instruction,
             EdgeWeight segment_duration,
             TravelMode travel_mode)
        : node(node), name_id(name_id), segment_duration(segment_duration),
          turn_instruction(turn_instruction), travel_mode(travel_mode)
    {
    }
    NodeID node;
    unsigned name_id;
    EdgeWeight segment_duration;
    TurnInstruction turn_instruction;
    TravelMode travel_mode : 4;
};

struct InternalRouteResult
{
    std::vector<std::vector<PathData>> unpacked_path_segments;
    std::vector<PathData> unpacked_alternative;
    std::vector<PhantomNodes> segment_end_coordinates;
    std::vector<bool> source_traversed_in_reverse;
    std::vector<bool> target_traversed_in_reverse;
    std::vector<bool> alt_source_traversed_in_reverse;
    std::vector<bool> alt_target_traversed_in_reverse;
    int shortest_path_length;
    int alternative_path_length;

    bool is_valid() const { return INVALID_EDGE_WEIGHT != shortest_path_length; }

    bool has_alternative() const { return INVALID_EDGE_WEIGHT != alternative_path_length; }

    bool is_via_leg(const std::size_t leg) const
    {
        return (leg != unpacked_path_segments.size() - 1);
    }

    InternalRouteResult()
        : shortest_path_length(INVALID_EDGE_WEIGHT), alternative_path_length(INVALID_EDGE_WEIGHT)
    {
    }
};

#endif // RAW_ROUTE_DATA_H
