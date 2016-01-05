#include "extractor/external_memory_node.hpp"
#include "extractor/query_node.hpp"

#include <limits>

namespace osrm
{
namespace extractor
{

ExternalMemoryNode::ExternalMemoryNode(
    int lat, int lon, OSMNodeID node_id, bool barrier, bool traffic_lights)
    : QueryNode(lat, lon, node_id), barrier(barrier), traffic_lights(traffic_lights)
{
}

ExternalMemoryNode::ExternalMemoryNode() : barrier(false), traffic_lights(false) {}

ExternalMemoryNode ExternalMemoryNode::min_value()
{
    return ExternalMemoryNode(0, 0, MIN_OSM_NODEID, false, false);
}

ExternalMemoryNode ExternalMemoryNode::max_value()
{
    return ExternalMemoryNode(std::numeric_limits<int>::max(), std::numeric_limits<int>::max(),
                              MAX_OSM_NODEID, false, false);
}

bool ExternalMemoryNodeSTXXLCompare::operator()(const ExternalMemoryNode &left,
                                                const ExternalMemoryNode &right) const
{
    return left.node_id < right.node_id;
}

ExternalMemoryNodeSTXXLCompare::value_type ExternalMemoryNodeSTXXLCompare::max_value()
{
    return ExternalMemoryNode::max_value();
}

ExternalMemoryNodeSTXXLCompare::value_type ExternalMemoryNodeSTXXLCompare::min_value()
{
    return ExternalMemoryNode::min_value();
}
}
}
