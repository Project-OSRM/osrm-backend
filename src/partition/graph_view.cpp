#include "partition/graph_view.hpp"

#include <iostream>
#include <iterator>

#include <boost/assert.hpp>

namespace osrm
{
namespace partition
{

GraphView::GraphView(const BisectionGraph &bisection_graph_)
    : GraphView(bisection_graph_, bisection_graph_.CBegin(), bisection_graph_.CEnd())
{
}

GraphView::GraphView(const BisectionGraph &bisection_graph_,
                     const BisectionGraph::ConstNodeIterator begin_,
                     const BisectionGraph::ConstNodeIterator end_)
    : bisection_graph(bisection_graph_), begin(begin_), end(end_)
{
}

GraphView::GraphView(const GraphView &other_view,
                     const BisectionGraph::ConstNodeIterator begin_,
                     const BisectionGraph::ConstNodeIterator end_)
    : GraphView(other_view.bisection_graph, begin_, end_)
{
}

std::size_t GraphView::NumberOfNodes() const { return std::distance(begin, end); }

NodeID GraphView::GetID(const NodeT &node) const
{
    const auto node_id = static_cast<NodeID>(&node - &(*begin));
    BOOST_ASSERT(node_id < NumberOfNodes());
    return node_id;
}

BisectionGraph::ConstNodeIterator GraphView::Begin() const { return begin; }

BisectionGraph::ConstNodeIterator GraphView::End() const { return end; }

const GraphView::NodeT &GraphView::Node(const NodeID nid) const { return *(begin + nid); }

const GraphView::EdgeT &GraphView::Edge(const EdgeID eid) const
{
    return bisection_graph.Edge(eid);
}

} // namespace partition
} // namespace osrm
