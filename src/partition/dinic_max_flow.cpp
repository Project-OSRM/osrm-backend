#include "partition/dinic_max_flow.hpp"

#include <queue>
#include <stack>

namespace osrm
{
namespace partition
{

DinicMaxFlow::MinCut DinicMaxFlow::operator()(const GraphView &view,
                                              const SourceSinkNodes &source_nodes,
                                              const SourceSinkNodes &sink_nodes) const
{
    // edges in current flow that have capacity
    // The graph (V,E) contains undirected edges for all (u,v) \in V x V. We describe the flow as a
    // set of vertices (s,t) with flow set to `true`. Since flow can be either from `s` to `t` or
    // from `t` to `s`, we can remove `(s,t)` from the flow, if we send flow back the first time,
    // and insert `(t,s)` only if we send flow again.
    FlowEdges flow;
    do
    {
        std::cout << "." << std::flush;
        auto levels = ComputeLevelGraph(view, source_nodes, flow);

        // check if the sink can be reached from the source
        const auto separated =
            std::find_if(sink_nodes.begin(), sink_nodes.end(), [&levels](const auto node) {
                return levels.find(node) != levels.end();
            }) == sink_nodes.end();

        if (!separated)
        {
            BlockingFlow(flow, levels, view, source_nodes, sink_nodes);
        }
        else
        {
            return MakeCut(view, levels);
        }
    } while (true);
}

DinicMaxFlow::MinCut DinicMaxFlow::MakeCut(const GraphView &view, const LevelGraph &levels) const
{

    // all elements within `levels` are on the source side
    // This part should opt to find the most balanced cut, which is not necessarily the case right
    // now
    std::vector<bool> result(view.NumberOfNodes(), true);
    for (auto itr = view.Begin(); itr != view.End(); ++itr)
    {
        if (levels.find(*itr) != levels.end())
            result[std::distance(view.Begin(), itr)] = false;
    }
    std::size_t num_edges = 0;
    for (auto itr = view.Begin(); itr != view.End(); ++itr)
    {
        const auto sink_side = levels.find(*itr) != levels.end();
        for (auto edge_itr = view.EdgeBegin(*itr); edge_itr != view.EdgeEnd(*itr); ++edge_itr)
        {
            if ((levels.find(view.GetEdge(*edge_itr).target) != levels.end()) != sink_side)
            {
                ++num_edges;
            }
        }
    }
    return {levels.size(), num_edges, std::move(result)};
}

DinicMaxFlow::LevelGraph DinicMaxFlow::ComputeLevelGraph(const GraphView &view,
                                                         const SourceSinkNodes &source_nodes,
                                                         const FlowEdges &flow) const
{
    LevelGraph levels;
    std::queue<NodeID> level_queue;

    for (const auto node : source_nodes)
    {
        levels[node] = 0;
        level_queue.push(node);
    }

    // check if there is flow present on an edge
    const auto has_flow = [&](const NodeID from, const NodeID to) {
        return flow.find(std::make_pair(from, to)) != flow.end();
    };

    // perform a relaxation step in the BFS algorithm
    const auto relax_node = [&](const NodeID node_id, const std::uint32_t level) {
        for (auto itr = view.EdgeBegin(node_id); itr != view.EdgeEnd(node_id); ++itr)
        {
            const auto target = view.GetEdge(*itr).target;

            // don't relax edges with flow on them
            if (has_flow(node_id, target))
                continue;

            // don't go back, only follow edges to new nodes
            if (levels.find(target) == levels.end())
            {
                level_queue.push(target);
                levels[target] = level;
            }
        }
    };

    // compute the levels of level graph using BFS
    for (std::uint32_t level = 1; !level_queue.empty(); ++level)
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

void DinicMaxFlow::BlockingFlow(FlowEdges &flow,
                                LevelGraph &levels,
                                const GraphView &view,
                                const SourceSinkNodes &source_nodes,
                                const SourceSinkNodes &sink_nodes) const
{
    // augment the flow along a path in the level graph
    const auto augment_flow = [&flow](const std::vector<NodeID> &path) {
        const auto augment_one = [&flow](const NodeID from, const NodeID to) {
            const auto flow_edge = std::make_pair(from, to);
            const auto reverse_flow_edge = std::make_pair(to, from);

            // check if there is flow in the opposite direction
            auto existing_edge = flow.find(reverse_flow_edge);
            if (existing_edge != flow.end())
            {
                // remove flow from reverse edges first
                flow.erase(existing_edge);
            }
            else
            {
                // only add flow if no opposite flow exists
                flow.insert(flow_edge);
            }
            // for adjacent find
            return false;
        };

        // augment all adjacent edges
        std::adjacent_find(path.begin(), path.end(), augment_one);
    };

    // find and augment the blocking flow
    auto sink_itr = sink_nodes.begin();
    while (sink_itr != sink_nodes.end())
    {
        if (levels.count(*sink_itr))
        {
            auto path = GetAugmentingPath(levels, *sink_itr, view, flow, source_nodes);

            if (!path.empty())
                augment_flow(path);
            else
                ++sink_itr;
        }
        else
            ++sink_itr;
    }
}

std::vector<NodeID> DinicMaxFlow::GetAugmentingPath(LevelGraph &levels,
                                                    const NodeID node_id,
                                                    const GraphView &view,
                                                    const FlowEdges &flow,
                                                    const SourceSinkNodes &source_nodes) const
{
    std::vector<NodeID> path;
    BOOST_ASSERT(sink_nodes.find(node_id) == sink_nodes.end());

    // Keeps the local state of the DFS in forms of the iterators
    using DFSState = struct
    {
        GraphView::EdgeIterator edge_iterator;
        const GraphView::EdgeIterator end_iterator;
    };

    std::stack<DFSState> dfs_stack;
    dfs_stack.push({view.EdgeBegin(node_id), view.EdgeEnd(node_id)});
    path.push_back(node_id);

    while (!dfs_stack.empty())
    {
        // the dfs_stack and the path have to be kept in sync
        BOOST_ASSERT(dfs_stack.size() == path.size());

        while (dfs_stack.top().edge_iterator != dfs_stack.top().end_iterator)
        {
            const auto target = view.GetEdge(*dfs_stack.top().edge_iterator).target;

            // look at every edge only once, so advance the state of the current node (last in path)
            dfs_stack.top().edge_iterator++;

            // check if the edge is valid
            const auto has_capacity = flow.find(std::make_pair(target, path.back())) == flow.end();
            const auto descends_level_graph =
                levels.count(target) != 0 &&
                levels.find(target)->second + 1 == levels.find(path.back())->second;

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
                dfs_stack.push({view.EdgeBegin(target), view.EdgeEnd(target)});
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
