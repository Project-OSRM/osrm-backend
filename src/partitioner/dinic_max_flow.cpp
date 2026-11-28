#include "partitioner/dinic_max_flow.hpp"

#include <algorithm>
#include <limits>
#include <queue>
#include <stack>
#include <unordered_set>

namespace osrm::partitioner
{

namespace
{

const auto constexpr INVALID_LEVEL = std::numeric_limits<DinicMaxFlow::Level>::max();

} // end namespace

class DinicMaxFlow::FlowEdges
{
  public:
    explicit FlowEdges(const std::size_t size) : m_targets(size, SPECIAL_NODEID) {}

    auto outgoing(const NodeID from) const
    {
        const auto target = m_targets[from];
        return [this, from, target](const NodeID to)
        { return target == from ? m_overflow.contains(key(from, to)) : target == to; };
    }

    bool contains(const NodeID from, const NodeID to) const { return outgoing(from)(to); }

    bool erase(const NodeID from, const NodeID to)
    {
        auto &target = m_targets[from];
        if (target == from)
            return m_overflow.erase(key(from, to)) != 0;
        if (target != to)
            return false;
        target = SPECIAL_NODEID;
        return true;
    }

    void insert(const NodeID from, const NodeID to)
    {
        BOOST_ASSERT(from != to && !contains(from, to));

        auto &target = m_targets[from];
        if (target == SPECIAL_NODEID)
        {
            target = to;
            return;
        }

        // A self-loop cannot carry augmenting flow, so the source ID marks overflow.
        if (target != from)
        {
            m_overflow.insert(key(from, target));
            target = from;
        }
        m_overflow.insert(key(from, to));
    }

  private:
    static std::uint64_t key(const NodeID from, const NodeID to)
    { return (static_cast<std::uint64_t>(from) << std::numeric_limits<NodeID>::digits) | to; }

    std::vector<NodeID> m_targets;
    std::unordered_set<std::uint64_t> m_overflow;
};

std::optional<DinicMaxFlow::MinCut>
DinicMaxFlow::operator()(const BisectionGraphView &view,
                         std::span<const Terminal> terminals,
                         const std::atomic<std::size_t> *cut_limit) const
{
    BOOST_ASSERT(terminals.size() == view.NumberOfNodes());

    // Start only from the boundary of each terminal region.
    std::vector<NodeID> border_source_nodes;
    std::vector<NodeID> border_sink_nodes;
    for (std::size_t index = 0; index < terminals.size(); ++index)
    {
        const auto terminal = terminals[index];
        if (terminal == Terminal::None)
            continue;

        const auto node = static_cast<NodeID>(index);
        const auto edges = view.Edges(node);
        if (std::ranges::any_of(
                edges, [&](const auto &edge) { return terminals[edge.target] != terminal; }))
        {
            auto &border = terminal == Terminal::Source ? border_source_nodes : border_sink_nodes;
            border.push_back(node);
        }
    }

    // edges in current flow that have capacity
    // The graph (V,E) contains undirected edges for all (u,v) \in V x V. We describe the flow as a
    // set of vertices (s,t) with flow set to `true`. Since flow can be either from `s` to `t` or
    // from `t` to `s`, we can remove `(s,t)` from the flow, if we send flow back the first time,
    // and insert `(t,s)` only if we send flow again.

    // allocate storage for the flow
    FlowEdges flow(view.NumberOfNodes());
    std::size_t flow_value = 0;
    while (cut_limit == nullptr || flow_value <= cut_limit->load(std::memory_order_relaxed))
    {
        auto levels = ComputeLevelGraph(view, border_source_nodes, terminals, flow);

        // check if the sink can be reached from the source, it's enough to check the border
        const auto separated =
            std::find_if(border_sink_nodes.begin(),
                         border_sink_nodes.end(),
                         [&levels](const auto node)
                         { return levels[node] != INVALID_LEVEL; }) == border_sink_nodes.end();

        if (!separated)
        {
            flow_value += BlockingFlow(flow, levels, view, terminals, border_sink_nodes);
        }
        else
        {
            return MakeCut(terminals, levels, flow_value);
        }
    }
    return std::nullopt;
}

DinicMaxFlow::MinCut DinicMaxFlow::MakeCut(std::span<const Terminal> terminals,
                                           const LevelGraph &levels,
                                           const std::size_t flow_value) const
{
    BOOST_ASSERT(terminals.size() == levels.size());

    // Interior sources are also on the source side, although BFS skips them.
    std::vector<bool> result(levels.size());
    std::size_t source_side_count = 0;
    for (std::size_t node = 0; node < levels.size(); ++node)
    {
        const auto on_source_side =
            levels[node] != INVALID_LEVEL || terminals[node] == Terminal::Source;
        result[node] = on_source_side;
        source_side_count += on_source_side;
    }
    return {source_side_count, flow_value, std::move(result)};
}

DinicMaxFlow::LevelGraph
DinicMaxFlow::ComputeLevelGraph(const BisectionGraphView &view,
                                const std::vector<NodeID> &border_source_nodes,
                                std::span<const Terminal> terminals,
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
            if (terminals[edge.target] == Terminal::Source)
                levels[edge.target] = 0;
    }
    while (!level_queue.empty())
    {
        const auto node = level_queue.front();
        level_queue.pop();

        const auto next_level = levels[node] + 1;
        const auto has_flow = flow.outgoing(node);
        for (const auto &edge : view.Edges(node))
        {
            const auto target = edge.target;
            if (levels[target] <= next_level || has_flow(target))
                continue;

            levels[target] = next_level;
            if (terminals[target] != Terminal::Sink)
                level_queue.push(target);
        }
    }

    return levels;
}

std::size_t DinicMaxFlow::BlockingFlow(FlowEdges &flow,
                                       LevelGraph &levels,
                                       const BisectionGraphView &view,
                                       std::span<const Terminal> terminals,
                                       const std::vector<NodeID> &border_sink_nodes) const
{
    // track the number of augmenting paths (which in sum will equal the number of unique border
    // edges) (since our graph is undirected)
    std::size_t flow_increase = 0;

    // augment the flow along a path in the level graph
    const auto augment_flow = [&flow](const std::vector<NodeID> &path)
    {
        // add/remove flow edges from the current residual graph
        const auto augment_one = [&flow](const NodeID from, const NodeID to)
        {
            // check if there is flow in the opposite direction
            if (!flow.erase(to, from))
                flow.insert(from, to);

            // do augmentation on all pairs, never stop early:
            return false;
        };

        // augment all adjacent edges
        // We're only interested in the side-effect of the augment_one function, the return
        // value is ignored
        [[maybe_unused]] auto _ = std::adjacent_find(path.begin(), path.end(), augment_one);
    };

    const auto augment_all_paths = [&](const NodeID sink_node_id)
    {
        // only augment sinks
        if (levels[sink_node_id] == INVALID_LEVEL)
            return;

        while (true)
        {
            // as long as there are augmenting paths from the sink, add them
            const auto path = GetAugmentingPath(levels, sink_node_id, view, flow, terminals);
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
                                                    std::span<const Terminal> terminals) const
{
    std::vector<NodeID> path;
    BOOST_ASSERT(terminals[node_id] != Terminal::Source);

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
            const auto has_capacity = !flow.contains(target, path.back());
            const auto descends_level_graph = levels[target] + 1 == levels[path.back()];

            if (has_capacity && descends_level_graph)
            {
                // recurse
                path.push_back(target);

                // termination
                if (terminals[target] == Terminal::Source)
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

} // namespace osrm::partitioner
