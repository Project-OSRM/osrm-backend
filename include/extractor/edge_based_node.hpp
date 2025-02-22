#ifndef OSRM_EXTRACTOR_EDGE_BASED_NODE_HPP_
#define OSRM_EXTRACTOR_EDGE_BASED_NODE_HPP_

#include "util/typedefs.hpp"

namespace osrm::extractor
{

struct EdgeBasedNode
{
    GeometryID geometry_id;
    ComponentID component_id;
    std::uint32_t annotation_id : 31;
    std::uint32_t segregated : 1;
};

} // namespace osrm::extractor

#endif // OSRM_EXTRACTOR_EDGE_BASED_NODE_HPP_
