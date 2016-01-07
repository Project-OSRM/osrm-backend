#ifndef EXTERNAL_MEMORY_NODE_HPP_
#define EXTERNAL_MEMORY_NODE_HPP_

#include "extractor/query_node.hpp"

#include "util/typedefs.hpp"

namespace osrm
{
namespace extractor
{

struct ExternalMemoryNode : QueryNode
{
    ExternalMemoryNode(int lat, int lon, OSMNodeID node_id, bool barrier, bool traffic_lights)
        : QueryNode(lat, lon, node_id), barrier(barrier), traffic_lights(traffic_lights)
    {
    }

    ExternalMemoryNode() : barrier(false), traffic_lights(false) {}

    static ExternalMemoryNode min_value()
    {
        return ExternalMemoryNode(0, 0, MIN_OSM_NODEID, false, false);
    }

    static ExternalMemoryNode max_value()
    {
        return ExternalMemoryNode(std::numeric_limits<int>::max(), std::numeric_limits<int>::max(),
                                  MAX_OSM_NODEID, false, false);
    }

    bool barrier;
    bool traffic_lights;
};

struct ExternalMemoryNodeSTXXLCompare
{
    using value_type = ExternalMemoryNode;
    value_type max_value() { return value_type::max_value(); }
    value_type min_value() { return value_type::min_value(); }
    bool operator()(const value_type &left, const value_type &right) const
    {
        return left.node_id < right.node_id;
    }
};
}
}

#endif /* EXTERNAL_MEMORY_NODE_HPP_ */
