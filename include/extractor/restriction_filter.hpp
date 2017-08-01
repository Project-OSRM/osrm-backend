#ifndef OSRM_EXTRACTOR_RESTRICTION_FILTER_HPP_
#define OSRM_EXTRACTOR_RESTRICTION_FILTER_HPP_

#include "extractor/restriction.hpp"
#include "util/node_based_graph.hpp"

#include <vector>

namespace osrm
{
namespace extractor
{

// To avoid handling invalid restrictions / creating unnecessary duplicate nodes for via-ways, we do
// a pre-flight check for restrictions and remove all invalid restrictions from the data. Use as
// `restrictions = removeInvalidRestrictions(std::move(restrictions))`
std::vector<ConditionalTurnRestriction>
removeInvalidRestrictions(std::vector<ConditionalTurnRestriction>,
                          const util::NodeBasedDynamicGraph &);

} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_RESTRICTION_FILTER_HPP_
