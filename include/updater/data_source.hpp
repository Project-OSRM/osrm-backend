#ifndef OSRM_UPDATER_DATA_SOURCE_HPP
#define OSRM_UPDATER_DATA_SOURCE_HPP

#include "updater/source.hpp"
#include "updater/updater_config.hpp"

namespace osrm
{
namespace updater
{
namespace data
{
SegmentLookupTable readSegmentValues(const std::vector<std::string> &paths, SpeedAndTurnPenaltyFormat format);
TurnLookupTable readTurnValues(const std::vector<std::string> &paths, SpeedAndTurnPenaltyFormat format);
} // namespace data
} // namespace updater
} // namespace osrm

#endif
