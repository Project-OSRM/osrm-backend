#ifndef OSRM_EXTRACTOR_TRAFFIC_SIGNALS_HPP
#define OSRM_EXTRACTOR_TRAFFIC_SIGNALS_HPP

#include "util/typedefs.hpp"

#include <boost/functional/hash.hpp>
#include <unordered_set>

namespace osrm::extractor
{

struct TrafficSignals
{
    std::unordered_set<NodeID> bidirectional_nodes;
    std::unordered_set<std::pair<NodeID, NodeID>, boost::hash<std::pair<NodeID, NodeID>>>
        unidirectional_segments;

    inline bool HasSignal(NodeID from, NodeID to) const
    {
        return bidirectional_nodes.count(to) > 0 || unidirectional_segments.count({from, to}) > 0;
    }

    void Compress(NodeID from, NodeID via, NodeID to)
    {
        bidirectional_nodes.erase(via);
        if (unidirectional_segments.count({via, to}))
        {
            unidirectional_segments.erase({via, to});
            unidirectional_segments.insert({from, to});
        }
        if (unidirectional_segments.count({via, from}))
        {
            unidirectional_segments.erase({via, from});
            unidirectional_segments.insert({to, from});
        }
    }
};
} // namespace osrm::extractor

#endif // OSRM_EXTRACTOR_TRAFFIC_SIGNALS_HPP
