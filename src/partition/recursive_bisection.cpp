#include "partition/recursive_bisection.hpp"
#include "partition/inertial_flow.hpp"

#include "partition/graph_view.hpp"
#include "partition/recursive_bisection_state.hpp"

#include "util/log.hpp"
#include "util/timing_util.hpp"

#include <tbb/parallel_do.h>

#include <climits> // for CHAR_BIT
#include <cstddef>

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

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
                                       std::size_t num_optimizing_cuts,
                                       BisectionGraph &bisection_graph_)
    : bisection_graph(bisection_graph_), internal_state(bisection_graph_)
{
    auto components = FakeFirstPartitionWithSCC(1000 /*limit for small*/); // TODO
    BOOST_ASSERT(!components.empty());

    // Parallelize recursive bisection trees. Root cut happens serially (well, this is a lie:
    // since we handle big components in parallel, too. But we don't know this and
    // don't have to. TBB's scheduler handles nested parallelism just fine).
    //
    //     [   |   ]
    //      /     \         root cut
    //  [ | ]     [ | ]
    //  /   \     /   \     descend, do cuts in parallel
    //
    // https://www.threadingbuildingblocks.org/docs/help/index.htm#reference/algorithms/parallel_do_func.html

    struct TreeNode
    {
        GraphView graph;
        std::uint64_t depth;
    };

    // Build a recursive bisection tree for all big components independently in parallel.
    // Last GraphView is all small components: skip for bisection.
    auto first = begin(components);
    auto last = end(components) - 1;

    // We construct the trees on the fly: the root node is the entry point.
    // All tree branches depend on the actual cut and will be generated while descending.
    std::vector<TreeNode> forest;
    forest.reserve(last - first);

    std::transform(first, last, std::back_inserter(forest), [](auto graph) {
        return TreeNode{std::move(graph), 0};
    });

    using Feeder = tbb::parallel_do_feeder<TreeNode>;

    TIMER_START(bisection);

    // Bisect graph into two parts. Get partition point and recurse left and right in parallel.
    tbb::parallel_do(begin(forest), end(forest), [&](const TreeNode &node, Feeder &feeder) {
        InertialFlow flow{node.graph};
        const auto partition = flow.ComputePartition(num_optimizing_cuts, balance, boundary_factor);
        const auto center = internal_state.ApplyBisection(
            node.graph.Begin(), node.graph.End(), node.depth, partition.flags);

        const auto terminal = [&](const auto &node) {
            const auto maximum_depth = sizeof(RecursiveBisectionState::BisectionID) * CHAR_BIT;
            const auto too_small = node.graph.NumberOfNodes() < maximum_cell_size;
            const auto too_deep = node.depth >= maximum_depth;
            return too_small || too_deep;
        };

        GraphView left_graph{bisection_graph, node.graph.Begin(), center};
        TreeNode left_node{std::move(left_graph), node.depth + 1};

        if (!terminal(left_node))
            feeder.add(std::move(left_node));

        GraphView right_graph{bisection_graph, center, node.graph.End()};
        TreeNode right_node{std::move(right_graph), node.depth + 1};

        if (!terminal(right_node))
            feeder.add(std::move(right_node));
    });

    TIMER_STOP(bisection);

    util::Log() << "Full bisection done in " << TIMER_SEC(bisection) << "s";
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
