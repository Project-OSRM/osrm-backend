#ifndef ORIGINAL_EDGE_DATA_HPP
#define ORIGINAL_EDGE_DATA_HPP

#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/travel_mode.hpp"
#include "util/guidance/turn_bearing.hpp"
#include "util/typedefs.hpp"

#include <cstddef>
#include <limits>

namespace osrm
{
namespace extractor
{

struct OriginalEdgeData
{
    explicit OriginalEdgeData(GeometryID via_geometry,
                              unsigned name_id,
                              LaneDataID lane_data_id,
                              guidance::TurnInstruction turn_instruction,
                              EntryClassID entry_classid,
                              TravelMode travel_mode,
                              util::guidance::TurnBearing pre_turn_bearing,
                              util::guidance::TurnBearing post_turn_bearing)
        : via_geometry(via_geometry), name_id(name_id), entry_classid(entry_classid),
          lane_data_id(lane_data_id), turn_instruction(turn_instruction), travel_mode(travel_mode),
          pre_turn_bearing(pre_turn_bearing), post_turn_bearing(post_turn_bearing)
    {
    }

    OriginalEdgeData()
        : via_geometry{std::numeric_limits<unsigned>::max() >> 1, false},
          name_id(std::numeric_limits<unsigned>::max()), entry_classid(INVALID_ENTRY_CLASSID),
          lane_data_id(INVALID_LANE_DATAID), turn_instruction(guidance::TurnInstruction::INVALID()),
          travel_mode(TRAVEL_MODE_INACCESSIBLE), pre_turn_bearing(0.0), post_turn_bearing(0.0)
    {
    }

    GeometryID via_geometry;
    NameID name_id;
    EntryClassID entry_classid;
    LaneDataID lane_data_id;
    guidance::TurnInstruction turn_instruction;
    TravelMode travel_mode;
    util::guidance::TurnBearing pre_turn_bearing;
    util::guidance::TurnBearing post_turn_bearing;
};

static_assert(sizeof(OriginalEdgeData) == 16,
              "Increasing the size of OriginalEdgeData increases memory consumption");
}
}

#endif // ORIGINAL_EDGE_DATA_HPP
