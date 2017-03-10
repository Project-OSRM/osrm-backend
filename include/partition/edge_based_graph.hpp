#ifndef OSRM_EDGE_BASED_GRAPH_HPP
#define OSRM_EDGE_BASED_GRAPH_HPP

#include "extractor/edge_based_edge.hpp"
#include "storage/io.hpp"
#include "util/coordinate.hpp"
#include "util/dynamic_graph.hpp"
#include "util/typedefs.hpp"

#include <cstdint>

#include <algorithm>
#include <iterator>
#include <memory>
#include <vector>

namespace osrm
{
namespace partition
{

struct EdgeBasedGraphEdgeData : extractor::EdgeBasedEdge::EdgeData
{
    // We need to write out the full edge based graph again.

    // TODO: in case we want to modify the graph we need to store a boundary_arc flag here
};

struct DynamicEdgeBasedGraph : util::DynamicGraph<EdgeBasedGraphEdgeData>
{
    using Base = util::DynamicGraph<EdgeBasedGraphEdgeData>;
    using Base::Base;
};

struct DynamicEdgeBasedGraphEdge : DynamicEdgeBasedGraph::InputEdge
{
    using Base = DynamicEdgeBasedGraph::InputEdge;
    using Base::Base;
};
}
}

#endif
