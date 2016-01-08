#ifndef SEGMENT_INFORMATION_HPP
#define SEGMENT_INFORMATION_HPP

#include "extractor/turn_instructions.hpp"
#include "extractor/travel_mode.hpp"
#include "util/typedefs.hpp"

#include "osrm/coordinate.hpp"
#include <utility>

namespace osrm
{
namespace engine
{

// Struct fits everything in one cache line
struct SegmentInformation
{
    util::FixedPointCoordinate location;
    NodeID name_id;
    EdgeWeight duration;
    float length;
    short pre_turn_bearing; // more than enough [0..3600] fits into 12 bits
    short post_turn_bearing;
    extractor::TurnInstruction turn_instruction;
    extractor::TravelMode travel_mode;
    bool necessary;
    bool is_via_location;

    explicit SegmentInformation(util::FixedPointCoordinate location,
                                const NodeID name_id,
                                const EdgeWeight duration,
                                const float length,
                                const extractor::TurnInstruction turn_instruction,
                                const bool necessary,
                                const bool is_via_location,
                                const extractor::TravelMode travel_mode)
        : location(std::move(location)), name_id(name_id), duration(duration), length(length),
          pre_turn_bearing(0), post_turn_bearing(0), turn_instruction(turn_instruction),
          travel_mode(travel_mode), necessary(necessary), is_via_location(is_via_location)
    {
    }

    explicit SegmentInformation(util::FixedPointCoordinate location,
                                const NodeID name_id,
                                const EdgeWeight duration,
                                const float length,
                                const extractor::TurnInstruction turn_instruction,
                                const extractor::TravelMode travel_mode)
        : location(std::move(location)), name_id(name_id), duration(duration), length(length),
          pre_turn_bearing(0), post_turn_bearing(0), turn_instruction(turn_instruction),
          travel_mode(travel_mode),
          necessary(turn_instruction != extractor::TurnInstruction::NoTurn), is_via_location(false)
    {
    }
};
}
}

#endif /* SEGMENT_INFORMATION_HPP */
