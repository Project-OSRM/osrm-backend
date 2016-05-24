#ifndef ORIGINAL_EDGE_DATA_HPP
#define ORIGINAL_EDGE_DATA_HPP

#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/travel_mode.hpp"
#include "util/typedefs.hpp"

#include <cstddef>
#include <limits>

namespace osrm
{
namespace extractor
{

struct OriginalEdgeData
{
    explicit OriginalEdgeData(NodeID via_node,
                              unsigned name_id,
                              unsigned destination_id,
                              guidance::TurnInstruction turn_instruction,
                              EntryClassID entry_classid,
                              TravelMode travel_mode)
        : via_node(via_node), name_id(name_id), destination_id(destination_id), entry_classid(entry_classid),
          turn_instruction(turn_instruction), travel_mode(travel_mode)
    {
    }

    OriginalEdgeData()
        : via_node(std::numeric_limits<unsigned>::max()),
          name_id(std::numeric_limits<unsigned>::max()),
          destination_id(std::numeric_limits<unsigned>::max()),
          entry_classid(INVALID_ENTRY_CLASSID),
          turn_instruction(guidance::TurnInstruction::INVALID()),
          travel_mode(TRAVEL_MODE_INACCESSIBLE)
    {
    }

    NodeID via_node;
    unsigned name_id;
    unsigned destination_id;
    EntryClassID entry_classid;
    guidance::TurnInstruction turn_instruction;
    TravelMode travel_mode;
};
}
}

#endif // ORIGINAL_EDGE_DATA_HPP
