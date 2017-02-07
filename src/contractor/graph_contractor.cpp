#include "contractor/graph_contractor.hpp"

namespace osrm
{
namespace contractor
{

GraphContractor::GraphContractor(int nodes, std::vector<ContractorEdge> input_edge_list)
    : GraphContractor(nodes, std::move(input_edge_list), {}, {})
{
}

GraphContractor::GraphContractor(int nodes,
                                 std::vector<ContractorEdge> edges,
                                 std::vector<float> &&node_levels_,
                                 std::vector<EdgeWeight> &&node_weights_)
    : node_levels(std::move(node_levels_)), node_weights(std::move(node_weights_))
{
    tbb::parallel_sort(edges.begin(), edges.end());
    NodeID edge = 0;
    for (NodeID i = 0; i < edges.size();)
    {
        const NodeID source = edges[i].source;
        const NodeID target = edges[i].target;
        const NodeID id = edges[i].data.id;
        // remove eigenloops
        if (source == target)
        {
            ++i;
            continue;
        }
        ContractorEdge forward_edge;
        ContractorEdge reverse_edge;
        forward_edge.source = reverse_edge.source = source;
        forward_edge.target = reverse_edge.target = target;
        forward_edge.data.forward = reverse_edge.data.backward = true;
        forward_edge.data.backward = reverse_edge.data.forward = false;
        forward_edge.data.shortcut = reverse_edge.data.shortcut = false;
        forward_edge.data.id = reverse_edge.data.id = id;
        forward_edge.data.originalEdges = reverse_edge.data.originalEdges = 1;
        forward_edge.data.weight = reverse_edge.data.weight = INVALID_EDGE_WEIGHT;
        forward_edge.data.duration = reverse_edge.data.duration = MAXIMAL_EDGE_DURATION;
        // remove parallel edges
        while (i < edges.size() && edges[i].source == source && edges[i].target == target)
        {
            if (edges[i].data.forward)
            {
                forward_edge.data.weight = std::min(edges[i].data.weight, forward_edge.data.weight);
                forward_edge.data.duration =
                    std::min(edges[i].data.duration, forward_edge.data.duration);
            }
            if (edges[i].data.backward)
            {
                reverse_edge.data.weight = std::min(edges[i].data.weight, reverse_edge.data.weight);
                reverse_edge.data.duration =
                    std::min(edges[i].data.duration, reverse_edge.data.duration);
            }
            ++i;
        }
        // merge edges (s,t) and (t,s) into bidirectional edge
        if (forward_edge.data.weight == reverse_edge.data.weight)
        {
            if ((int)forward_edge.data.weight != INVALID_EDGE_WEIGHT)
            {
                forward_edge.data.backward = true;
                edges[edge++] = forward_edge;
            }
        }
        else
        { // insert seperate edges
            if (((int)forward_edge.data.weight) != INVALID_EDGE_WEIGHT)
            {
                edges[edge++] = forward_edge;
            }
            if ((int)reverse_edge.data.weight != INVALID_EDGE_WEIGHT)
            {
                edges[edge++] = reverse_edge;
            }
        }
    }
    util::Log() << "merged " << edges.size() - edge << " edges out of " << edges.size();
    edges.resize(edge);
    contractor_graph = std::make_shared<ContractorGraph>(nodes, edges);
    edges.clear();
    edges.shrink_to_fit();

    BOOST_ASSERT(0 == edges.capacity());
    util::Log() << "contractor finished initalization";
}

/* Flush all data from the contraction to disc and reorder stuff for better locality */
void GraphContractor::FlushDataAndRebuildContractorGraph(
    ThreadDataContainer &thread_data_list,
    std::vector<RemainingNodeData> &remaining_nodes,
    std::vector<float> &node_priorities)
{
    util::DeallocatingVector<ContractorEdge> new_edge_set; // this one is not explicitely
                                                           // cleared since it goes out of
                                                           // scope anywa
    // Delete old heap data to free memory that we need for the coming operations
    thread_data_list.data.clear();
    // Create new priority array
    std::vector<float> new_node_priority(remaining_nodes.size());
    std::vector<EdgeWeight> new_node_weights(remaining_nodes.size());
    // this map gives the old IDs from the new ones, necessary to get a consistent graph
    // at the end of contraction
    orig_node_id_from_new_node_id_map.resize(remaining_nodes.size());
    // this map gives the new IDs from the old ones, necessary to remap targets from the
    // remaining graph
    const auto number_of_nodes = contractor_graph->GetNumberOfNodes();
    std::vector<NodeID> new_node_id_from_orig_id_map(number_of_nodes, SPECIAL_NODEID);
    for (const auto new_node_id : util::irange<std::size_t>(0UL, remaining_nodes.size()))
    {
        auto &node = remaining_nodes[new_node_id];
        BOOST_ASSERT(node_priorities.size() > node.id);
        new_node_priority[new_node_id] = node_priorities[node.id];
        BOOST_ASSERT(node_weights.size() > node.id);
        new_node_weights[new_node_id] = node_weights[node.id];
    }
    // build forward and backward renumbering map and remap ids in remaining_nodes
    for (const auto new_node_id : util::irange<std::size_t>(0UL, remaining_nodes.size()))
    {
        auto &node = remaining_nodes[new_node_id];
        // create renumbering maps in both directions
        orig_node_id_from_new_node_id_map[new_node_id] = node.id;
        new_node_id_from_orig_id_map[node.id] = new_node_id;
        node.id = new_node_id;
    }
    // walk over all nodes
    for (const auto source : util::irange<NodeID>(0UL, contractor_graph->GetNumberOfNodes()))
    {
        for (auto current_edge : contractor_graph->GetAdjacentEdgeRange(source))
        {
            ContractorGraph::EdgeData &data = contractor_graph->GetEdgeData(current_edge);
            const NodeID target = contractor_graph->GetTarget(current_edge);
            if (SPECIAL_NODEID == new_node_id_from_orig_id_map[source])
            {
                external_edge_list.push_back({source, target, data});
            }
            else
            {
                // node is not yet contracted.
                // add (renumbered) outgoing edges to new util::DynamicGraph.
                ContractorEdge new_edge = {new_node_id_from_orig_id_map[source],
                                           new_node_id_from_orig_id_map[target],
                                           data};
                new_edge.data.is_original_via_node_ID = true;
                BOOST_ASSERT_MSG(SPECIAL_NODEID != new_node_id_from_orig_id_map[source],
                                 "new source id not resolveable");
                BOOST_ASSERT_MSG(SPECIAL_NODEID != new_node_id_from_orig_id_map[target],
                                 "new target id not resolveable");
                new_edge_set.push_back(new_edge);
            }
        }
    }
    // Replace old priorities array by new one
    node_priorities.swap(new_node_priority);
    // Delete old node_priorities vector
    node_weights.swap(new_node_weights);
    // old Graph is removed
    contractor_graph.reset();
    // create new graph
    tbb::parallel_sort(new_edge_set.begin(), new_edge_set.end());
    contractor_graph = std::make_shared<ContractorGraph>(remaining_nodes.size(), new_edge_set);
    new_edge_set.clear();
    // INFO: MAKE SURE THIS IS THE LAST OPERATION OF THE FLUSH!
    // reinitialize heaps and ThreadData objects with appropriate size
    thread_data_list.number_of_nodes = contractor_graph->GetNumberOfNodes();
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

    const NodeID number_of_nodes = contractor_graph->GetNumberOfNodes();

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
    bool flushed_contractor = false;
    while (remaining_nodes.size() > 1 &&
           number_of_contracted_nodes < static_cast<NodeID>(number_of_nodes * core_factor))
    {
        if (!flushed_contractor && (number_of_contracted_nodes >
                                    static_cast<NodeID>(number_of_nodes * 0.65 * core_factor)))
        {
            log << " [flush " << number_of_contracted_nodes << " nodes] ";

            FlushDataAndRebuildContractorGraph(thread_data_list, remaining_nodes, node_priorities);

            flushed_contractor = true;
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
                [this, remaining_nodes, flushed_contractor, current_level](
                    const tbb::blocked_range<NodeID> &range) {
                    if (flushed_contractor)
                    {
                        for (auto position = range.begin(), end = range.end(); position != end;
                             ++position)
                        {
                            const NodeID x = remaining_nodes[position].id;
                            node_levels[orig_node_id_from_new_node_id_map[x]] = current_level;
                        }
                    }
                    else
                    {
                        for (auto position = range.begin(), end = range.end(); position != end;
                             ++position)
                        {
                            const NodeID x = remaining_nodes[position].id;
                            node_levels[x] = current_level;
                        }
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
                const EdgeID current_edge_ID = contractor_graph->FindEdge(edge.source, edge.target);
                if (current_edge_ID != SPECIAL_EDGEID)
                {
                    ContractorGraph::EdgeData &current_data =
                        contractor_graph->GetEdgeData(current_edge_ID);
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
                contractor_graph->InsertEdge(edge.source, edge.target, edge.data);
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
        if (flushed_contractor)
        {
            tbb::parallel_for(tbb::blocked_range<NodeID>(0, remaining_nodes.size(), InitGrainSize),
                              [this, &remaining_nodes](const tbb::blocked_range<NodeID> &range) {
                                  for (auto x = range.begin(), end = range.end(); x != end; ++x)
                                  {
                                      const auto orig_id = remaining_nodes[x].id;
                                      is_core_node[orig_node_id_from_new_node_id_map[orig_id]] =
                                          true;
                                  }
                              });
        }
        else
        {
            tbb::parallel_for(tbb::blocked_range<NodeID>(0, remaining_nodes.size(), InitGrainSize),
                              [this, &remaining_nodes](const tbb::blocked_range<NodeID> &range) {
                                  for (auto x = range.begin(), end = range.end(); x != end; ++x)
                                  {
                                      const auto orig_id = remaining_nodes[x].id;
                                      is_core_node[orig_id] = true;
                                  }
                              });
        }
    }
    else
    {
        // in this case we don't need core markers since we fully contracted
        // the graph
        is_core_node.clear();
    }

    util::Log() << "[core] " << remaining_nodes.size() << " nodes "
                << contractor_graph->GetNumberOfEdges() << " edges.";

    thread_data_list.data.clear();
}

void GraphContractor::GetCoreMarker(std::vector<bool> &out_is_core_node)
{
    out_is_core_node.swap(is_core_node);
}

void GraphContractor::GetNodeLevels(std::vector<float> &out_node_levels)
{
    out_node_levels.swap(node_levels);
}

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
    for (auto e : contractor_graph->GetAdjacentEdgeRange(node))
    {
        const NodeID u = contractor_graph->GetTarget(e);
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
        contractor_graph->DeleteEdgesTo(neighbours[i], node);
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
    for (auto e : contractor_graph->GetAdjacentEdgeRange(node))
    {
        const NodeID u = contractor_graph->GetTarget(e);
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

    for (auto e : contractor_graph->GetAdjacentEdgeRange(node))
    {
        const NodeID target = contractor_graph->GetTarget(e);
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
        for (auto e : contractor_graph->GetAdjacentEdgeRange(u))
        {
            const NodeID target = contractor_graph->GetTarget(e);
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
