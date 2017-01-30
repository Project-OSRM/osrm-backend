#include "partition/graph_view.hpp"

#include <iostream>
#include <iterator>

namespace osrm
{
namespace partition
{

HasSamePartitionID::HasSamePartitionID(const RecursiveBisectionState::BisectionID bisection_id_,
                                       const BisectionGraph &bisection_graph_,
                                       const RecursiveBisectionState &recursive_bisection_state_)
    : bisection_id(bisection_id_), bisection_graph(bisection_graph_),
      recursive_bisection_state(recursive_bisection_state_)
{
}

bool HasSamePartitionID::operator()(const EdgeID eid) const
{
    return recursive_bisection_state.GetBisectionID(bisection_graph.GetEdge(eid).target) ==
           bisection_id;
}

GraphView::GraphView(const BisectionGraph &bisection_graph_,
                     const RecursiveBisectionState &bisection_state_,
                     const RecursiveBisectionState::IDIterator begin_,
                     const RecursiveBisectionState::IDIterator end_)
    : bisection_graph(bisection_graph_), bisection_state(bisection_state_), begin(begin_), end(end_)
{
}

RecursiveBisectionState::IDIterator GraphView::Begin() const { return begin; }

RecursiveBisectionState::IDIterator GraphView::End() const { return end; }

std::size_t GraphView::NumberOfNodes() const { return std::distance(begin, end); }

GraphView::EdgeIterator GraphView::EdgeBegin(const NodeID nid) const
{
    HasSamePartitionID predicate{
        bisection_state.GetBisectionID(nid), bisection_graph, bisection_state};

    EdgeIDIterator first{bisection_graph.BeginEdges(nid)};
    EdgeIDIterator last{bisection_graph.EndEdges(nid)};

    return boost::make_filter_iterator(predicate, first, last);
}

GraphView::EdgeIterator GraphView::EdgeEnd(const NodeID nid) const
{
    HasSamePartitionID predicate{
        bisection_state.GetBisectionID(nid), bisection_graph, bisection_state};

    EdgeIDIterator last{bisection_graph.EndEdges(nid)};

    return boost::make_filter_iterator(predicate, last, last);
}

const BisectionNode &GraphView::GetNode(const NodeID nid) const
{
    return bisection_graph.GetNode(nid);
}

const BisectionEdge &GraphView::GetEdge(const EdgeID eid) const
{
    return bisection_graph.GetEdge(eid);
}

std::uint32_t GraphView::GetPosition(const NodeID nid) const
{
    BOOST_ASSERT(bisection_state.GetPosition(nid) >= bisection_state.GetPosition(*Begin()));
    return bisection_state.GetPosition(nid) - bisection_state.GetPosition(*Begin());
}

} // namespace partition
} // namespace osrm
