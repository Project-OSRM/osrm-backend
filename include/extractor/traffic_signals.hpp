#ifndef OSRM_EXTRACTOR_TRAFFIC_SIGNALS_HPP
#define OSRM_EXTRACTOR_TRAFFIC_SIGNALS_HPP

#include "util/std_hash.hpp"
#include "util/typedefs.hpp"

#include <unordered_set>
#include <utility>

namespace osrm::extractor
{

struct TrafficSignals
{
    std::unordered_set<NodeID> bidirectional_nodes;
    std::unordered_set<std::pair<NodeID, NodeID>> unidirectional_segments;

    inline bool HasSignal(NodeID from, NodeID to) const
    {
        return bidirectional_nodes.contains(to) || unidirectional_segments.contains({from, to});
    }

    void Compress(NodeID from, NodeID via, NodeID to)
    {
        bidirectional_nodes.erase(via);
        if (unidirectional_segments.contains({via, to}))
        {
            unidirectional_segments.erase({via, to});
            unidirectional_segments.insert({from, to});
        }
        if (unidirectional_segments.contains({via, from}))
        {
            unidirectional_segments.erase({via, from});
            unidirectional_segments.insert({to, from});
        }
    }
};
} // namespace osrm::extractor

#endif // OSRM_EXTRACTOR_TRAFFIC_SIGNALS_HPP
