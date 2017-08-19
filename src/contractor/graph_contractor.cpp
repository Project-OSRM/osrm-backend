#include "contractor/graph_contractor.hpp"

namespace osrm
{
namespace contractor
{

GraphContractor::GraphContractor(ContractorGraph graph_)
    : GraphContractor(std::move(graph_), {}, {})
{
}

GraphContractor::GraphContractor(ContractorGraph graph_,
                                 std::vector<float> node_levels_,
                                 std::vector<EdgeWeight> node_weights_)
    : graph(std::move(graph_)), orig_node_id_from_new_node_id_map(graph.GetNumberOfNodes()),
      node_levels(std::move(node_levels_)), node_weights(std::move(node_weights_))
{
    // Fill the map with an identiy mapping
    std::iota(
        orig_node_id_from_new_node_id_map.begin(), orig_node_id_from_new_node_id_map.end(), 0);
}

/* Reorder nodes for better locality during contraction */
void GraphContractor::RenumberGraph(ThreadDataContainer &thread_data_list,
                                    std::vector<RemainingNodeData> &remaining_nodes,
                                    std::vector<float> &node_priorities)
{
    // Delete old heap data to free memory that we need for the coming operations
    thread_data_list.data.clear();
    std::vector<NodeID> new_node_id_from_current_node_id(graph.GetNumberOfNodes(), SPECIAL_NODEID);

    // we need to make a copy here because we are going to modify it
    auto to_orig = orig_node_id_from_new_node_id_map;

    auto new_node_id = 0;

    // All remaining nodes get the low IDs
    for (auto &remaining : remaining_nodes)
    {
        auto id = new_node_id++;
        new_node_id_from_current_node_id[remaining.id] = id;
        orig_node_id_from_new_node_id_map[id] = to_orig[remaining.id];
        remaining.id = id;
    }

    // Already contracted nodes get the high IDs
    for (const auto current_id : util::irange<std::size_t>(0, graph.GetNumberOfNodes()))
    {
        if (new_node_id_from_current_node_id[current_id] == SPECIAL_NODEID)
        {
            auto id = new_node_id++;
            new_node_id_from_current_node_id[current_id] = id;
            orig_node_id_from_new_node_id_map[id] = to_orig[current_id];
        }
    }
    BOOST_ASSERT(new_node_id == graph.GetNumberOfNodes());

    util::inplacePermutation(
        node_priorities.begin(), node_priorities.end(), new_node_id_from_current_node_id);
    util::inplacePermutation(
        node_weights.begin(), node_weights.end(), new_node_id_from_current_node_id);
    util::inplacePermutation(
        node_levels.begin(), node_levels.end(), new_node_id_from_current_node_id);
    util::inplacePermutation(
        is_core_node.begin(), is_core_node.end(), new_node_id_from_current_node_id);
    graph.Renumber(new_node_id_from_current_node_id);

    // Renumber all shortcut node IDs
    for (const auto node : util::irange<NodeID>(0, graph.GetNumberOfNodes()))
    {
        for (const auto edge : graph.GetAdjacentEdgeRange(node))
        {
            auto &data = graph.GetEdgeData(edge);
            if (data.shortcut)
            {
                data.id = new_node_id_from_current_node_id[data.id];
            }
        }
    }
}

void GraphContractor::Run(double core_factor)
{
    // for the preperation we can use a big grain size, which is much faster (probably cache)
    const constexpr size_t InitGrainSize = 100000;
    const constexpr size_t PQGrainSize = 100000;
    // auto_partitioner will automatically increase the blocksize if we have
    // a lot of data. It is *important* for the last loop iterations
    // (which have a very small dataset) that it is devisible.
    const constexpr size_t IndependentGrainSize = 1;
    const constexpr size_t ContractGrainSize = 1;
    const constexpr size_t NeighboursGrainSize = 1;
    const constexpr size_t DeleteGrainSize = 1;

    const NodeID number_of_nodes = graph.GetNumberOfNodes();

    ThreadDataContainer thread_data_list(number_of_nodes);

    NodeID number_of_contracted_nodes = 0;
    std::vector<NodeDepth> node_depth;
    std::vector<float> node_priorities;
    is_core_node.resize(number_of_nodes, false);

    std::vector<RemainingNodeData> remaining_nodes(number_of_nodes);
    // initialize priorities in parallel
    tbb::parallel_for(tbb::blocked_range<NodeID>(0, number_of_nodes, InitGrainSize),
                      [this, &remaining_nodes](const tbb::blocked_range<NodeID> &range) {
                          for (auto x = range.begin(), end = range.end(); x != end; ++x)
                          {
                              remaining_nodes[x].id = x;
                          }
                      });

    bool use_cached_node_priorities = !node_levels.empty();
    if (use_cached_node_priorities)
    {
        util::UnbufferedLog log;
        log << "using cached node priorities ...";
        node_priorities.swap(node_levels);
        log << "ok";
    }
    else
    {
        node_depth.resize(number_of_nodes, 0);
        node_priorities.resize(number_of_nodes);
        node_levels.resize(number_of_nodes);

        util::UnbufferedLog log;
        log << "initializing elimination PQ ...";
        tbb::parallel_for(tbb::blocked_range<NodeID>(0, number_of_nodes, PQGrainSize),
                          [this, &node_priorities, &node_depth, &thread_data_list](
                              const tbb::blocked_range<NodeID> &range) {
                              ContractorThreadData *data = thread_data_list.GetThreadData();
                              for (auto x = range.begin(), end = range.end(); x != end; ++x)
                              {
                                  node_priorities[x] =
                                      this->EvaluateNodePriority(data, node_depth[x], x);
                              }
                          });
        log << "ok";
    }
    BOOST_ASSERT(node_priorities.size() == number_of_nodes);

    util::Log() << "preprocessing " << number_of_nodes << " nodes ...";

    util::UnbufferedLog log;
    util::Percent p(log, number_of_nodes);

    unsigned current_level = 0;
    std::size_t next_renumbering = number_of_nodes * 0.65 * core_factor;
    while (remaining_nodes.size() > 1 &&
           number_of_contracted_nodes < static_cast<NodeID>(number_of_nodes * core_factor))
    {
        if (number_of_contracted_nodes > next_renumbering)
        {
            RenumberGraph(thread_data_list, remaining_nodes, node_priorities);
            log << "[renumbered]";
            // only one renumbering for now
            next_renumbering = number_of_nodes;
        }

        tbb::parallel_for(
            tbb::blocked_range<NodeID>(0, remaining_nodes.size(), IndependentGrainSize),
            [this, &node_priorities, &remaining_nodes, &thread_data_list](
                const tbb::blocked_range<NodeID> &range) {
                ContractorThreadData *data = thread_data_list.GetThreadData();
                // determine independent node set
                for (auto i = range.begin(), end = range.end(); i != end; ++i)
                {
                    const NodeID node = remaining_nodes[i].id;
                    remaining_nodes[i].is_independent =
                        this->IsNodeIndependent(node_priorities, data, node);
                }
            });

        // sort all remaining nodes to the beginning of the sequence
        const auto begin_independent_nodes =
            stable_partition(remaining_nodes.begin(),
                             remaining_nodes.end(),
                             [](RemainingNodeData node_data) { return !node_data.is_independent; });
        auto begin_independent_nodes_idx =
            std::distance(remaining_nodes.begin(), begin_independent_nodes);
        auto end_independent_nodes_idx = remaining_nodes.size();

        if (!use_cached_node_priorities)
        {
            // write out contraction level
            tbb::parallel_for(
                tbb::blocked_range<NodeID>(
                    begin_independent_nodes_idx, end_independent_nodes_idx, ContractGrainSize),
                [this, remaining_nodes, current_level](const tbb::blocked_range<NodeID> &range) {
                    for (auto position = range.begin(), end = range.end(); position != end;
                         ++position)
                    {
                        node_levels[remaining_nodes[position].id] = current_level;
                    }
                });
        }

        // contract independent nodes
        tbb::parallel_for(
            tbb::blocked_range<NodeID>(
                begin_independent_nodes_idx, end_independent_nodes_idx, ContractGrainSize),
            [this, &remaining_nodes, &thread_data_list](const tbb::blocked_range<NodeID> &range) {
                ContractorThreadData *data = thread_data_list.GetThreadData();
                for (auto position = range.begin(), end = range.end(); position != end; ++position)
                {
                    const NodeID x = remaining_nodes[position].id;
                    this->ContractNode<false>(data, x);
                }
            });

        tbb::parallel_for(
            tbb::blocked_range<NodeID>(
                begin_independent_nodes_idx, end_independent_nodes_idx, DeleteGrainSize),
            [this, &remaining_nodes, &thread_data_list](const tbb::blocked_range<NodeID> &range) {
                ContractorThreadData *data = thread_data_list.GetThreadData();
                for (auto position = range.begin(), end = range.end(); position != end; ++position)
                {
                    const NodeID x = remaining_nodes[position].id;
                    this->DeleteIncomingEdges(data, x);
                }
            });

        // make sure we really sort each block
        tbb::parallel_for(thread_data_list.data.range(),
                          [&](const ThreadDataContainer::EnumerableThreadData::range_type &range) {
                              for (auto &data : range)
                                  tbb::parallel_sort(data->inserted_edges.begin(),
                                                     data->inserted_edges.end());
                          });

        // insert new edges
        for (auto &data : thread_data_list.data)
        {
            for (const ContractorEdge &edge : data->inserted_edges)
            {
                const EdgeID current_edge_ID = graph.FindEdge(edge.source, edge.target);
                if (current_edge_ID != SPECIAL_EDGEID)
                {
                    ContractorGraph::EdgeData &current_data = graph.GetEdgeData(current_edge_ID);
                    if (current_data.shortcut && edge.data.forward == current_data.forward &&
                        edge.data.backward == current_data.backward)
                    {
                        // found a duplicate edge with smaller weight, update it.
                        if (edge.data.weight < current_data.weight)
                        {
                            current_data = edge.data;
                        }
                        // don't insert duplicates
                        continue;
                    }
                }
                graph.InsertEdge(edge.source, edge.target, edge.data);
            }
            data->inserted_edges.clear();
        }

        if (!use_cached_node_priorities)
        {
            tbb::parallel_for(
                tbb::blocked_range<NodeID>(
                    begin_independent_nodes_idx, end_independent_nodes_idx, NeighboursGrainSize),
                [this, &node_priorities, &remaining_nodes, &node_depth, &thread_data_list](
                    const tbb::blocked_range<NodeID> &range) {
                    ContractorThreadData *data = thread_data_list.GetThreadData();
                    for (auto position = range.begin(), end = range.end(); position != end;
                         ++position)
                    {
                        NodeID x = remaining_nodes[position].id;
                        this->UpdateNodeNeighbours(node_priorities, node_depth, data, x);
                    }
                });
        }

        // remove contracted nodes from the pool
        BOOST_ASSERT(end_independent_nodes_idx - begin_independent_nodes_idx > 0);
        number_of_contracted_nodes += end_independent_nodes_idx - begin_independent_nodes_idx;
        remaining_nodes.resize(begin_independent_nodes_idx);

        p.PrintStatus(number_of_contracted_nodes);
        ++current_level;
    }

    if (remaining_nodes.size() > 2)
    {
        tbb::parallel_for(tbb::blocked_range<NodeID>(0, remaining_nodes.size(), InitGrainSize),
                          [this, &remaining_nodes](const tbb::blocked_range<NodeID> &range) {
                              for (auto x = range.begin(), end = range.end(); x != end; ++x)
                              {
                                  is_core_node[remaining_nodes[x].id] = true;
                              }
                          });
    }
    else
    {
        // in this case we don't need core markers since we fully contracted
        // the graph
        is_core_node.clear();
    }

    log << "\n";

    util::Log() << "[core] " << remaining_nodes.size() << " nodes " << graph.GetNumberOfEdges()
                << " edges.";

    util::inplacePermutation(node_levels.begin(), node_levels.end(), orig_node_id_from_new_node_id_map);
    util::inplacePermutation(is_core_node.begin(), is_core_node.end(), orig_node_id_from_new_node_id_map);

    thread_data_list.data.clear();
}

// Can only be called once because it invalides the marker
std::vector<bool> GraphContractor::GetCoreMarker() { return std::move(is_core_node); }

// Can only be called once because it invalides the node levels
std::vector<float> GraphContractor::GetNodeLevels() { return std::move(node_levels); }

float GraphContractor::EvaluateNodePriority(ContractorThreadData *const data,
                                            const NodeDepth node_depth,
                                            const NodeID node)
{
    ContractionStats stats;

    // perform simulated contraction
    ContractNode<true>(data, node, &stats);

    // Result will contain the priority
    float result;
    if (0 == (stats.edges_deleted_count * stats.original_edges_deleted_count))
    {
        result = 1.f * node_depth;
    }
    else
    {
        result =
            2.f * (((float)stats.edges_added_count) / stats.edges_deleted_count) +
            4.f * (((float)stats.original_edges_added_count) / stats.original_edges_deleted_count) +
            1.f * node_depth;
    }
    BOOST_ASSERT(result >= 0);
    return result;
}

void GraphContractor::DeleteIncomingEdges(ContractorThreadData *data, const NodeID node)
{
    std::vector<NodeID> &neighbours = data->neighbours;
    neighbours.clear();

    // find all neighbours
    for (auto e : graph.GetAdjacentEdgeRange(node))
    {
        const NodeID u = graph.GetTarget(e);
        if (u != node)
        {
            neighbours.push_back(u);
        }
    }
    // eliminate duplicate entries ( forward + backward edges )
    std::sort(neighbours.begin(), neighbours.end());
    neighbours.resize(std::unique(neighbours.begin(), neighbours.end()) - neighbours.begin());

    for (const auto i : util::irange<std::size_t>(0, neighbours.size()))
    {
        graph.DeleteEdgesTo(neighbours[i], node);
    }
}

bool GraphContractor::UpdateNodeNeighbours(std::vector<float> &priorities,
                                           std::vector<NodeDepth> &node_depth,
                                           ContractorThreadData *const data,
                                           const NodeID node)
{
    std::vector<NodeID> &neighbours = data->neighbours;
    neighbours.clear();

    // find all neighbours
    for (auto e : graph.GetAdjacentEdgeRange(node))
    {
        const NodeID u = graph.GetTarget(e);
        if (u == node)
        {
            continue;
        }
        neighbours.push_back(u);
        node_depth[u] = std::max(node_depth[node] + 1, node_depth[u]);
    }
    // eliminate duplicate entries ( forward + backward edges )
    std::sort(neighbours.begin(), neighbours.end());
    neighbours.resize(std::unique(neighbours.begin(), neighbours.end()) - neighbours.begin());

    // re-evaluate priorities of neighboring nodes
    for (const NodeID u : neighbours)
    {
        priorities[u] = EvaluateNodePriority(data, node_depth[u], u);
    }
    return true;
}

bool GraphContractor::IsNodeIndependent(const std::vector<float> &priorities,
                                        ContractorThreadData *const data,
                                        NodeID node) const
{
    const float priority = priorities[node];

    std::vector<NodeID> &neighbours = data->neighbours;
    neighbours.clear();

    for (auto e : graph.GetAdjacentEdgeRange(node))
    {
        const NodeID target = graph.GetTarget(e);
        if (node == target)
        {
            continue;
        }
        const float target_priority = priorities[target];
        BOOST_ASSERT(target_priority >= 0);
        // found a neighbour with lower priority?
        if (priority > target_priority)
        {
            return false;
        }
        // tie breaking
        if (std::abs(priority - target_priority) < std::numeric_limits<float>::epsilon() &&
            Bias(node, target))
        {
            return false;
        }
        neighbours.push_back(target);
    }

    std::sort(neighbours.begin(), neighbours.end());
    neighbours.resize(std::unique(neighbours.begin(), neighbours.end()) - neighbours.begin());

    // examine all neighbours that are at most 2 hops away
    for (const NodeID u : neighbours)
    {
        for (auto e : graph.GetAdjacentEdgeRange(u))
        {
            const NodeID target = graph.GetTarget(e);
            if (node == target)
            {
                continue;
            }
            const float target_priority = priorities[target];
            BOOST_ASSERT(target_priority >= 0);
            // found a neighbour with lower priority?
            if (priority > target_priority)
            {
                return false;
            }
            // tie breaking
            if (std::abs(priority - target_priority) < std::numeric_limits<float>::epsilon() &&
                Bias(node, target))
            {
                return false;
            }
        }
    }
    return true;
}

// This bias function takes up 22 assembly instructions in total on X86
bool GraphContractor::Bias(const NodeID a, const NodeID b) const
{
    const unsigned short hasha = fast_hash(a);
    const unsigned short hashb = fast_hash(b);

    // The compiler optimizes that to conditional register flags but without branching
    // statements!
    if (hasha != hashb)
    {
        return hasha < hashb;
    }
    return a < b;
}

} // namespace contractor
} // namespace osrm
