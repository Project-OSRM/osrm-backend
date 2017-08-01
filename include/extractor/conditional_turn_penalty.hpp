#ifndef OSRM_EXTRACTOR_CONDITIONAL_TURN_PENALTY_HPP_
#define OSRM_EXTRACTOR_CONDITIONAL_TURN_PENALTY_HPP_

#include "util/coordinate.hpp"
#include "util/opening_hours.hpp"
#include <cstdint>
#include <vector>

#include <type_traits>

namespace osrm

{
namespace extractor
{

struct ConditionalTurnPenalty
{
    // offset into the sequential list of turn penalties (see TurnIndexBlock for reference);
    std::uint64_t turn_offset;
    util::Coordinate location;
    std::vector<util::OpeningHours> conditions;
};

} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_CONDITIONAL_TURN_PENALTY_HPP_
