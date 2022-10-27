#ifndef OSRM_EXTRACTOR_TRAFFIC_SIGNALS_HPP
#define OSRM_EXTRACTOR_TRAFFIC_SIGNALS_HPP

#include "util/typedefs.hpp"
#include <unordered_set>

#include <boost/unordered_set.hpp>

namespace osrm
{
namespace extractor
{

// TODO: better naming
struct RoadObjects
{
    std::unordered_set<NodeID> bidirectional_nodes;
    std::unordered_set<std::pair<NodeID, NodeID>, boost::hash<std::pair<NodeID, NodeID>>>
        unidirectional_segments;

    inline bool HasSignal(NodeID from, NodeID to) const
    {
        return bidirectional_nodes.count(to) > 0 || unidirectional_segments.count({from, to}) > 0;
    }
};

struct TrafficSignals final : public RoadObjects {};

// TODO: better naming ?
struct StopSigns final : public RoadObjects {};
struct GiveWaySigns final : public RoadObjects {};


} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_TRAFFIC_SIGNALS_HPP
