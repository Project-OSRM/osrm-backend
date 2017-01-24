#ifndef EXTERNAL_MEMORY_NODE_HPP_
#define EXTERNAL_MEMORY_NODE_HPP_

#include "extractor/query_node.hpp"

#include "util/typedefs.hpp"

#include <cstdint>

namespace osrm
{
namespace extractor
{

struct ExternalMemoryNode : QueryNode
{
    ExternalMemoryNode(const util::FixedLongitude lon_,
                       const util::FixedLatitude lat_,
                       OSMNodeID node_id_,
                       bool barrier_,
                       bool traffic_lights_)
        : QueryNode(lon_, lat_, node_id_), barrier(barrier_), traffic_lights(traffic_lights_)
    {
    }

    ExternalMemoryNode() : barrier(false), traffic_lights(false) {}

    static ExternalMemoryNode min_value()
    {
        return ExternalMemoryNode(
            util::FixedLongitude{0}, util::FixedLatitude{0}, MIN_OSM_NODEID, false, false);
    }

    static ExternalMemoryNode max_value()
    {
        return ExternalMemoryNode(util::FixedLongitude{std::numeric_limits<std::int32_t>::max()},
                                  util::FixedLatitude{std::numeric_limits<std::int32_t>::max()},
                                  MAX_OSM_NODEID,
                                  false,
                                  false);
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
