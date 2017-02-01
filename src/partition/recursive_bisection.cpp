#include "partition/recursive_bisection.hpp"
#include "partition/inertial_flow.hpp"

#include "partition/graph_view.hpp"
#include "partition/recursive_bisection_state.hpp"

#include "util/timing_util.hpp"

#include "util/geojson_debug_logger.hpp"
#include "util/geojson_debug_policies.hpp"

#include "extractor/tarjan_scc.hpp"
#include "partition/tarjan_graph_wrapper.hpp"

#include <unordered_map>

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
    auto views = FakeFirstPartitionWithSCC(1000);

    std::cout << "Components: " << views.size() << std::endl;
    ;

    TIMER_START(bisection);
    GraphView view = views.front();
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
    GraphView recursive_view_lhs(bisection_graph, view.Begin(), center);
    InertialFlow flow_lhs(recursive_view_lhs);
    const auto partition_lhs = flow_lhs.ComputePartition(10, balance, boundary_factor);
    internal_state.ApplyBisection(
        recursive_view_lhs.Begin(), recursive_view_lhs.End(), 1, partition_lhs.flags);
    TIMER_STOP(bisection_2_1);
    std::cout << "Bisection(2) completed in " << TIMER_SEC(bisection_2_1)
              << " Cut Size: " << partition_lhs.num_edges
              << " Balance: " << partition_lhs.num_nodes_source << std::endl;

    TIMER_START(bisection_2_2);
    GraphView recursive_view_rhs(bisection_graph, center, view.End());
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

std::vector<GraphView>
RecursiveBisection::FakeFirstPartitionWithSCC(const std::size_t small_component_size)
{
    // since our graphs are unidirectional, we don't realy need the scc. But tarjan is so nice and
    // assigns IDs and counts sizes
    TarjanGraphWrapper wrapped_graph(bisection_graph);
    extractor::TarjanSCC<TarjanGraphWrapper> scc_algo(wrapped_graph);
    scc_algo.Run();

    // Map Edges to Sccs
    const auto in_small = [&scc_algo, small_component_size](const NodeID node_id) {
        return scc_algo.GetComponentSize(scc_algo.GetComponentID(node_id)) <= small_component_size;
    };

    const constexpr std::size_t small_component_id = -1;
    std::unordered_map<std::size_t, std::size_t> component_map;
    const auto transform_id = [&](const NodeID node_id) -> std::size_t {
        if (in_small(node_id))
            return small_component_id;
        else
            return scc_algo.GetComponentID(node_id);
    };

    std::vector<NodeID> mapping(bisection_graph.NumberOfNodes(), SPECIAL_NODEID);
    for (const auto &node : bisection_graph.Nodes())
        mapping[node.original_id] = component_map[transform_id(node.original_id)]++;

    // needs to remove edges, if we should ever switch to directed graphs here
    std::stable_sort(
        bisection_graph.Begin(), bisection_graph.End(), [&](const auto &lhs, const auto &rhs) {
            return transform_id(lhs.original_id) < transform_id(rhs.original_id);
        });

    // remap all remaining edges
    std::for_each(bisection_graph.Begin(), bisection_graph.End(), [&](const auto &node) {
        for (auto &edge : bisection_graph.Edges(node))
            edge.target = mapping[edge.target];
    });

    std::vector<GraphView> views;
    auto last = bisection_graph.CBegin();
    auto last_id = transform_id(bisection_graph.Begin()->original_id);

    for (auto itr = bisection_graph.CBegin(); itr != bisection_graph.CEnd(); ++itr)
    {
        auto itr_id = transform_id(itr->original_id);
        if (last_id != itr_id)
        {
            views.push_back(GraphView(bisection_graph, last, itr));
            last_id = itr_id;
            last = itr;
        }
    }
    views.push_back(GraphView(bisection_graph, last, bisection_graph.CEnd()));
    return views;
}

} // namespace partition
} // namespace osrm
