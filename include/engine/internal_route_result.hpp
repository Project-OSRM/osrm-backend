#ifndef RAW_ROUTE_DATA_H
#define RAW_ROUTE_DATA_H

#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/travel_mode.hpp"
#include "engine/phantom_node.hpp"
#include "osrm/coordinate.hpp"
#include "util/guidance/turn_lanes.hpp"
#include "util/typedefs.hpp"

#include <vector>

namespace osrm
{
namespace engine
{

const constexpr unsigned INVALID_EXIT_NR = 0;

struct PathData
{
    // id of via node of the turn
    NodeID turn_via_node;
    // name of the street that leads to the turn
    unsigned name_id;
    // duration that is traveled on the segment until the turn is reached
    EdgeWeight duration_until_turn;
    // instruction to execute at the turn
    extractor::guidance::TurnInstruction turn_instruction;
    // turn lane data
    util::guidance::LaneTupelIdPair lane_data;
    // travel mode of the street that leads to the turn
    extractor::TravelMode travel_mode : 4;
    // entry class of the turn, indicating possibility of turns
    EntryClassID entry_classid;

    // Source of the speed value on this road segment
    DatasourceID datasource_id;
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
}
}

#endif // RAW_ROUTE_DATA_H
