#ifndef EXTERNAL_MEMORY_NODE_HPP_
#define EXTERNAL_MEMORY_NODE_HPP_

#include "extractor/query_node.hpp"

#include "util/typedefs.hpp"

struct ExternalMemoryNode : QueryNode
{
    ExternalMemoryNode(int lat, int lon, OSMNodeID id, bool barrier, bool traffic_light);

    ExternalMemoryNode();

    static ExternalMemoryNode min_value();

    static ExternalMemoryNode max_value();

    bool barrier;
    bool traffic_lights;
};

struct ExternalMemoryNodeSTXXLCompare
{
    using value_type = ExternalMemoryNode;
    bool operator()(const ExternalMemoryNode &left, const ExternalMemoryNode &right) const;
    value_type max_value();
    value_type min_value();
};

#endif /* EXTERNAL_MEMORY_NODE_HPP_ */
