#include "partition/graph_view.hpp"

#include <iostream>
#include <iterator>

namespace osrm
{
namespace partition
{

namespace
{
bool verifyView(const GraphView &view)
{
    for (std::size_t i = 0; i < view.NumberOfNodes(); ++i)
    {
        for (const auto &edge : view.Edges(i))
            if (edge.target > view.NumberOfNodes())
            {
                std::cout << "Out of range: " << edge.target << " not within " << view.NumberOfNodes() << std::endl;
                return false;
            }
    }
    return true;
}
} // namespace

GraphView::GraphView(const BisectionGraph &bisection_graph_)
    : bisection_graph(bisection_graph_),
      begin(bisection_graph.CBegin()), end(bisection_graph.CEnd())
{
    BOOST_ASSERT(verifyView(*this));
}

GraphView::GraphView(const BisectionGraph &bisection_graph_,
                     const BisectionGraph::ConstNodeIterator begin_,
                     const BisectionGraph::ConstNodeIterator end_)
    : bisection_graph(bisection_graph_), begin(begin_), end(end_)
{
    BOOST_ASSERT(verifyView(*this));
}

NodeID GraphView::GetID(const BisectionGraph::NodeT &node) const
{
    return static_cast<NodeID>(&node - &(*begin));
}

BisectionGraph::ConstNodeIterator GraphView::Begin() const { return begin; }

BisectionGraph::ConstNodeIterator GraphView::End() const { return end; }

std::size_t GraphView::NumberOfNodes() const { return std::distance(begin, end); }

const BisectionNode &GraphView::GetNode(const NodeID nid) const
{
    return bisection_graph.Node(nid);
}

const BisectionEdge &GraphView::GetEdge(const EdgeID eid) const
{
    return bisection_graph.Edge(eid);
}

} // namespace partition
} // namespace osrm
