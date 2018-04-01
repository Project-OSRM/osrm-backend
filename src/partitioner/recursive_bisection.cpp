#include "partitioner/recursive_bisection.hpp"
#include "partitioner/inertial_flow.hpp"

#include "partitioner/bisection_graph_view.hpp"
#include "partitioner/recursive_bisection_state.hpp"

#include "util/log.hpp"
#include "util/timing_util.hpp"

#include <tbb/parallel_do.h>

#include <algorithm>
#include <climits> // for CHAR_BIT
#include <cstddef>
#include <iterator>
#include <unordered_map>
#include <utility>
#include <vector>

namespace osrm
{
namespace partitioner
{

RecursiveBisection::RecursiveBisection(BisectionGraph &bisection_graph_,
                                       const std::size_t maximum_cell_size,
                                       const double balance,
                                       const double boundary_factor,
                                       const std::size_t num_optimizing_cuts,
                                       const std::size_t small_component_size)
    : bisection_graph(bisection_graph_), internal_state(bisection_graph_)
{
    auto components = internal_state.PrePartitionWithSCC(small_component_size);
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
        BisectionGraphView graph;
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

    std::transform(first, last, std::back_inserter(forest), [this](auto graph) {
        return TreeNode{std::move(graph), internal_state.SCCDepth()};
    });

    using Feeder = tbb::parallel_do_feeder<TreeNode>;

    TIMER_START(bisection);

    // Bisect graph into two parts. Get partition point and recurse left and right in parallel.
    tbb::parallel_do(begin(forest), end(forest), [&](const TreeNode &node, Feeder &feeder) {
        const auto cut =
            computeInertialFlowCut(node.graph, num_optimizing_cuts, balance, boundary_factor);
        const auto center = internal_state.ApplyBisection(
            node.graph.Begin(), node.graph.End(), node.depth, cut.flags);

        const auto terminal = [&](const auto &node) {
            const auto maximum_depth = sizeof(BisectionID) * CHAR_BIT;
            const auto too_small = node.graph.NumberOfNodes() < maximum_cell_size;
            const auto too_deep = node.depth >= maximum_depth;
            return too_small || too_deep;
        };

        BisectionGraphView left_graph{bisection_graph, node.graph.Begin(), center};
        TreeNode left_node{std::move(left_graph), node.depth + 1};

        if (!terminal(left_node))
            feeder.add(std::move(left_node));

        BisectionGraphView right_graph{bisection_graph, center, node.graph.End()};
        TreeNode right_node{std::move(right_graph), node.depth + 1};

        if (!terminal(right_node))
            feeder.add(std::move(right_node));
    });

    TIMER_STOP(bisection);

    util::Log() << "Full bisection done in " << TIMER_SEC(bisection) << "s";
}

const std::vector<BisectionID> &RecursiveBisection::BisectionIDs() const
{
    return internal_state.BisectionIDs();
}

std::uint32_t RecursiveBisection::SCCDepth() const { return internal_state.SCCDepth(); }

} // namespace partitioner
} // namespace osrm
