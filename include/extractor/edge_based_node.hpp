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
    AnnotationID annotation_id;
};

} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_EDGE_BASED_NODE_HPP_
