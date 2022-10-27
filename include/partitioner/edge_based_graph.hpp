#ifndef OSRM_PARTITIONER_EDGE_BASED_GRAPH_HPP
#define OSRM_PARTITIONER_EDGE_BASED_GRAPH_HPP

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
namespace partitioner
{

struct EdgeBasedGraphEdgeData : extractor::EdgeBasedEdge::EdgeData
{
    using Base = extractor::EdgeBasedEdge::EdgeData;
    using Base::Base;

    EdgeBasedGraphEdgeData(const EdgeBasedGraphEdgeData &) = default;
    EdgeBasedGraphEdgeData(EdgeBasedGraphEdgeData &&) = default;
    EdgeBasedGraphEdgeData &operator=(const EdgeBasedGraphEdgeData &) = default;
    EdgeBasedGraphEdgeData &operator=(EdgeBasedGraphEdgeData &&) = default;
    EdgeBasedGraphEdgeData(const Base &base) : Base(base) {}
    EdgeBasedGraphEdgeData() : Base() {}
};

struct DynamicEdgeBasedGraph : util::DynamicGraph<EdgeBasedGraphEdgeData>
{
    using Base = util::DynamicGraph<EdgeBasedGraphEdgeData>;
    using Base::Base;

    template <class ContainerT>
    DynamicEdgeBasedGraph(const NodeIterator nodes,
                          const ContainerT &graph,
                          std::uint32_t connectivity_checksum)
        : Base(nodes, graph), connectivity_checksum(connectivity_checksum)
    {
    }

    std::uint32_t connectivity_checksum;
};

struct DynamicEdgeBasedGraphEdge : DynamicEdgeBasedGraph::InputEdge
{
    using Base = DynamicEdgeBasedGraph::InputEdge;
    using Base::Base;
};
} // namespace partitioner
} // namespace osrm

#endif
