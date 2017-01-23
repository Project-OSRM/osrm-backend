#include "partition/recursive_bisection.hpp"
#include "partition/inertial_flow.hpp"

#include "partition/graph_view.hpp"
#include "partition/recursive_bisection_state.hpp"

namespace osrm
{
namespace partition
{

RecursiveBisection::RecursiveBisection(std::size_t maximum_cell_size,
                                       double balance,
                                       double boundary_factor,
                                       const BisectionGraph &bisection_graph_)
    : bisection_graph(bisection_graph_), internal_state(bisection_graph_)
{
    GraphView view(bisection_graph, internal_state, internal_state.Begin(), internal_state.End());
    InertialFlow flow(view);
    const auto partition = flow.ComputePartition(balance, boundary_factor);
    const auto center = internal_state.ApplyBisection(view.Begin(), view.End(), partition);
    {
        auto state = internal_state;
    }

    GraphView recursive_view_lhs(bisection_graph, internal_state, view.Begin(), center);
    InertialFlow flow_lhs(recursive_view_lhs);
    const auto partition_lhs = flow_lhs.ComputePartition(balance,boundary_factor);
    internal_state.ApplyBisection(recursive_view_lhs.Begin(),recursive_view_lhs.End(),partition_lhs);

    GraphView recursive_view_rhs(bisection_graph, internal_state, center, view.End());
    InertialFlow flow_rhs(recursive_view_rhs);
    const auto partition_rhs = flow_rhs.ComputePartition(balance,boundary_factor);
    internal_state.ApplyBisection(recursive_view_rhs.Begin(),recursive_view_rhs.End(),partition_rhs);
}

} // namespace partition
} // namespace osrm
