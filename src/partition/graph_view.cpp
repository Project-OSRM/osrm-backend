#include "partition/graph_view.hpp"
#include <iterator>

namespace osrm
{
namespace partition
{

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
