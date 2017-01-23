#include "partition/graph_view.hpp"
#include <iterator>

namespace osrm
{
namespace partition
{

HasSamePartitionID::HasSamePartitionID(const RecursiveBisectionState::BisectionID bisection_id_,
                                       const BisectionGraph &bisection_graph_,
                                       const RecursiveBisectionState &recursive_bisection_state_)
    : bisection_id(bisection_id), bisection_graph(bisection_graph_),
      recursive_bisection_state(recursive_bisection_state_)
{
}

bool HasSamePartitionID::operator()(const EdgeID eid) const
{
    return recursive_bisection_state.GetBisectionID(bisection_graph.GetTarget(eid)) == bisection_id;
}

GraphView::GraphView(const BisectionGraph &graph_,
                     const RecursiveBisectionState &bisection_state_,
                     const RecursiveBisectionState::IDIterator begin_,
                     const RecursiveBisectionState::IDIterator end_)
    : graph(graph_), bisection_state(bisection_state_), begin(begin_), end(end_)
{
}

RecursiveBisectionState::IDIterator GraphView::Begin() const { return begin; }

RecursiveBisectionState::IDIterator GraphView::End() const { return end; }

std::size_t GraphView::NumberOfNodes() const { return std::distance(begin, end); }

} // namespace partition
} // namespace osrm
