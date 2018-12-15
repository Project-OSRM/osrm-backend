#include "contractor/graph_contractor.hpp"
#include "contractor/contractor_graph.hpp"
#include "contractor/contractor_search.hpp"
#include "contractor/query_edge.hpp"
#include "util/deallocating_vector.hpp"
#include "util/integer_range.hpp"
#include "util/log.hpp"
#include "util/percent.hpp"
#include "util/timing_util.hpp"
#include "util/typedefs.hpp"
#include "util/xor_fast_hash.hpp"

#include <boost/assert.hpp>

#include <tbb/blocked_range.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_invoke.h>
#include <tbb/parallel_sort.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <vector>

namespace osrm
{
namespace contractor
{
namespace
{
struct ContractorThreadData
{
    ContractorHeap heap;
    std::vector<ContractorEdge> inserted_edges;
    std::vector<NodeID> neighbours;
    explicit ContractorThreadData(NodeID nodes) : heap(nodes) {}
};

struct ContractorNodeData
{
    using NodeDepth = int;
    using NodePriority = float;
    using NodeLevel = float;

    ContractorNodeData(std::size_t number_of_nodes,
                       std::vector<bool> uncontracted_nodes_,
                       std::vector<bool> contractable_,
                       std::vector<EdgeWeight> weights_)
        : is_core(std::move(uncontracted_nodes_)), contractable(std::move(contractable_)),
          priorities(number_of_nodes), weights(std::move(weights_)), depths(number_of_nodes, 0)
    {
        if (contractable.empty())
        {
            contractable.resize(number_of_nodes, true);
        }
        if (is_core.empty())
        {
            is_core.resize(number_of_nodes, true);
        }
    }

    void Renumber(const std::vector<NodeID> &old_to_new)
    {
        tbb::parallel_invoke(
            [&] { util::inplacePermutation(priorities.begin(), priorities.end(), old_to_new); },
            [&] { util::inplacePermutation(weights.begin(), weights.end(), old_to_new); },
            [&] { util::inplacePermutation(is_core.begin(), is_core.end(), old_to_new); },
            [&] { util::inplacePermutation(contractable.begin(), contractable.end(), old_to_new); },
            [&] { util::inplacePermutation(depths.begin(), depths.end(), old_to_new); });
    }

    std::vector<bool> is_core;
    std::vector<bool> contractable;
    std::vector<NodePriority> priorities;
    std::vector<EdgeWeight> weights;
    std::vector<NodeDepth> depths;
};

struct ContractionStats
{
    int edges_deleted_count;
    int edges_added_count;
    int original_edges_deleted_count;
    int original_edges_added_count;
    ContractionStats()
        : edges_deleted_count(0), edges_added_count(0), original_edges_deleted_count(0),
          original_edges_added_count(0)
    {
    }
};

struct RemainingNodeData
{
    RemainingNodeData() = default;
    RemainingNodeData(NodeID id, bool is_independent) : id(id), is_independent(is_independent) {}
    NodeID id : 31;
    bool is_independent : 1;
};

struct ThreadDataContainer
{
    explicit ThreadDataContainer(int number_of_nodes) : number_of_nodes(number_of_nodes) {}

    inline ContractorThreadData *GetThreadData()
    {
        bool exists = false;
        auto &ref = data.local(exists);
        if (!exists)
        {
            ref = std::make_shared<ContractorThreadData>(number_of_nodes);
        }

        return ref.get();
    }

