#ifndef OSRM_UTIL_EXCLUDE_FLAG_HPP
#define OSRM_UTIL_EXCLUDE_FLAG_HPP

#include "extractor/node_data_container.hpp"
#include "extractor/profile_properties.hpp"

namespace osrm::util
{

inline std::vector<std::vector<bool>>
excludeFlagsToNodeFilter(const NodeID number_of_nodes,
                         const extractor::EdgeBasedNodeDataContainer &node_data,
                         const extractor::ProfileProperties &properties)
{
    std::vector<std::vector<bool>> filters;
    for (auto mask : properties.excludable_classes)
    {
        if (mask != extractor::INAVLID_CLASS_DATA)
        {
            std::vector<bool> allowed_nodes(number_of_nodes);
            for (const auto node : util::irange<NodeID>(0, number_of_nodes))
            {
                allowed_nodes[node] = (node_data.GetClassData(node) & mask) == 0;
            }
            filters.push_back(std::move(allowed_nodes));
        }
    }
    return filters;
}
} // namespace osrm::util

#endif
