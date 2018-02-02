#include "partitioner/dinic_max_flow.hpp"
#include "util/integer_range.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <queue>
#include <set>
#include <stack>

namespace osrm
{
namespace partitioner
{

namespace
{

const auto constexpr INVALID_LEVEL = std::numeric_limits<DinicMaxFlow::Level>::max();

auto makeHasNeighborNotInCheck(const DinicMaxFlow::SourceSinkNodes &set,
                               const BisectionGraphView &view)
{
    return [&](const NodeID nid) {
        const auto is_not_contained = [&set](const BisectionEdge &edge) {
            return set.count(edge.target) == 0;
        };
        return view.EndEdges(nid) !=
               std::find_if(view.BeginEdges(nid), view.EndEdges(nid), is_not_contained);
    };
}

} // end namespace

DinicMaxFlow::MinCut DinicMaxFlow::operator()(const BisectionGraphView &view,
                                              const SourceSinkNodes &source_nodes,
                                              const SourceSinkNodes &sink_nodes) const
{
    BOOST_ASSERT(Validate(view, source_nodes, sink_nodes));
    // for the inertial flow algorithm, we use quite a large set of nodes as source/sink nodes. Only
    // a few of them can be part of the process, since they are grouped together. A standard
    // parameterisation would be 25% sink/source nodes. This already includes 50% of the graph. By
    // only focussing on a small set on the outside of the source/sink blob, we can save quite some
    // overhead in initialisation/search cost.

    std::vector<NodeID> border_source_nodes;
    border_source_nodes.reserve(0.01 * source_nodes.size());

    std::copy_if(source_nodes.begin(),
                 source_nodes.end(),
                 std::back_inserter(border_source_nodes),
                 makeHasNeighborNotInCheck(source_nodes, view));

    std::vector<NodeID> border_sink_nodes;
    border_sink_nodes.reserve(0.01 * sink_nodes.size());
    std::copy_if(sink_nodes.begin(),
                 sink_nodes.end(),
                 std::back_inserter(border_sink_nodes),
                 makeHasNeighborNotInCheck(sink_nodes, view));

    // edges in current flow that have capacity
    // The graph (V,E) contains undirected edges for all (u,v) \in V x V. We describe the flow as a
    // set of vertices (s,t) with flow set to `true`. Since flow can be either from `s` to `t` or
    // from `t` to `s`, we can remove `(s,t)` from the flow, if we send flow back the first time,
    // and insert `(t,s)` only if we send flow again.

    // allocate storage for the flow
    FlowEdges flow(view.NumberOfNodes());
    std::size_t flow_value = 0;
    do
    {
        auto levels = ComputeLevelGraph(view, border_source_nodes, source_nodes, sink_nodes, flow);

        // check if the sink can be reached from the source, it's enough to check the border
        const auto separated = std::find_if(border_sink_nodes.begin(),
                                            border_sink_nodes.end(),
                                            [&levels](const auto node) {
                                                return levels[node] != INVALID_LEVEL;
                                            }) == border_sink_nodes.end();

        if (!separated)
        {
            flow_value += BlockingFlow(flow, levels, view, source_nodes, border_sink_nodes);
        }
        else
        {
            // mark levels for all sources to not confuse make-cut (due to the border nodes
            // heuristic)
            for (auto s : source_nodes)
                levels[s] = 0;
            const auto cut = MakeCut(view, levels, flow_value);
            return cut;
        }
    } while (true);
}

DinicMaxFlow::MinCut DinicMaxFlow::MakeCut(const BisectionGraphView &view,
                                           const LevelGraph &levels,
                                           const std::size_t flow_value) const
{
    const auto is_valid_level = [](const Level level) { return level != INVALID_LEVEL; };

    // all elements within `levels` are on the source side
    // This part should opt to find the most balanced cut, which is not necessarily the case right
    // now. There is potential for optimisation here.
    std::vector<bool> result(view.NumberOfNodes());

    BOOST_ASSERT(view.NumberOfNodes() == levels.size());
    std::size_t source_side_count = std::count_if(levels.begin(), levels.end(), is_valid_level);
    std::transform(levels.begin(), levels.end(), result.begin(), is_valid_level);

    return {source_side_count, flow_value, std::move(result)};
}

DinicMaxFlow::LevelGraph
DinicMaxFlow::ComputeLevelGraph(const BisectionGraphView &view,
                                const std::vector<NodeID> &border_source_nodes,
                                const SourceSinkNodes &source_nodes,
                                const SourceSinkNodes &sink_nodes,
                                const FlowEdges &flow) const
{
    LevelGraph levels(view.NumberOfNodes(), INVALID_LEVEL);
    std::queue<NodeID> level_queue;

    // set the front of the source nodes to zero and add them to the BFS queue. In addition, set all
    // neighbors to zero as well (which allows direct usage of the levels to see what we visited,
    // and still don't go back into the hughe set of sources)
    for (const auto node_id : border_source_nodes)
    {
        levels[node_id] = 0;
        level_queue.push(node_id);
        for (const auto &edge : view.Edges(node_id))
            if (source_nodes.count(edge.target))
                levels[edge.target] = 0;
    }
    // check if there is flow present on an edge
    const auto has_flow = [&](const NodeID from, const NodeID to) {
        return flow[from].find(to) != flow[from].end();
    };

    // perform a relaxation step in the BFS algorithm
    const auto relax_node = [&](const NodeID node_id) {
        // don't relax sink nodes
        if (sink_nodes.count(node_id))
            return;

        const auto level = levels[node_id] + 1;
        for (const auto &edge : view.Edges(node_id))
        {
            const auto target = edge.target;
            // don't relax edges with flow on them
            if (has_flow(node_id, target))
                continue;

            // don't go back, only follow edges to new nodes
            if (levels[target] > level)
            {
                level_queue.push(target);
                levels[target] = level;
            }
        }
    };

    // compute the levels of level graph using BFS
    while (!level_queue.empty())
    {
        relax_node(level_queue.front());
        level_queue.pop();
    }

    return levels;
}

std::size_t DinicMaxFlow::BlockingFlow(FlowEdges &flow,
                                       LevelGraph &levels,
                                       const BisectionGraphView &view,
                                       const SourceSinkNodes &source_nodes,
                                       const std::vector<NodeID> &border_sink_nodes) const
{
    // track the number of augmenting paths (which in sum will equal the number of unique border
    // edges) (since our graph is undirected)
    std::size_t flow_increase = 0;

    // augment the flow along a path in the level graph
    const auto augment_flow = [&flow](const std::vector<NodeID> &path) {

        // add/remove flow edges from the current residual graph
        const auto augment_one = [&flow](const NodeID from, const NodeID to) {
            // check if there is flow in the opposite direction
            auto existing_edge = flow[to].find(from);
            if (existing_edge != flow[to].end())
                flow[to].erase(existing_edge); // remove flow from reverse edges first
            else
                flow[from].insert(to); // only add flow if no opposite flow exists

            // do augmentation on all pairs, never stop early:
            return false;
        };

        // augment all adjacent edges
        std::adjacent_find(path.begin(), path.end(), augment_one);
    };

    const auto augment_all_paths = [&](const NodeID sink_node_id) {
        // only augment sinks
        if (levels[sink_node_id] == INVALID_LEVEL)
            return;

        while (true)
        {
            // as long as there are augmenting paths from the sink, add them
            const auto path = GetAugmentingPath(levels, sink_node_id, view, flow, source_nodes);
            if (path.empty())
                break;
            else
            {
                augment_flow(path);
                ++flow_increase;
            }
        }
    };

    std::for_each(border_sink_nodes.begin(), border_sink_nodes.end(), augment_all_paths);
    BOOST_ASSERT(flow_increase > 0);
    return flow_increase;
}

// performs a dfs in the level graph, by adjusting levels that don't offer any further paths to
// INVALID_LEVEL and by following the level graph, this looks at every edge at most `c` times (O(E))
std::vector<NodeID> DinicMaxFlow::GetAugmentingPath(LevelGraph &levels,
                                                    const NodeID node_id,
                                                    const BisectionGraphView &view,
                                                    const FlowEdges &flow,
                                                    const SourceSinkNodes &source_nodes) const
{
    std::vector<NodeID> path;
    BOOST_ASSERT(source_nodes.find(node_id) == source_nodes.end());

    // Keeps the local state of the DFS in forms of the iterators
    struct DFSState
    {
        BisectionGraph::ConstEdgeIterator edge_iterator;
        const BisectionGraph::ConstEdgeIterator end_iterator;
    };

    std::stack<DFSState> dfs_stack;
    DFSState initial_state = {view.BeginEdges(node_id), view.EndEdges(node_id)};
    dfs_stack.push(std::move(initial_state));
    path.push_back(node_id);

    while (!dfs_stack.empty())
    {
        // the dfs_stack and the path have to be kept in sync
        BOOST_ASSERT(dfs_stack.size() == path.size());

        while (dfs_stack.top().edge_iterator != dfs_stack.top().end_iterator)
        {
            const auto target = dfs_stack.top().edge_iterator->target;

            // look at every edge only once, so advance the state of the current node (last in
            // path)
            dfs_stack.top().edge_iterator++;

            // check if the edge is valid
            const auto has_capacity = flow[target].count(path.back()) == 0;
            const auto descends_level_graph = levels[target] + 1 == levels[path.back()];

            if (has_capacity && descends_level_graph)
            {
                // recurse
                path.push_back(target);

                // termination
                if (source_nodes.find(target) != source_nodes.end())
                {
                    std::reverse(path.begin(), path.end());
                    return path;
                }

                // start next iteration
                dfs_stack.push({view.BeginEdges(target), view.EndEdges(target)});
            }
        }

        // backtrack - mark that there is no way to the target
        levels[path.back()] = -1;
        path.pop_back();
        dfs_stack.pop();
    }
    BOOST_ASSERT(path.empty());
    return path;
}

bool DinicMaxFlow::Validate(const BisectionGraphView &view,
                            const SourceSinkNodes &source_nodes,
                            const SourceSinkNodes &sink_nodes) const
{
    // sink and source cannot share a common node
    const auto separated =
        std::find_if(source_nodes.begin(), source_nodes.end(), [&sink_nodes](const auto node) {
            return sink_nodes.count(node);
        }) == source_nodes.end();

    const auto invalid_id = [&view](const NodeID nid) { return nid >= view.NumberOfNodes(); };
    const auto in_range_source =
        std::find_if(source_nodes.begin(), source_nodes.end(), invalid_id) == source_nodes.end();
    const auto in_range_sink =
        std::find_if(sink_nodes.begin(), sink_nodes.end(), invalid_id) == sink_nodes.end();

    return separated && in_range_source && in_range_sink;
}

} // namespace partitioner
} // namespace osrm