    int number_of_nodes;
    using EnumerableThreadData =
        tbb::enumerable_thread_specific<std::shared_ptr<ContractorThreadData>>;
    EnumerableThreadData data;
};

// This bias function takes up 22 assembly instructions in total on X86
inline bool Bias(const util::XORFastHash<> &fast_hash, const NodeID a, const NodeID b)
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

template <bool RUNSIMULATION, typename ContractorGraph>
void ContractNode(ContractorThreadData *data,
                  const ContractorGraph &graph,
                  const NodeID node,
                  std::vector<EdgeWeight> &node_weights,
                  ContractionStats *stats = nullptr)
{
    auto &heap = data->heap;
    std::size_t inserted_edges_size = data->inserted_edges.size();
    std::vector<ContractorEdge> &inserted_edges = data->inserted_edges;
    constexpr bool SHORTCUT_ARC = true;
    constexpr bool FORWARD_DIRECTION_ENABLED = true;
    constexpr bool FORWARD_DIRECTION_DISABLED = false;
    constexpr bool REVERSE_DIRECTION_ENABLED = true;
    constexpr bool REVERSE_DIRECTION_DISABLED = false;

    for (auto in_edge : graph.GetAdjacentEdgeRange(node))
    {
        const ContractorEdgeData &in_data = graph.GetEdgeData(in_edge);
        const NodeID source = graph.GetTarget(in_edge);
        if (source == node)
            continue;

        if (RUNSIMULATION)
        {
            BOOST_ASSERT(stats != nullptr);
            ++stats->edges_deleted_count;
            stats->original_edges_deleted_count += in_data.originalEdges;
        }
        if (!in_data.backward)
        {
            continue;
        }

        heap.Clear();
        heap.Insert(source, 0, ContractorHeapData{});
        EdgeWeight max_weight = 0;
        unsigned number_of_targets = 0;

        for (auto out_edge : graph.GetAdjacentEdgeRange(node))
        {
            const ContractorEdgeData &out_data = graph.GetEdgeData(out_edge);
            if (!out_data.forward)
            {
                continue;
            }
            const NodeID target = graph.GetTarget(out_edge);
            if (node == target)
            {
                continue;
            }

            const EdgeWeight path_weight = in_data.weight + out_data.weight;
            if (target == source)
            {
                if (path_weight < node_weights[node])
                {
                    if (RUNSIMULATION)
                    {
                        // make sure to prune better, but keep inserting this loop if it should
                        // still be the best
                        // CAREFUL: This only works due to the independent node-setting. This
                        // guarantees that source is not connected to another node that is
                        // contracted
                        node_weights[source] = path_weight + 1;
                        BOOST_ASSERT(stats != nullptr);
                        stats->edges_added_count += 2;
                        stats->original_edges_added_count +=
                            2 * (out_data.originalEdges + in_data.originalEdges);
                    }
                    else
                    {
                        // CAREFUL: This only works due to the independent node-setting. This
                        // guarantees that source is not connected to another node that is
                        // contracted
                        node_weights[source] = path_weight; // make sure to prune better
                        inserted_edges.emplace_back(source,
                                                    target,
                                                    path_weight,
                                                    in_data.duration + out_data.duration,
                                                    in_data.distance + out_data.distance,
                                                    out_data.originalEdges + in_data.originalEdges,
                                                    node,
                                                    SHORTCUT_ARC,
                                                    FORWARD_DIRECTION_ENABLED,
                                                    REVERSE_DIRECTION_DISABLED);

                        inserted_edges.emplace_back(target,
                                                    source,
                                                    path_weight,
                                                    in_data.duration + out_data.duration,
                                                    in_data.distance + out_data.distance,
                                                    out_data.originalEdges + in_data.originalEdges,
                                                    node,
                                                    SHORTCUT_ARC,
                                                    FORWARD_DIRECTION_DISABLED,
                                                    REVERSE_DIRECTION_ENABLED);
                    }
                }
                continue;
            }
            max_weight = std::max(max_weight, path_weight);
            if (!heap.WasInserted(target))
            {
                heap.Insert(target, INVALID_EDGE_WEIGHT, ContractorHeapData{0, true});
                ++number_of_targets;
            }
        }

        if (RUNSIMULATION)
        {
            const int constexpr SIMULATION_SEARCH_SPACE_SIZE = 1000;
            search(heap, graph, number_of_targets, SIMULATION_SEARCH_SPACE_SIZE, max_weight, node);
        }
        else
        {
            const int constexpr FULL_SEARCH_SPACE_SIZE = 2000;
            search(heap, graph, number_of_targets, FULL_SEARCH_SPACE_SIZE, max_weight, node);
        }
        for (auto out_edge : graph.GetAdjacentEdgeRange(node))
        {
            const ContractorEdgeData &out_data = graph.GetEdgeData(out_edge);
            if (!out_data.forward)
            {
                continue;
            }
            const NodeID target = graph.GetTarget(out_edge);
            if (target == node)
                continue;

            const EdgeWeight path_weight = in_data.weight + out_data.weight;
            const EdgeWeight weight = heap.GetKey(target);
            if (path_weight < weight)
            {
                if (RUNSIMULATION)
                {
                    BOOST_ASSERT(stats != nullptr);
                    stats->edges_added_count += 2;
                    stats->original_edges_added_count +=
                        2 * (out_data.originalEdges + in_data.originalEdges);
                }
                else
                {
                    inserted_edges.emplace_back(source,
                                                target,
                                                path_weight,
                                                in_data.duration + out_data.duration,
                                                in_data.distance + out_data.distance,
                                                out_data.originalEdges + in_data.originalEdges,
                                                node,
                                                SHORTCUT_ARC,
                                                FORWARD_DIRECTION_ENABLED,
                                                REVERSE_DIRECTION_DISABLED);

                    inserted_edges.emplace_back(target,
                                                source,
                                                path_weight,
                                                in_data.duration + out_data.duration,
                                                in_data.distance + out_data.distance,
                                                out_data.originalEdges + in_data.originalEdges,
                                                node,
                                                SHORTCUT_ARC,
                                                FORWARD_DIRECTION_DISABLED,
                                                REVERSE_DIRECTION_ENABLED);
                }
            }
        }
    }

    // Check For One-Way Streets to decide on the creation of self-loops
    if (!RUNSIMULATION)
    {
        std::size_t iend = inserted_edges.size();
        for (std::size_t i = inserted_edges_size; i < iend; ++i)
        {
            bool found = false;
            for (std::size_t other = i + 1; other < iend; ++other)
            {
                if (inserted_edges[other].source != inserted_edges[i].source)
                {
                    continue;
                }
                if (inserted_edges[other].target != inserted_edges[i].target)
                {
                    continue;
                }
                if (inserted_edges[other].data.weight != inserted_edges[i].data.weight)
                {
                    continue;
                }
                if (inserted_edges[other].data.shortcut != inserted_edges[i].data.shortcut)
                {
                    continue;
                }
                inserted_edges[other].data.forward |= inserted_edges[i].data.forward;
                inserted_edges[other].data.backward |= inserted_edges[i].data.backward;
                found = true;
                break;
            }
            if (!found)
            {
                inserted_edges[inserted_edges_size++] = inserted_edges[i];
            }
        }
        inserted_edges.resize(inserted_edges_size);
    }
}

void ContractNode(ContractorThreadData *data,
                  const ContractorGraph &graph,
                  const NodeID node,
                  std::vector<EdgeWeight> &node_weights)
{
    ContractNode<false>(data, graph, node, node_weights, nullptr);
}

ContractionStats SimulateNodeContraction(ContractorThreadData *data,
                                         const ContractorGraph &graph,
                                         const NodeID node,
                                         std::vector<EdgeWeight> &node_weights)
{
    ContractionStats stats;
    ContractNode<true>(data, graph, node, node_weights, &stats);
    return stats;
}

void RenumberGraph(ContractorGraph &graph, const std::vector<NodeID> &old_to_new)
{
    graph.Renumber(old_to_new);
    // Renumber all shortcut node IDs
    for (const auto node : util::irange<NodeID>(0, graph.GetNumberOfNodes()))
    {
        for (const auto edge : graph.GetAdjacentEdgeRange(node))
        {
            auto &data = graph.GetEdgeData(edge);
            if (data.shortcut)
            {
                data.id = old_to_new[data.id];
            }
        }
    }
}

/* Reorder nodes for better locality during contraction */
void RenumberData(std::vector<RemainingNodeData> &remaining_nodes,
                  std::vector<NodeID> &new_to_old_node_id,
                  ContractorNodeData &node_data,
                  ContractorGraph &graph)
{
    std::vector<NodeID> current_to_new_node_id(graph.GetNumberOfNodes(), SPECIAL_NODEID);

    // we need to make a copy here because we are going to modify it
    auto to_orig = new_to_old_node_id;

    auto new_node_id = 0u;

    // All remaining nodes get the low IDs
    for (auto &remaining : remaining_nodes)
    {
        auto id = new_node_id++;
        current_to_new_node_id[remaining.id] = id;
        new_to_old_node_id[id] = to_orig[remaining.id];
        remaining.id = id;
    }

    // Already contracted nodes get the high IDs
    for (const auto current_id : util::irange<std::size_t>(0, graph.GetNumberOfNodes()))
    {
        if (current_to_new_node_id[current_id] == SPECIAL_NODEID)
        {
            auto id = new_node_id++;
            current_to_new_node_id[current_id] = id;
            new_to_old_node_id[id] = to_orig[current_id];
        }
    }
    BOOST_ASSERT(new_node_id == graph.GetNumberOfNodes());

    node_data.Renumber(current_to_new_node_id);
    RenumberGraph(graph, current_to_new_node_id);
}

float EvaluateNodePriority(const ContractionStats &stats,
                           const ContractorNodeData::NodeDepth node_depth)
{
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

void DeleteIncomingEdges(ContractorThreadData *data, ContractorGraph &graph, const NodeID node)
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

bool UpdateNodeNeighbours(ContractorNodeData &node_data,
                          ContractorThreadData *data,
                          const ContractorGraph &graph,
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
        node_data.depths[u] = std::max(node_data.depths[node] + 1, node_data.depths[u]);
    }
    // eliminate duplicate entries ( forward + backward edges )
    std::sort(neighbours.begin(), neighbours.end());
    neighbours.resize(std::unique(neighbours.begin(), neighbours.end()) - neighbours.begin());

    // re-evaluate priorities of neighboring nodes
    for (const NodeID u : neighbours)
    {
        if (node_data.contractable[u])
        {
            node_data.priorities[u] = EvaluateNodePriority(
                SimulateNodeContraction(data, graph, u, node_data.weights), node_data.depths[u]);
        }
    }
    return true;
}

bool IsNodeIndependent(const util::XORFastHash<> &hash,
                       const std::vector<float> &priorities,
                       const std::vector<NodeID> &new_to_old_node_id,
                       const ContractorGraph &graph,
                       ContractorThreadData *const data,
                       const NodeID node)
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
            Bias(hash, new_to_old_node_id[node], new_to_old_node_id[target]))
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
                Bias(hash, new_to_old_node_id[node], new_to_old_node_id[target]))
            {
                return false;
            }
        }
    }
    return true;
}
}

