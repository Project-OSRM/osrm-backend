#ifndef OSRM_EXTRACTOR_TRAFFIC_SIGNALS_HPP
#define OSRM_EXTRACTOR_TRAFFIC_SIGNALS_HPP

#include "util/typedefs.hpp"

#include <boost/functional/hash.hpp>
#include <unordered_set>

namespace osrm
{
namespace extractor
{

// The direction annotation is extracted from node tags.
// The directions in which traffic flow object applies are relative to the way containing the node.
enum class TrafficFlowControlNodeDirection : std::uint8_t
{
    NONE = 0,
    ALL = 1,
    FORWARD = 2,
    REVERSE = 3
};

// represents traffic lights, stop signs, give way signs, etc.
struct TrafficFlowControlNodes
{
    std::unordered_set<NodeID> bidirectional_nodes;
    std::unordered_set<std::pair<NodeID, NodeID>, boost::hash<std::pair<NodeID, NodeID>>>
        unidirectional_segments;

    inline bool Has(NodeID from, NodeID to) const
    {
        return bidirectional_nodes.count(to) > 0 || unidirectional_segments.count({from, to}) > 0;
    }
};

} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_TRAFFIC_SIGNALS_HPP
