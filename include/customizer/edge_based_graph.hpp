#ifndef OSRM_CUSTOMIZE_EDGE_BASED_GRAPH_HPP
#define OSRM_CUSTOMIZE_EDGE_BASED_GRAPH_HPP

#include "extractor/edge_based_edge.hpp"
#include "partition/edge_based_graph.hpp"
#include "util/static_graph.hpp"
#include "util/typedefs.hpp"

#include <boost/filesystem/path.hpp>

namespace osrm
{
namespace customizer
{

struct StaticEdgeBasedGraph;

namespace io
{
void read(const boost::filesystem::path &path, StaticEdgeBasedGraph &graph);
void write(const boost::filesystem::path &path, const StaticEdgeBasedGraph &graph);
}

using EdgeBasedGraphEdgeData = partition::EdgeBasedGraphEdgeData;

struct StaticEdgeBasedGraph : util::StaticGraph<EdgeBasedGraphEdgeData>
{
    using Base = util::StaticGraph<EdgeBasedGraphEdgeData>;
    using Base::Base;

    friend void io::read(const boost::filesystem::path &path, StaticEdgeBasedGraph &graph);
    friend void io::write(const boost::filesystem::path &path, const StaticEdgeBasedGraph &graph);
};

struct StaticEdgeBasedGraphView : util::StaticGraph<EdgeBasedGraphEdgeData, true>
{
    using Base = util::StaticGraph<EdgeBasedGraphEdgeData, true>;
    using Base::Base;
};

struct StaticEdgeBasedGraphEdge : StaticEdgeBasedGraph::InputEdge
{
    using Base = StaticEdgeBasedGraph::InputEdge;
    using Base::Base;
};
}
}

#endif
