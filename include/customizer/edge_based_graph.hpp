#ifndef OSRM_CUSTOMIZE_EDGE_BASED_GRAPH_HPP
#define OSRM_CUSTOMIZE_EDGE_BASED_GRAPH_HPP

#include "extractor/edge_based_edge.hpp"
#include "partition/edge_based_graph.hpp"
#include "partition/multi_level_graph.hpp"
#include "util/static_graph.hpp"
#include "util/typedefs.hpp"

#include "storage/shared_memory_ownership.hpp"

#include <boost/filesystem/path.hpp>

namespace osrm
{
namespace customizer
{

using EdgeBasedGraphEdgeData = partition::EdgeBasedGraphEdgeData;

struct MultiLevelEdgeBasedGraph
    : public partition::MultiLevelGraph<EdgeBasedGraphEdgeData, storage::Ownership::Container>
{
    using Base = partition::MultiLevelGraph<EdgeBasedGraphEdgeData, storage::Ownership::Container>;
    using Base::Base;
};

struct MultiLevelEdgeBasedGraphView
    : public partition::MultiLevelGraph<EdgeBasedGraphEdgeData, storage::Ownership::View>
{
    using Base = partition::MultiLevelGraph<EdgeBasedGraphEdgeData, storage::Ownership::View>;
    using Base::Base;
};

struct StaticEdgeBasedGraphEdge : MultiLevelEdgeBasedGraph::InputEdge
{
    using Base = MultiLevelEdgeBasedGraph::InputEdge;
    using Base::Base;
};
}
}

#endif