std::vector<bool> contractGraph(ContractorGraph &graph,
                                std::vector<bool> node_is_uncontracted_,
                                std::vector<bool> node_is_contractable_,
                                std::vector<EdgeWeight> node_weights_,
                                double core_factor)
{
    BOOST_ASSERT(node_weights_.size() == graph.GetNumberOfNodes());
    util::XORFastHash<> fast_hash;

    // for the preperation we can use a big grain size, which is much faster (probably cache)
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
    std::vector<NodeID> new_to_old_node_id(number_of_nodes);
    // Fill the map with an identiy mapping
    std::iota(new_to_old_node_id.begin(), new_to_old_node_id.end(), 0);

    ContractorNodeData node_data{graph.GetNumberOfNodes(),
                                 std::move(node_is_uncontracted_),
                                 std::move(node_is_contractable_),
                                 std::move(node_weights_)};

    std::vector<RemainingNodeData> remaining_nodes;
    remaining_nodes.reserve(number_of_nodes);
    for (auto node : util::irange<NodeID>(0, number_of_nodes))
    {
        if (node_data.is_core[node])
        {
            if (node_data.contractable[node])
            {
                remaining_nodes.emplace_back(node, false);
            }
            else
            {
                node_data.priorities[node] =
                    std::numeric_limits<ContractorNodeData::NodePriority>::max();
            }
        }
        else
        {
            node_data.priorities[node] = 0;
        }
    }

    {
        util::UnbufferedLog log;
        log << "initializing node priorities...";
        tbb::parallel_for(tbb::blocked_range<std::size_t>(0, remaining_nodes.size(), PQGrainSize),
                          [&](const auto &range) {
                              ContractorThreadData *data = thread_data_list.GetThreadData();
                              for (auto x = range.begin(), end = range.end(); x != end; ++x)
                              {
                                  auto node = remaining_nodes[x].id;
                                  BOOST_ASSERT(node_data.contractable[node]);
                                  node_data.priorities[node] = EvaluateNodePriority(
                                      SimulateNodeContraction(data, graph, node, node_data.weights),
                                      node_data.depths[node]);
                              }
                          });
        log << " ok.";
    }

    auto number_of_core_nodes = std::max<std::size_t>(0, (1 - core_factor) * number_of_nodes);
    auto number_of_nodes_to_contract = remaining_nodes.size() - number_of_core_nodes;
    util::Log() << "preprocessing " << number_of_nodes_to_contract << " ("
                << (number_of_nodes_to_contract / (float)number_of_nodes * 100.) << "%) nodes...";

    util::UnbufferedLog log;
    util::Percent p(log, remaining_nodes.size());

    const util::XORFastHash<> hash;

    unsigned current_level = 0;
    std::size_t next_renumbering = number_of_nodes * 0.35;
    while (remaining_nodes.size() > number_of_core_nodes)
    {
        if (remaining_nodes.size() < next_renumbering)
        {
            RenumberData(remaining_nodes, new_to_old_node_id, node_data, graph);
            log << "[renumbered]";
            // only one renumbering for now
            next_renumbering = 0;
        }

        tbb::parallel_for(
            tbb::blocked_range<NodeID>(0, remaining_nodes.size(), IndependentGrainSize),
            [&](const auto &range) {
                ContractorThreadData *data = thread_data_list.GetThreadData();
                // determine independent node set
                for (auto i = range.begin(), end = range.end(); i != end; ++i)
                {
                    const NodeID node = remaining_nodes[i].id;
                    remaining_nodes[i].is_independent = IsNodeIndependent(
                        hash, node_data.priorities, new_to_old_node_id, graph, data, node);
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

        // contract independent nodes
        tbb::parallel_for(
            tbb::blocked_range<NodeID>(
                begin_independent_nodes_idx, end_independent_nodes_idx, ContractGrainSize),
            [&](const auto &range) {
                ContractorThreadData *data = thread_data_list.GetThreadData();
                for (auto position = range.begin(), end = range.end(); position != end; ++position)
                {
                    const NodeID node = remaining_nodes[position].id;
                    ContractNode(data, graph, node, node_data.weights);
                }
            });

        // core flags need to be set in serial since vector<bool> is not thread safe
        for (auto position :
             util::irange<std::size_t>(begin_independent_nodes_idx, end_independent_nodes_idx))
        {
            node_data.is_core[remaining_nodes[position].id] = false;
        }

        tbb::parallel_for(
            tbb::blocked_range<NodeID>(
                begin_independent_nodes_idx, end_independent_nodes_idx, DeleteGrainSize),
            [&](const auto &range) {
                ContractorThreadData *data = thread_data_list.GetThreadData();
                for (auto position = range.begin(), end = range.end(); position != end; ++position)
                {
                    const NodeID node = remaining_nodes[position].id;
                    DeleteIncomingEdges(data, graph, node);
                }
            });

        // make sure we really sort each block
        tbb::parallel_for(thread_data_list.data.range(), [&](const auto &range) {
            for (auto &data : range)
                tbb::parallel_sort(data->inserted_edges.begin(), data->inserted_edges.end());
        });

        // insert new edges
        for (auto &data : thread_data_list.data)
        {
            for (const ContractorEdge &edge : data->inserted_edges)
            {
                const EdgeID current_edge_ID = graph.FindEdge(edge.source, edge.target);
                if (current_edge_ID != SPECIAL_EDGEID)
                {
                    auto &current_data = graph.GetEdgeData(current_edge_ID);
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

        tbb::parallel_for(
            tbb::blocked_range<NodeID>(
                begin_independent_nodes_idx, end_independent_nodes_idx, NeighboursGrainSize),
            [&](const auto &range) {
                ContractorThreadData *data = thread_data_list.GetThreadData();
                for (auto position = range.begin(), end = range.end(); position != end; ++position)
                {
                    NodeID node = remaining_nodes[position].id;
                    UpdateNodeNeighbours(node_data, data, graph, node);
                }
            });

        // remove contracted nodes from the pool
        BOOST_ASSERT(end_independent_nodes_idx - begin_independent_nodes_idx > 0);
        number_of_contracted_nodes += end_independent_nodes_idx - begin_independent_nodes_idx;
        remaining_nodes.resize(begin_independent_nodes_idx);

        p.PrintStatus(number_of_contracted_nodes);
        ++current_level;
    }

    node_data.Renumber(new_to_old_node_id);
    RenumberGraph(graph, new_to_old_node_id);

    return std::move(node_data.is_core);
}

} // namespace contractor
} // namespace osrm
