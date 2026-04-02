#include "contractor/graph_contractor.hpp"
#include "contractor/contracted_edge_container.hpp"
#include "contractor/contractor_graph.hpp"
#include "contractor/contractor_heap.hpp"
#include "contractor/contractor_search.hpp"
#include "contractor/graph_contractor_adaptors.hpp"
#include "contractor/query_edge.hpp"
#include "util/integer_range.hpp"
#include "util/log.hpp"
#include "util/percent.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/concurrent_priority_queue.h>
#include <oneapi/tbb/concurrent_vector.h>
#include <oneapi/tbb/enumerable_thread_specific.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/parallel_for_each.h>
#include <oneapi/tbb/parallel_invoke.h>
#include <oneapi/tbb/parallel_sort.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <limits>
#include <vector>

#define SELF_LOOPS

/**
The algorithm here implemented is described in the papers:

[Geisberger2008]
Contraction Hierarchies: Faster and Simpler Hierarchical Routing in Road Networks
Robert Geisberger, Peter Sanders, Dominik Schultes, and Daniel Delling
https://turing.iem.thm.de/routeplanning/hwy/contract.pdf

[Vetter2009]
Parallel Time-Dependent Contraction Hierarchies
Christian Vetter - July 13, 2009
https://ae.iti.kit.edu/download/vetter_sa.pdf

tl;dr:

All nodes in the graph are ordered by their "priority". Searches only follow edges from
lower priority nodes (think: residential road) to higher priority nodes (think:
motorway).  Searches start from both source and target at the same time until they meet
somewhere in the middle.

"A node v is contracted by removing it from the network in such a way that shortest
paths in the remaining overlay graph are preserved." [Geisberger2008]  Node contraction
removes all edges going "down" into the contracted node.  This does not change any
distance from the contracted node itself to any other node since all searches starting
at the contracted node only ever go "up".  But all paths that went through the
contracted node before are now interrupted.  To fix this, contraction also inserts
"shortcuts" around the contracted node: For each pair of immediate neighbours of the
contracted node a shortcut is inserted iff the way through the node was the shortest
path between both nodes in the pair.

The nodes get contracted in order of their priority. Since there will be fewer and fewer
nodes left to be contracted the shortcuts will cover greater and greater distances.
After a node is contracted the priorities of all neighbouring nodes are updated.

Contraction in [Geisberger2008] is strictly sequential from the lowest priority node to
the highest.  To parallelize this process we introduce the concept of independent node
[Vetter2009].  A node is independent if it is far enough removed from any other
independent node.  Independent nodes can thus be contracted in parallel.

We first find all independent nodes, then contract all of them in parallel. This step is
repeated until a sufficient percentage of all nodes is contracted.

See: Algorithm 2 in Chapter 4.3 of [Vetter2009]

A note about self-loops

The shortest distance between any two nodes must be invariant under contraction.  We
must also keep invariant the shortest loop distance from any node back to itself.  This
requirement arises for us from the need to "go around" if source and target are on the
same node, with source downstream from target.  For this reason we must insert
self-loops whenever this is the shortest path from self to self.


Notes

The overlay graph G' for node v consists of v and all nodes with higher priority than v.
The core is the set of nodes not (yet) contracted.
*/

