#ifndef OSRM_UPDATER_CSV_SOURCE_HPP
#define OSRM_UPDATER_CSV_SOURCE_HPP

#include "updater/source.hpp"
#include "updater/updater_config.hpp"

namespace osrm
{
namespace updater
{
namespace csv
{
SegmentLookupTable readSegmentValues(const std::vector<std::string> &paths, UpdaterConfig::SpeedAndTurnPenaltyFormat format);
TurnLookupTable readTurnValues(const std::vector<std::string> &paths, UpdaterConfig::SpeedAndTurnPenaltyFormat format);
} // namespace csv
} // namespace updater
} // namespace osrm

#endif
