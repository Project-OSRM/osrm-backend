#ifndef OSRM_EXTRACTOR_TRAFFIC_SIGNALS_HPP
#define OSRM_EXTRACTOR_TRAFFIC_SIGNALS_HPP

#include "util/typedefs.hpp"
#include <unordered_set>

#include <boost/unordered_set.hpp>

namespace osrm
{
namespace extractor
{

// Stop Signs tagged on nodes can be present or not. In addition Stop Signs have
// an optional way direction they apply to. If the direction is unknown from the
// data we have to compute by checking the distance to the next intersection.
//
// Impl. detail: namespace + enum instead of enum class to make Luabind happy


// The traffic light annotation is extracted from node tags.
// The directions in which the traffic light applies are relative to the way containing the node.
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