namespace osrm::contractor
{
using CLOCK = std::chrono::high_resolution_clock;

#define TIMER_DECLARE(_X)                                                                          \
    auto _X##_start = CLOCK::now();                                                                \
    auto _X##_duration = CLOCK::now() - CLOCK::now();
#define TIMER_START(_X) _X##_start = CLOCK::now()
#define TIMER_STOP(_X) _X##_duration = (CLOCK::now() - _X##_start)
#define TIMER_MSEC(_X)                                                                             \
    std::fixed << std::setprecision(2)                                                             \
               << (0.000001 *                                                                      \
                   std::chrono::duration_cast<std::chrono::nanoseconds>(_X##_duration).count())    \
               << "ms"

namespace
{

struct ContractorNodeData
{
    using NodeDepth = int;
    using NodePriority = float;
    using NodeLevel = float;

    ContractorNodeData(std::size_t number_of_nodes,
                       std::vector<bool> uncontracted_nodes_,
                       std::vector<bool> contractible_)
        : is_core(std::move(uncontracted_nodes_)), is_contractible(std::move(contractible_)),
          priorities(number_of_nodes), depths(number_of_nodes, 0)
    {
        if (is_contractible.empty())
        {
            is_contractible.resize(number_of_nodes, true);
        }
        if (is_core.empty())
        {
            is_core.resize(number_of_nodes, true);
        }
    }

    /** All these are keyed by NodeID */
    std::vector<bool> is_core;
    std::vector<bool> is_contractible;
    std::vector<NodePriority> priorities;
    std::vector<NodeDepth> depths;
};

struct ContractionStats
{
    int edges_deleted_count{};
    int edges_added_count{};
    int original_edges_deleted_count{};
    int original_edges_added_count{};
};

using ThreadData = tbb::enumerable_thread_specific<ContractorHeap>;

/**
 * @brief Get all immediate neighbours of a node. Duplicates removed.
 *
 * @param graph The graph
 * @param v The node
 * @return std::vector<NodeID> The neighbours
 */
inline std::vector<NodeID> GetNeighbours(const ContractorGraph &graph, const NodeID v)
{
    auto rg = graph.GetAdjacentEdgeRange(v);
    std::vector<NodeID> neighbours;
    neighbours.reserve(rg.size());

    for (auto e : rg)
    {
        const NodeID u = graph.GetTarget(e);
        if (u != v)
        {
            neighbours.push_back(u);
        }
    }
    std::sort(neighbours.begin(), neighbours.end());
    neighbours.erase(std::unique(neighbours.begin(), neighbours.end()), neighbours.end());
    return neighbours;
}

/**
 * @brief Contract one node or simulate its contraction
 *
 * For each pair of neighbours (u, w) if the path u->v->w was the shortest path in the
 * graph from u to w, a shortcut edge u->w is inserted.
 *
 * @tparam RUNSIMULATION
 * @param graph
 * @param node the node to contract
 * @param thread_data
 * @param node_data
 * @param inserted_edges output of actual contraction
 * @param stats output of simulation
 */
template <bool RUNSIMULATION>
void ContractNode(const ContractorGraph &graph,
                  const NodeID node,
                  ThreadData &thread_data,
                  ContractorNodeData &node_data,
                  tbb::concurrent_vector<ContractorEdge> *inserted_edges,
                  ContractionStats *stats)
{
    if (RUNSIMULATION)
    {
        BOOST_ASSERT(stats);
    }
    else
    {
        BOOST_ASSERT(inserted_edges);
    }
    const int SEARCH_SPACE_SIZE = RUNSIMULATION ? 1000 : 2000;

    ContractorHeap &heap = thread_data.local();

    // loop over all incoming edges
    auto node_edge_range = graph.GetAdjacentEdgeRange(node);
    for (auto in_edge : node_edge_range)
    {
        const NodeID source = graph.GetTarget(in_edge);
        if (source == node)
            continue;

        const ContractorEdgeData &s2n_data = graph.GetEdgeData(in_edge);
        if (RUNSIMULATION)
        {
            ++stats->edges_deleted_count;
            stats->original_edges_deleted_count += s2n_data.originalEdges;
        }

        if (!s2n_data.backward)
            continue;

        heap.Clear();
        unsigned number_of_targets = 0;
        EdgeWeight max_weight = {0};

        // insert all targets outgoing from node into query heap
        for (auto out_edge : node_edge_range)
        {
            const ContractorEdgeData &n2t_data = graph.GetEdgeData(out_edge);
            if (!n2t_data.forward)
                continue;
            const NodeID target = graph.GetTarget(out_edge);
            if (target == node)
                continue;
#ifndef SELF_LOOPS
            if (target == source)
                continue;
#endif

            if (!heap.WasInserted(target))
            {
                heap.Insert(target, INVALID_EDGE_WEIGHT, true); // true: node is a target
                max_weight = std::max(max_weight, s2n_data.weight + n2t_data.weight);
                ++number_of_targets;
            }
        }

        // Look for paths around the node that are shorter than the way through the
        // node. If we find any we don't have to insert a shortcut.  This is a
        // one-to-many search.
        search(heap,
               graph,
               source,
               node_data.is_contractible,
               number_of_targets,
               SEARCH_SPACE_SIZE,
               max_weight,
               node);

        for (auto n2t_edge : node_edge_range)
        {
            const ContractorEdgeData &n2t_data = graph.GetEdgeData(n2t_edge);
            if (!n2t_data.forward)
                continue;
            const NodeID target = graph.GetTarget(n2t_edge);
            if (target == node)
                continue;
#ifndef SELF_LOOPS
            if (target == source)
                continue;
#endif
            const EdgeWeight thru_weight = s2n_data.weight + n2t_data.weight;
            const EdgeWeight around_weight = heap.GetKey(target);

            if (thru_weight < around_weight)
            {
                // the way through the node was the shortest: since the node after
                // contraction is not reachable from source any more we must insert a
                // shortcut to preserve shortest distances
                if (RUNSIMULATION)
                {
#ifdef SELF_LOOPS
                    const int mult = (target == source) ? 1 : 2;
#else
                    const int mult = 1;
#endif
                    stats->edges_added_count += mult;
                    stats->original_edges_added_count +=
                        mult * (n2t_data.originalEdges + s2n_data.originalEdges);
                }
                else
                {
                    // For any logical edge u->v the contractor inserts a forward edge
                    // on u and a backward edge on v. The backward edge on v is logically
                    // redundant but essential when we look for neighbours of v.
                    if (target != source)
                    {
                        inserted_edges->emplace_back(source,
                                                     target,
                                                     thru_weight,
                                                     s2n_data.duration + n2t_data.duration,
                                                     s2n_data.distance + n2t_data.distance,
                                                     n2t_data.originalEdges +
                                                         s2n_data.originalEdges,
                                                     node,
                                                     true,   // shortcut
                                                     true,   // forward
                                                     false); // backward

                        inserted_edges->emplace_back(target,
                                                     source,
                                                     thru_weight,
                                                     s2n_data.duration + n2t_data.duration,
                                                     s2n_data.distance + n2t_data.distance,
                                                     n2t_data.originalEdges +
                                                         s2n_data.originalEdges,
                                                     node,
                                                     true,  // shortcut
                                                     false, // forward
                                                     true); // backward
                    }
#ifdef SELF_LOOPS
                    if (target == source)
                    {
                        inserted_edges->emplace_back(source,
                                                     target,
                                                     thru_weight,
                                                     s2n_data.duration + n2t_data.duration,
                                                     s2n_data.distance + n2t_data.distance,
                                                     n2t_data.originalEdges +
                                                         s2n_data.originalEdges,
                                                     node,
                                                     true,  // shortcut
                                                     true,  // forward
                                                     true); // backward
                    }

#endif
                }
            }
        }
    }
}

/**
 * @brief Calculate a node's priority. Lower priority nodes get contracted first.
 *
 * Note: This function is metric-agnostic to better accomodate the following
 * customization phase in which metrics will be added.
 *
 * @param stats The statistics obtained from a simulated contraction.
 * @param node_depth The node's depth.
 * @return float The priority
 */
float EvaluateNodePriority(const ContractionStats &stats,
                           const ContractorNodeData::NodeDepth node_depth)
{
    float priority;
    if (stats.edges_deleted_count == 0 || stats.original_edges_deleted_count == 0)
    {
        priority = 1.f * node_depth;
    }
    else
    {
        priority =
            2.f * (((float)stats.edges_added_count) / stats.edges_deleted_count) +
            4.f * (((float)stats.original_edges_added_count) / stats.original_edges_deleted_count) +
            1.f * node_depth;
    }
    BOOST_ASSERT(priority >= 0);
    return priority;
}

/**
 * @brief Post-process an independent node after contraction
 *
 * - Algo 2: Move I to their Level
 *
 * @param graph
 * @param v
 * @param node_data
 */
void PostProcess(ContractorGraph &graph, const NodeID v, ContractorNodeData &node_data)
{
    ContractorNodeData::NodeDepth depth = node_data.depths[v] + 1;
    for (const NodeID u : GetNeighbours(graph, v))
    {
        node_data.depths[u] = std::max(depth, node_data.depths[u]);
        // "Irrespective of the direction ﬂags, each edge (u, v) is stored only once,
        // namely at the smaller node, which complies with the requirements of both
        // forward and backward search (including the stall-on-demand technique)."
        // [Geisberger2008]

        // See also: self-loops

        graph.DeleteEdgesTo(u, v);
    }
}

/**
 * @brief Inserts the edges produced by node contraction into the graph.
 *
 * - Algo 2: Insert E into Remaining graph
 *
 * Alas edge insertion is not thread-safe even for "independent" nodes (but edge erasure
 * curiously is!). If the graph ever gets fixed to be thread-safe, this function can use
 * parallel execution too.
 *
 * @param graph
 * @param inserted_edges
 */

void InsertEdges(ContractorGraph &graph,
                 const tbb::concurrent_vector<ContractorEdge> &inserted_edges)
{
    for (const ContractorEdge &edge : inserted_edges)
    {
        const EdgeID current_edge_ID = graph.FindEdge(edge.source, edge.target);
        if (current_edge_ID != SPECIAL_EDGEID)
        {
            // found edge in graph ...
            auto &current_data = graph.GetEdgeData(current_edge_ID);
            if (edge.data.forward == current_data.forward &&
                edge.data.backward == current_data.backward)
            {
                // ... but found edge has smaller weight, update it.
                if (edge.data.weight < current_data.weight ||
                    edge.data.duration < current_data.duration ||
                    edge.data.distance < current_data.distance)
                {
                    current_data = edge.data;
                }
                // ... so don't insert a duplicate.
                continue;
            }
        }
        graph.InsertEdge(edge.source, edge.target, edge.data);
    }
}

/**
 * @brief Recalculate the priorities of all neighbouring nodes.
 *
 * @param graph
 * @param v The node id
 * @param data
 * @param node_data
 */
void UpdateNeighbourPriorities(const ContractorGraph &graph,
                               const NodeID v,
                               ContractorNodeData &node_data,
                               ThreadData &thread_data)
{
    for (const NodeID u : GetNeighbours(graph, v))
    {
        if (node_data.is_core[u] && node_data.is_contractible[u])
        {
            ContractionStats stats;
            ContractNode<true>(graph, u, thread_data, node_data, nullptr, &stats);
            node_data.priorities[u] = EvaluateNodePriority(stats, node_data.depths[u]);
        }
    }
}

/**
 * @brief Test if a node is independent.
 *
 * Two independent nodes can be contracted in parallel without influencing each other.
 *
 * A node is independent if there is no node with a lower priority less than 3 hops away
 * from it. (In case of equal priorities the node id is used as tie breaker.) The
 * next-nearest independent node must be at least 3 hops away: they can be processed at
 * the same time because all their neighbours are distinct.
 *
 * @param graph
 * @param v the node to test
 * @param priorities
 * @return bool true if the node is independent.
 */
bool IsNodeIndependent(const ContractorGraph &graph,
                       const NodeID v,
                       const std::vector<float> &priorities)
{
    const float priority = priorities[v];
    BOOST_ASSERT(priority >= 0);

    for (const NodeID hop1 : GetNeighbours(graph, v))
    {
        // 1 hop away
        const float hop1_priority = priorities[hop1];
        BOOST_ASSERT(hop1_priority >= 0);

        if (hop1_priority < priority || (hop1_priority == priority && hop1 < v))
        {
            return false;
        }

        for (auto e : graph.GetAdjacentEdgeRange(hop1))
        {
            // 2 hops away
            const NodeID hop2 = graph.GetTarget(e);
            // it is cheaper to evaluate a node twice than to do an expensive test here
            if (hop2 == v)
                continue;
            const float hop2_priority = priorities[hop2];
            BOOST_ASSERT(hop2_priority >= 0);

            if (hop2_priority < priority || (hop2_priority == priority && hop2 < v))
            {
                return false;
            }
        }
    }
    return true;
}

} // namespace

/**
 * @brief Contract the graph
 *
 * See: Algorithm 2 in Chapter 4.3 of [Vetter2009]
 *
 * @param graph
 * @param node_is_uncontracted_
 * @param node_is_contractible_
 * @param edge_weights_
 * @param core_factor
 * @return std::vector<bool>
 */
std::vector<bool> contractGraph(ContractorGraph &graph,
                                std::vector<bool> node_is_uncontracted_,
                                std::vector<bool> node_is_contractible_,
                                double core_factor)
{
    /** A heap kept in thread-local storage to avoid multiple recreations of it. */
    ContractorHeap heap_exemplar(8000);
    ThreadData thread_data(heap_exemplar);
    /** Nodes still waiting for contraction. Not all of them will be contracted though. */
    tbb::concurrent_vector<NodeID> remaining_nodes;

    std::size_t number_of_contracted_nodes = 0;

    const unsigned int number_of_nodes = graph.GetNumberOfNodes();
    ContractorNodeData node_data{
        number_of_nodes, std::move(node_is_uncontracted_), std::move(node_is_contractible_)};

    TIMER_DECLARE(init_priorities);
    TIMER_DECLARE(contract);
    TIMER_DECLARE(post_process);
    TIMER_DECLARE(insert_edges);
    TIMER_DECLARE(update_priorities);
    TIMER_DECLARE(update_core);
    TIMER_DECLARE(adjust_remaining);
    TIMER_DECLARE(renumber);

    // Update Priorities of all Nodes with Simulated Contractions
    util::Log() << "initializing node priorities...";
    TIMER_START(init_priorities);
    tbb::parallel_for(NodeID{0},
                      NodeID{number_of_nodes},
                      [&](const NodeID v)
                      {
                          if (node_data.is_core[v] && node_data.is_contractible[v])
                          {
                              remaining_nodes.emplace_back(v);
                              ContractionStats stats;
                              ContractNode<true>(graph, v, thread_data, node_data, nullptr, &stats);
                              node_data.priorities[v] =
                                  EvaluateNodePriority(stats, node_data.depths[v]);
                          }
                          else
                          {
                              node_data.priorities[v] =
                                  std::numeric_limits<ContractorNodeData::NodePriority>::max();
                          }
                      });
    TIMER_STOP(init_priorities);

    auto number_of_core_nodes = std::max<std::size_t>(0, (1 - core_factor) * number_of_nodes);
    auto number_of_nodes_to_contract = remaining_nodes.size() - number_of_core_nodes;
    util::Log() << "will contract " << number_of_nodes_to_contract << " ("
                << (number_of_nodes_to_contract / (float)number_of_nodes * 100.) << "%) nodes...";
    util::Log() << "will leave " << number_of_core_nodes << " core nodes ("
                << (number_of_core_nodes / (float)number_of_nodes * 100.) << "%) nodes...";

    util::UnbufferedLog log;
    util::Percent p(log, remaining_nodes.size());

    // Algo 2: while Remaining Graph not Empty
    //
    // contract a chunk of nodes until enough nodes are contracted
    while (remaining_nodes.size() > number_of_core_nodes)
    {
        /** List of discovered independent nodes */
        tbb::concurrent_vector<NodeID> independent_nodes;
        /** List of new edges to insert into the graph */
        tbb::concurrent_vector<ContractorEdge> inserted_edges;

        TIMER_START(contract);
        tbb::parallel_for_each(
            remaining_nodes,
            [&](NodeID &v)
            {
                // Algo 2: I ← Independent Node Set
                //
                // push the discovered independent nodes into
                // `independent_nodes` and mark them for deletion from
                // `remaining_nodes`
                if (IsNodeIndependent(graph, v, node_data.priorities))
                {
                    independent_nodes.emplace_back(v);

                    // Algo 2: E ← Necessary Shortcuts
                    //
                    // contract all independent nodes
                    // since all nodes are independent the order does not matter
                    ContractNode<false>(graph, v, thread_data, node_data, &inserted_edges, nullptr);
                    v = SPECIAL_NODEID; // mark for removal
                }
            });
        TIMER_STOP(contract);

        if (independent_nodes.size() == 0)
            // safety exit
            break;

        // adjust remaining_nodes
        TIMER_START(adjust_remaining);
        const auto new_end =
            std::remove(remaining_nodes.begin(), remaining_nodes.end(), SPECIAL_NODEID);
        remaining_nodes.resize(std::distance(remaining_nodes.begin(), new_end));
        TIMER_STOP(adjust_remaining);

        // core graph: the high(est) priority nodes
        // core flags need to be set in serial since vector<bool> is not thread safe
        TIMER_START(update_core);
        for (const NodeID v : independent_nodes)
        {
            node_data.is_core[v] = false;
        }
        TIMER_STOP(update_core);

        // We cannot incorporate this into the loop above because the graph search done
        // during `ContractNode()` above may well intrude upon other nodes' zones of
        // "independence". *The graph cannot change while searches are done.*
        TIMER_START(post_process);
        if (remaining_nodes.size() > number_of_core_nodes)
        {
            tbb::parallel_for_each(independent_nodes,
                                   [&](const NodeID v)
                                   {
                                       // Algo 2: Move I to their Level
                                       // and delete "down" edges
                                       PostProcess(graph, v, node_data);
                                   });
        }
        TIMER_STOP(post_process);

        // Algo 2: Insert E into Remaining graph
        TIMER_START(insert_edges);
        tbb::parallel_sort(inserted_edges);
        InsertEdges(graph, inserted_edges);
        TIMER_STOP(insert_edges);

        // Algo 2: Update Priority of Neighbors of I with Simulated Contractions
        // This again searches the graph, so graph updates cannot happen at the same time.
        TIMER_START(update_priorities);
        if (remaining_nodes.size() > number_of_core_nodes)
        {
            tbb::parallel_for_each(independent_nodes,
                                   [&](const NodeID v) {
                                       UpdateNeighbourPriorities(graph, v, node_data, thread_data);
                                   });
        }
        TIMER_STOP(update_priorities);

        number_of_contracted_nodes += independent_nodes.size();
        p.PrintStatus(number_of_contracted_nodes);
    }

    // no permutation happens here but the edge list is compressed
    TIMER_START(renumber);
    graph.Renumber(std::vector<NodeID>());
    TIMER_STOP(renumber);

    util::Log() << "node priorities initialized in " << TIMER_MSEC(init_priorities);
    util::Log() << "nodes contracted in " << TIMER_MSEC(contract);
    util::Log() << "nodes post-processed in " << TIMER_MSEC(post_process);
    util::Log() << "edges inserted in " << TIMER_MSEC(insert_edges);
    util::Log() << "node priorities updated in " << TIMER_MSEC(update_priorities);
    util::Log() << "core flags updated in " << TIMER_MSEC(update_core);
    util::Log() << "adjusted remaining nodes left in " << TIMER_MSEC(adjust_remaining);
    util::Log() << "graph renumbered in " << TIMER_MSEC(renumber);

    return std::move(node_data.is_core);
}

using GraphAndFilter = std::tuple<QueryGraph, std::vector<std::vector<bool>>>;

GraphAndFilter contractFullGraph(ContractorGraph contractor_graph)
{
    auto num_nodes = contractor_graph.GetNumberOfNodes();
    contractGraph(contractor_graph);

    auto edges = toEdges<QueryEdge>(std::move(contractor_graph));
    std::vector<bool> edge_filter(edges.size(), true);

    return GraphAndFilter{QueryGraph{num_nodes, edges}, {std::move(edge_filter)}};
}

GraphAndFilter contractExcludableGraph(ContractorGraph contractor_graph_,
                                       const std::vector<std::vector<bool>> &filters)
{
    if (filters.size() == 1)
    {
        if (std::all_of(filters.front().begin(), filters.front().end(), [](auto v) { return v; }))
        {
            return contractFullGraph(std::move(contractor_graph_));
        }
    }

    auto num_nodes = contractor_graph_.GetNumberOfNodes();
    ContractedEdgeContainer edge_container;
    ContractorGraph shared_core_graph;
    std::vector<bool> is_shared_core;
    {
        ContractorGraph contractor_graph = std::move(contractor_graph_);
        std::vector<bool> always_allowed(num_nodes, true);
        for (const auto &filter : filters)
        {
            for (const auto node : util::irange<NodeID>(0, num_nodes))
            {
                always_allowed[node] = always_allowed[node] && filter[node];
            }
        }

        // By not contracting all contractible nodes we avoid creating
        // a very dense core. This increases the overall graph sizes a little bit
        // but increases the final CH quality and contraction speed.
        constexpr float BASE_CORE = 0.9f;
        is_shared_core = contractGraph(contractor_graph, std::move(always_allowed), BASE_CORE);

        // Add all non-core edges to container
        {
            auto non_core_edges = toEdges<QueryEdge>(contractor_graph);
            auto new_end = std::remove_if(non_core_edges.begin(),
                                          non_core_edges.end(),
                                          [&](const auto &edge) {
                                              return is_shared_core[edge.source] &&
                                                     is_shared_core[edge.target];
                                          });
            non_core_edges.resize(new_end - non_core_edges.begin());
            edge_container.Insert(std::move(non_core_edges));

            for (const auto filter_index : util::irange<std::size_t>(0, filters.size()))
            {
                edge_container.Filter(filters[filter_index], filter_index);
            }
        }

        // Extract core graph for further contraction
        shared_core_graph = contractor_graph.Filter([&is_shared_core](const NodeID node)
                                                    { return is_shared_core[node]; });
    }

    for (const auto &filter : filters)
    {
        auto filtered_core_graph =
            shared_core_graph.Filter([&filter](const NodeID node) { return filter[node]; });

        contractGraph(filtered_core_graph, is_shared_core, is_shared_core);

        edge_container.Merge(toEdges<QueryEdge>(std::move(filtered_core_graph)));
    }

    return GraphAndFilter{QueryGraph{num_nodes, edge_container.edges},
                          edge_container.MakeEdgeFilters()};
}

} // namespace osrm::contractor
