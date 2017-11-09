#ifndef OSRM_EXTRACTOR_EDGE_BASED_NODE_HPP_
#define OSRM_EXTRACTOR_EDGE_BASED_NODE_HPP_

#include "util/typedefs.hpp"

namespace osrm
{
namespace extractor
{

struct EdgeBasedNode
{
    GeometryID geometry_id;
    ComponentID component_id;
    AnnotationID annotation_id : 31;
    bool segregated : 1;
};

static_assert(sizeof(EdgeBasedNode) == 3 * 4, "Should be 3 * sizeof(uint32_t)");

} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_EDGE_BASED_NODE_HPP_
