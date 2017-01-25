#include "partition/dinic_max_flow.hpp"

#include <queue>

namespace osrm
{
namespace partition
{

DinicMaxFlow::PartitionResult DinicMaxFlow::operator()(const GraphView &view,
                                                       const SourceSinkNodes &source_nodes,
                                                       const SourceSinkNodes &sink_nodes) const
{
    FlowEdges flow;
    do
    {
        const auto levels = ComputeLevelGraph(view, source_nodes, flow);

        // check if the sink can be reached from the source
        const auto separated =
            std::find_if(sink_nodes.begin(), sink_nodes.end(), [&levels](const auto node) {
                return levels.find(node) != levels.end();
            }) == sink_nodes.end();

        // no further elements can be found
        if (separated)
        {
            // all elements within `levels` are on the source side
            PartitionResult result(view.NumberOfNodes(), true);
            for (auto itr = view.Begin(); itr != view.End(); ++itr)
            {
                if (levels.find(*itr) != levels.end())
                    result[std::distance(view.Begin(), itr)] = false;
            }
            return result;
        }

        AugmentFlow(flow, view, source_nodes, sink_nodes, levels);
    } while (true);
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
            const auto target = view.GetTarget(*itr);

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

void DinicMaxFlow::AugmentFlow(FlowEdges &flow,
                               const GraphView &view,
                               const SourceSinkNodes &source_nodes,
                               const SourceSinkNodes &sink_nodes,
                               const LevelGraph &levels) const
{
    const auto has_flow = [&](const auto from, const auto to) {
        return flow.find(std::make_pair(from, to)) != flow.end();
    };

    // find a path and augment the flow along its edges
    // do a dfs on the level graph, augment all paths that are found
    const auto find_and_augment_path = [&](const NodeID source) {
        std::vector<NodeID> path;
        auto has_path = findPath(source, path, view, levels, flow, sink_nodes);

        if (has_path)
        {
            NodeID last = path.back();
            path.pop_back();
            // augment the flow graph with the flow
            while (!path.empty())
            {
                flow.insert(std::make_pair(path.back(), last));

                // update the residual capacities correctly
                const auto reverse_flow = flow.find(std::make_pair(last, path.back()));
                if (reverse_flow != flow.end())
                    flow.erase(reverse_flow);

                last = path.back();
                path.pop_back();
            }
            return true;
        }
        else
        {
            return false;
        }
    };

    // find and augment the blocking flow
    for (auto source : source_nodes)
    {
        while (find_and_augment_path(source))
        { /*augment*/
        }
    }
}

bool DinicMaxFlow::findPath(const NodeID node_id,
                            std::vector<NodeID> &path,
                            const GraphView &view,
                            const LevelGraph &levels,
                            const FlowEdges &flow,
                            const SourceSinkNodes &sink_nodes) const
{
    path.push_back(node_id);
    if (sink_nodes.find(node_id) != sink_nodes.end())
        return true;

    const auto node_level = levels.find(node_id)->second;
    for (auto itr = view.EdgeBegin(node_id); itr != view.EdgeEnd(node_id); ++itr)
    {
        const auto target = view.GetTarget(*itr);

        // don't relax edges with flow on them
        if (flow.find(std::make_pair(node_id, target)) != flow.end())
            continue;

        // don't go back, only follow edges to new nodes
        const auto level = levels.find(target)->second;
        if( level != node_level + 1 )
            continue;

        if( findPath(target,path,view,levels,flow,sink_nodes) )
            return true;
    }
    path.pop_back();
    return false;
}

} // namespace partition
} // namespace osrm
