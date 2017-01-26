#ifndef OSRM_PARTITION_GRAPHVIEW_HPP_
#define OSRM_PARTITION_GRAPHVIEW_HPP_

#include "partition/bisection_graph.hpp"
#include "partition/recursive_bisection_state.hpp"

#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include <cstddef>
#include <cstdint>

namespace osrm
{
namespace partition
{

// Non-owning immutable sub-graph view into a base graph.
// The part of the graph to select is determined by the recursive bisection state.
class GraphView
{
  public:
    GraphView(const BisectionGraph &graph,
              const BisectionGraph::ConstNodeIterator begin,
              const BisectionGraph::ConstNodeIterator end);

    GraphView(const BisectionGraph &graph);

    // Number of nodes _in this sub-graph.
    std::size_t NumberOfNodes() const;

    BisectionGraph::ConstNodeIterator Begin() const;
    BisectionGraph::ConstNodeIterator End() const;

    const BisectionNode &GetNode(const NodeID nid) const;
    const BisectionEdge &GetEdge(const EdgeID eid) const;

    NodeID GetID(const BisectionGraph::NodeT &node) const;

    inline auto Edges(const NodeID nid) const { return bisection_graph.Edges(*(begin + nid)); }
    inline auto BeginEdges(const NodeID nid) const
    {
        return bisection_graph.BeginEdges(*(begin + nid));
    }
    inline auto EndEdges(const NodeID nid) const
    {
        return bisection_graph.EndEdges(*(begin + nid));
    }

  private:
    const BisectionGraph &bisection_graph;

    const BisectionGraph::ConstNodeIterator begin;
    const BisectionGraph::ConstNodeIterator end;
};

} // namespace partition
} // namespace osrm

#endif // OSRM_PARTITION_GRAPHVIEW_HPP_
