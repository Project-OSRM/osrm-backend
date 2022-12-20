#ifndef OSRM_UPDATER_CSV_SOURCE_HPP
#define OSRM_UPDATER_CSV_SOURCE_HPP

#include "updater/source.hpp"

namespace osrm::updater::csv
{
SegmentLookupTable readSegmentValues(const std::vector<std::string> &paths);
TurnLookupTable readTurnValues(const std::vector<std::string> &paths);
} // namespace osrm::updater::csv

#endif
