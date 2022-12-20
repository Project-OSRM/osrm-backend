#ifndef OSRM_EXTRACTOR_TURN_PATH_FILTER_HPP_
#define OSRM_EXTRACTOR_TURN_PATH_FILTER_HPP_

#include "extractor/restriction.hpp"
#include "util/node_based_graph.hpp"

#include <vector>

namespace osrm::extractor
{

// To avoid handling invalid turn paths / creating unnecessary duplicate nodes for via-ways, we do
// a pre-flight check for paths and remove all invalid turn relations from the data. Use as
// `restrictions = removeInvalidRestrictions(std::move(restrictions))`
template <typename T>
std::vector<T> removeInvalidTurnPaths(std::vector<T>, const util::NodeBasedDynamicGraph &);
} // namespace osrm::extractor

#endif // OSRM_EXTRACTOR_TURN_PATH_FILTER_HPP_
