#include "partitioner/bisection_graph_view.hpp"

#include <iostream>
#include <iterator>

#include <boost/assert.hpp>

namespace osrm
{
namespace partitioner
{

BisectionGraphView::BisectionGraphView(const BisectionGraph &bisection_graph_)
    : BisectionGraphView(bisection_graph_, bisection_graph_.CBegin(), bisection_graph_.CEnd())
{
}

BisectionGraphView::BisectionGraphView(const BisectionGraph &bisection_graph_,
                                       const BisectionGraph::ConstNodeIterator begin_,
                                       const BisectionGraph::ConstNodeIterator end_)
    : bisection_graph(bisection_graph_), begin(begin_), end(end_)
{
}

BisectionGraphView::BisectionGraphView(const BisectionGraphView &other_view,
                                       const BisectionGraph::ConstNodeIterator begin_,
                                       const BisectionGraph::ConstNodeIterator end_)
    : BisectionGraphView(other_view.bisection_graph, begin_, end_)
{
}

std::size_t BisectionGraphView::NumberOfNodes() const { return std::distance(begin, end); }

NodeID BisectionGraphView::GetID(const NodeT &node) const
{
    const auto node_id = static_cast<NodeID>(&node - &(*begin));
    BOOST_ASSERT(node_id < NumberOfNodes());
    return node_id;
}

BisectionGraph::ConstNodeIterator BisectionGraphView::Begin() const { return begin; }

BisectionGraph::ConstNodeIterator BisectionGraphView::End() const { return end; }

const BisectionGraphView::NodeT &BisectionGraphView::Node(const NodeID nid) const
{
    return *(begin + nid);
}

const BisectionGraphView::EdgeT &BisectionGraphView::Edge(const EdgeID eid) const
{
    return bisection_graph.Edge(eid);
}

} // namespace partitioner
} // namespace osrm
