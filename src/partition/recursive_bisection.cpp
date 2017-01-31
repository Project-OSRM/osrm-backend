#include "partition/recursive_bisection.hpp"
#include "partition/inertial_flow.hpp"

#include "partition/graph_view.hpp"
#include "partition/recursive_bisection_state.hpp"

#include "util/timing_util.hpp"

#include "util/geojson_debug_logger.hpp"
#include "util/geojson_debug_policies.hpp"

namespace osrm
{
namespace partition
{

RecursiveBisection::RecursiveBisection(std::size_t maximum_cell_size,
                                       double balance,
                                       double boundary_factor,
                                       BisectionGraph &bisection_graph_)
    : bisection_graph(bisection_graph_), internal_state(bisection_graph_)
{
    TIMER_START(bisection);
    GraphView view(bisection_graph, internal_state);
    InertialFlow flow(view);
    const auto partition = flow.ComputePartition(10, balance, boundary_factor);
    const auto center = internal_state.ApplyBisection(view.Begin(), view.End(), 0, partition.flags);
    {
        auto state = internal_state;
    }
    TIMER_STOP(bisection);
    std::cout << "Bisection completed in " << TIMER_SEC(bisection)
              << " Cut Size: " << partition.num_edges << " Balance: " << partition.num_nodes_source
              << std::endl;

    util::ScopedGeojsonLoggerGuard<util::CoordinateVectorToLineString, util::LoggingScenario(0)>
        logger_zero("level_0.geojson");
    for (NodeID nid = 0; nid < bisection_graph.NumberOfNodes(); ++nid)
    {
        for (const auto &edge : bisection_graph.Edges(nid))
        {
            const auto target = edge.target;
            if (internal_state.GetBisectionID(nid) != internal_state.GetBisectionID(target))
            {
                std::vector<util::Coordinate> coordinates;
                coordinates.push_back(bisection_graph.Node(nid).coordinate);
                coordinates.push_back(bisection_graph.Node(target).coordinate);
                logger_zero.Write(coordinates);
            }
        }
    }

    TIMER_START(bisection_2_1);
    GraphView recursive_view_lhs(bisection_graph, internal_state, view.Begin(), center);
    InertialFlow flow_lhs(recursive_view_lhs);
    const auto partition_lhs = flow_lhs.ComputePartition(10, balance, boundary_factor);
    internal_state.ApplyBisection(
        recursive_view_lhs.Begin(), recursive_view_lhs.End(), 1, partition_lhs.flags);
    TIMER_STOP(bisection_2_1);
    std::cout << "Bisection(2) completed in " << TIMER_SEC(bisection_2_1)
              << " Cut Size: " << partition_lhs.num_edges
              << " Balance: " << partition_lhs.num_nodes_source << std::endl;

    TIMER_START(bisection_2_2);
    GraphView recursive_view_rhs(bisection_graph, internal_state, center, view.End());
    InertialFlow flow_rhs(recursive_view_rhs);
    const auto partition_rhs = flow_rhs.ComputePartition(10, balance, boundary_factor);
    internal_state.ApplyBisection(
        recursive_view_rhs.Begin(), recursive_view_rhs.End(), 1, partition_rhs.flags);
    TIMER_STOP(bisection_2_2);
    std::cout << "Bisection(3) completed in " << TIMER_SEC(bisection_2_2)
              << " Cut Size: " << partition_rhs.num_edges
              << " Balance: " << partition_rhs.num_nodes_source << std::endl;

    util::ScopedGeojsonLoggerGuard<util::CoordinateVectorToLineString, util::LoggingScenario(1)>
        logger_one("level_1.geojson");

    for (NodeID nid = 0; nid < bisection_graph.NumberOfNodes(); ++nid)
    {
        for (const auto &edge : bisection_graph.Edges(nid))
        {
            const auto target = edge.target;
            if (internal_state.GetBisectionID(nid) != internal_state.GetBisectionID(target))
            {
                std::vector<util::Coordinate> coordinates;
                coordinates.push_back(bisection_graph.Node(nid).coordinate);
                coordinates.push_back(bisection_graph.Node(target).coordinate);
                logger_one.Write(coordinates);
            }
        }
    }
}

} // namespace partition
} // namespace osrm
