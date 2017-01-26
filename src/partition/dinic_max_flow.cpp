#include "partition/dinic_max_flow.hpp"

#include <algorithm>
#include <limits>
#include <queue>
#include <set>
#include <stack>

namespace osrm
{
namespace partition
{

namespace
{
const auto constexpr INVALID_LEVEL = std::numeric_limits<DinicMaxFlow::Level>::max();
} // end namespace

DinicMaxFlow::MinCut DinicMaxFlow::operator()(const GraphView &view,
                                              const SourceSinkNodes &source_nodes,
                                              const SourceSinkNodes &sink_nodes) const
{
    std::vector<NodeID> border_source_nodes;
    border_source_nodes.reserve(0.01 * source_nodes.size());

    for (auto node : source_nodes)
    {
        for (const auto &edge : view.Edges(node))
        {
            const auto target = edge.target;
            if (0 == source_nodes.count(target))
            {
                border_source_nodes.push_back(node);
                break;
            }
        }
    }

    std::vector<NodeID> border_sink_nodes;
    border_sink_nodes.reserve(0.01 * sink_nodes.size());
    for (auto node : sink_nodes)
    {
        for (const auto &edge : view.Edges(node))
        {
            const auto target = edge.target;
            if (0 == sink_nodes.count(target))
            {
                border_sink_nodes.push_back(node);
                break;
            }
        }
    }

    // edges in current flow that have capacity
    // The graph (V,E) contains undirected edges for all (u,v) \in V x V. We describe the flow as a
    // set of vertices (s,t) with flow set to `true`. Since flow can be either from `s` to `t` or
    // from `t` to `s`, we can remove `(s,t)` from the flow, if we send flow back the first time,
    // and insert `(t,s)` only if we send flow again.

    FlowEdges flow(view.NumberOfNodes());
    do
    {
        std::cout << "." << std::flush;
        auto levels = ComputeLevelGraph(view, border_source_nodes, source_nodes, sink_nodes, flow);

        // check if the sink can be reached from the source
        const auto separated = std::find_if(border_sink_nodes.begin(),
                                            border_sink_nodes.end(),
                                            [&levels, &view](const auto node) {
                                                return levels[node] != INVALID_LEVEL;
                                            }) == border_sink_nodes.end();

        if (!separated)
        {
            BlockingFlow(flow, levels, view, source_nodes, border_sink_nodes);
        }
        else
        {
            // mark levels for all sources to not confuse make-cut
            for (auto s : source_nodes)
                levels[s] = 0;
            return MakeCut(view, levels);
        }
    } while (true);
}

DinicMaxFlow::MinCut DinicMaxFlow::MakeCut(const GraphView &view, const LevelGraph &levels) const
{
    const auto is_sink_side = [&view, &levels](const NodeID nid) {
        return levels[nid] == INVALID_LEVEL;
    };

    // all elements within `levels` are on the source side
    // This part should opt to find the most balanced cut, which is not necessarily the case right
    // now
    std::vector<bool> result(view.NumberOfNodes(), true);
    std::size_t source_side_count = view.NumberOfNodes();
    for (auto itr = view.Begin(); itr != view.End(); ++itr)
    {
        if (is_sink_side(std::distance(view.Begin(), itr)))
        {
            result[std::distance(view.Begin(), itr)] = false;
            --source_side_count;
        }
    }
    std::size_t num_edges = 0;
    for (auto itr = view.Begin(); itr != view.End(); ++itr)
    {
        const auto nid = std::distance(view.Begin(), itr);
        const auto sink_side = is_sink_side(nid);
        for (const auto &edge : view.Edges(nid))
        {
            if (is_sink_side(edge.target) != sink_side)
            {
                ++num_edges;
            }
        }
    }
    return {source_side_count, num_edges, std::move(result)};
}

DinicMaxFlow::LevelGraph
DinicMaxFlow::ComputeLevelGraph(const GraphView &view,
                                const std::vector<NodeID> &border_source_nodes,
                                const SourceSinkNodes &source_nodes,
                                const SourceSinkNodes &sink_nodes,
                                const FlowEdges &flow) const
{
    LevelGraph levels(view.NumberOfNodes(), INVALID_LEVEL);
    std::queue<NodeID> level_queue;

    for (const auto node_id : border_source_nodes)
    {
        levels[node_id] = 0;
        level_queue.push(node_id);
        for (const auto &edge : view.Edges(node_id))
        {
            const auto target = edge.target;
            if (source_nodes.count(target))
            {
                levels[target] = 0;
            }
        }
    }
    // check if there is flow present on an edge
    const auto has_flow = [&](const NodeID from, const NodeID to) {
        return flow[from].find(to) != flow[from].end();
    };

    // perform a relaxation step in the BFS algorithm
    const auto relax_node = [&](const NodeID node_id, const Level level) {
        // don't relax sink nodes
        if (sink_nodes.count(node_id))
            return;

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
    for (Level level = 1; !level_queue.empty(); ++level)
    {
        // run through the current level
        auto steps = level_queue.size();
        while (steps--)
        {
            relax_node(level_queue.front(), level);
            level_queue.pop();
        }
    }

    return levels;
}

std::uint32_t DinicMaxFlow::BlockingFlow(FlowEdges &flow,
                                         LevelGraph &levels,
                                         const GraphView &view,
                                         const SourceSinkNodes &source_nodes,
                                         const std::vector<NodeID> &border_sink_nodes) const
{
    std::uint32_t flow_increase = 0;
    // augment the flow along a path in the level graph
    const auto augment_flow = [&flow, &view](const std::vector<NodeID> &path) {
        const auto augment_one = [&flow, &view](const NodeID from, const NodeID to) {

            // check if there is flow in the opposite direction
            auto existing_edge = flow[to].find(from);
            if (existing_edge != flow[to].end())
            {
                // remove flow from reverse edges first
                flow[to].erase(existing_edge);
            }
            else
            {
                // only add flow if no opposite flow exists
                flow[from].insert(to);
            }
            // for adjacent find
            return false;
        };

        // augment all adjacent edges
        std::adjacent_find(path.begin(), path.end(), augment_one);
    };

    // find and augment the blocking flow

    std::vector<std::pair<std::uint32_t, NodeID>> reached_sinks;
    for (auto sink : border_sink_nodes)
    {
        if (levels[sink] != INVALID_LEVEL)
        {
            reached_sinks.push_back(std::make_pair(levels[sink], sink));
        }
    }
    std::sort(reached_sinks.begin(), reached_sinks.end());

    for (auto sink_itr = reached_sinks.begin(); sink_itr != reached_sinks.end();)
    {

        auto path = GetAugmentingPath(levels, sink_itr->second, view, flow, source_nodes);

        if (!path.empty())
            augment_flow(path);
        else
            ++sink_itr;
    }

    return flow_increase;
}

std::vector<NodeID> DinicMaxFlow::GetAugmentingPath(LevelGraph &levels,
                                                    const NodeID node_id,
                                                    const GraphView &view,
                                                    const FlowEdges &flow,
                                                    const SourceSinkNodes &source_nodes) const
{
    std::vector<NodeID> path;
    BOOST_ASSERT(source_nodes.find(node_id) == source_nodes.end());

    // Keeps the local state of the DFS in forms of the iterators
    using DFSState = struct
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

} // namespace partition
} // namespace osrm
