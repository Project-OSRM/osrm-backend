#ifndef GRAPH_CONTRACTOR_HPP
#define GRAPH_CONTRACTOR_HPP

#include "contractor/query_edge.hpp"
#include "util/binary_heap.hpp"
#include "util/deallocating_vector.hpp"
#include "util/dynamic_graph.hpp"
#include "util/integer_range.hpp"
#include "util/percent.hpp"
#include "util/simple_logger.hpp"
#include "util/timing_util.hpp"
#include "util/typedefs.hpp"
#include "util/xor_fast_hash.hpp"
#include "util/xor_fast_hash_storage.hpp"

#include <boost/assert.hpp>

#include <stxxl/vector>

#include <tbb/enumerable_thread_specific.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <vector>

namespace osrm
{
namespace contractor
{

class GraphContractor
{
  private:
    struct ContractorEdgeData
    {
        ContractorEdgeData()
            : weight(0), id(0), originalEdges(0), shortcut(0), forward(0), backward(0),
              is_original_via_node_ID(false)
        {
        }
        ContractorEdgeData(unsigned weight,
                           unsigned original_edges,
                           unsigned id,
                           bool shortcut,
                           bool forward,
                           bool backward)
            : weight(weight), id(id), originalEdges(std::min((unsigned)1 << 28, original_edges)),
              shortcut(shortcut), forward(forward), backward(backward),
              is_original_via_node_ID(false)
        {
        }
        unsigned weight;
        unsigned id;
        unsigned originalEdges : 28;
        bool shortcut : 1;
        bool forward : 1;
        bool backward : 1;
        bool is_original_via_node_ID : 1;
    } data;

    struct ContractorHeapData
    {
        ContractorHeapData() {}
        ContractorHeapData(short hop_, bool target_) : hop(hop_), target(target_) {}

        short hop = 0;
        bool target = false;
    };

    using ContractorGraph = util::DynamicGraph<ContractorEdgeData>;
    //    using ContractorHeap = util::BinaryHeap<NodeID, NodeID, int, ContractorHeapData,
    //    ArrayStorage<NodeID, NodeID>
    //    >;
    using ContractorHeap = util::BinaryHeap<NodeID,
                                            NodeID,
                                            int,
                                            ContractorHeapData,
                                            util::XORFastHashStorage<NodeID, NodeID>>;
    using ContractorEdge = ContractorGraph::InputEdge;

    struct ContractorThreadData
    {
        ContractorHeap heap;
        std::vector<ContractorEdge> inserted_edges;
        std::vector<NodeID> neighbours;
        explicit ContractorThreadData(NodeID nodes) : heap(nodes) {}
    };

    using NodeDepth = int;

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
        RemainingNodeData() : id(0), is_independent(false) {}
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

  public:
    template <class ContainerT>
    GraphContractor(int nodes, ContainerT &input_edge_list)
        : GraphContractor(nodes, input_edge_list, {}, {})
    {
    }

    template <class ContainerT>
    GraphContractor(int nodes,
                    ContainerT &input_edge_list,
                    std::vector<float> &&node_levels_,
                    std::vector<EdgeWeight> &&node_weights_)
        : node_levels(std::move(node_levels_)), node_weights(std::move(node_weights_))
    {
        std::vector<ContractorEdge> edges;
        edges.reserve(input_edge_list.size() * 2);

        const auto dend = input_edge_list.dend();
        for (auto diter = input_edge_list.dbegin(); diter != dend; ++diter)
        {
#ifndef NDEBUG
            if (static_cast<unsigned int>(std::max(diter->weight, 1)) > 24 * 60 * 60 * 10)
            {
                util::SimpleLogger().Write(logWARNING)
                    << "Edge weight large -> "
                    << static_cast<unsigned int>(std::max(diter->weight, 1)) << " : "
                    << static_cast<unsigned int>(diter->source) << " -> "
                    << static_cast<unsigned int>(diter->target);
            }
#endif
            edges.emplace_back(diter->source,
                               diter->target,
                               static_cast<unsigned int>(std::max(diter->weight, 1)),
                               1,
                               diter->edge_id,
                               false,
                               diter->forward ? true : false,
                               diter->backward ? true : false);

            edges.emplace_back(diter->target,
                               diter->source,
                               static_cast<unsigned int>(std::max(diter->weight, 1)),
                               1,
                               diter->edge_id,
                               false,
                               diter->backward ? true : false,
                               diter->forward ? true : false);
        }
        // clear input vector
        input_edge_list.clear();
        // FIXME not sure if we need this
        edges.shrink_to_fit();

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
            // remove parallel edges
            while (i < edges.size() && edges[i].source == source && edges[i].target == target)
            {
                if (edges[i].data.forward)
                {
                    forward_edge.data.weight =
                        std::min(edges[i].data.weight, forward_edge.data.weight);
                }
                if (edges[i].data.backward)
                {
                    reverse_edge.data.weight =
                        std::min(edges[i].data.weight, reverse_edge.data.weight);
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
        util::SimpleLogger().Write() << "merged " << edges.size() - edge << " edges out of "
                                     << edges.size();
        edges.resize(edge);
        contractor_graph = std::make_shared<ContractorGraph>(nodes, edges);
        edges.clear();
        edges.shrink_to_fit();

        BOOST_ASSERT(0 == edges.capacity());
        util::SimpleLogger().Write() << "contractor finished initalization";
    }

    void Run(double core_factor = 1.0)
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
        util::Percent p(number_of_nodes);

        ThreadDataContainer thread_data_list(number_of_nodes);

        NodeID number_of_contracted_nodes = 0;
        std::vector<NodeDepth> node_depth;
        std::vector<float> node_priorities;
        is_core_node.resize(number_of_nodes, false);

        std::vector<RemainingNodeData> remaining_nodes(number_of_nodes);
        // initialize priorities in parallel
        tbb::parallel_for(tbb::blocked_range<int>(0, number_of_nodes, InitGrainSize),
                          [this, &remaining_nodes](const tbb::blocked_range<int> &range) {
                              for (int x = range.begin(), end = range.end(); x != end; ++x)
                              {
                                  remaining_nodes[x].id = x;
                              }
                          });

        bool use_cached_node_priorities = !node_levels.empty();
        if (use_cached_node_priorities)
        {
            std::cout << "using cached node priorities ..." << std::flush;
            node_priorities.swap(node_levels);
            std::cout << "ok" << std::endl;
        }
        else
        {
            node_depth.resize(number_of_nodes, 0);
            node_priorities.resize(number_of_nodes);
            node_levels.resize(number_of_nodes);

            std::cout << "initializing elimination PQ ..." << std::flush;
            tbb::parallel_for(tbb::blocked_range<int>(0, number_of_nodes, PQGrainSize),
                              [this, &node_priorities, &node_depth, &thread_data_list](
                                  const tbb::blocked_range<int> &range) {
                                  ContractorThreadData *data = thread_data_list.GetThreadData();
                                  for (int x = range.begin(), end = range.end(); x != end; ++x)
                                  {
                                      node_priorities[x] =
                                          this->EvaluateNodePriority(data, node_depth[x], x);
                                  }
                              });
            std::cout << "ok" << std::endl;
        }
        BOOST_ASSERT(node_priorities.size() == number_of_nodes);

        std::cout << "preprocessing " << number_of_nodes << " nodes ..." << std::flush;

        unsigned current_level = 0;
        bool flushed_contractor = false;
        while (number_of_nodes > 2 &&
               number_of_contracted_nodes < static_cast<NodeID>(number_of_nodes * core_factor))
        {
            if (!flushed_contractor && (number_of_contracted_nodes >
                                        static_cast<NodeID>(number_of_nodes * 0.65 * core_factor)))
            {
                util::DeallocatingVector<ContractorEdge>
                    new_edge_set; // this one is not explicitely
                                  // cleared since it goes out of
                                  // scope anywa
                std::cout << " [flush " << number_of_contracted_nodes << " nodes] " << std::flush;

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
                std::vector<NodeID> new_node_id_from_orig_id_map(number_of_nodes, SPECIAL_NODEID);

                for (const auto new_node_id :
                     util::irange<std::size_t>(0UL, remaining_nodes.size()))
                {
                    auto &node = remaining_nodes[new_node_id];
                    BOOST_ASSERT(node_priorities.size() > node.id);
                    new_node_priority[new_node_id] = node_priorities[node.id];
                    BOOST_ASSERT(node_weights.size() > node.id);
                    new_node_weights[new_node_id] = node_weights[node.id];
                }

                // build forward and backward renumbering map and remap ids in remaining_nodes
                for (const auto new_node_id :
                     util::irange<std::size_t>(0UL, remaining_nodes.size()))
                {
                    auto &node = remaining_nodes[new_node_id];
                    // create renumbering maps in both directions
                    orig_node_id_from_new_node_id_map[new_node_id] = node.id;
                    new_node_id_from_orig_id_map[node.id] = new_node_id;
                    node.id = new_node_id;
                }
                // walk over all nodes
                for (const auto source :
                     util::irange<NodeID>(0UL, contractor_graph->GetNumberOfNodes()))
                {
                    for (auto current_edge : contractor_graph->GetAdjacentEdgeRange(source))
                    {
                        ContractorGraph::EdgeData &data =
                            contractor_graph->GetEdgeData(current_edge);
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

                // Delete map from old NodeIDs to new ones.
                new_node_id_from_orig_id_map.clear();
                new_node_id_from_orig_id_map.shrink_to_fit();

                // Replace old priorities array by new one
                node_priorities.swap(new_node_priority);
                // Delete old node_priorities vector
                // Due to the scope, these should get cleared automatically? @daniel-j-h do you
                // agree?
                new_node_priority.clear();
                new_node_priority.shrink_to_fit();

                node_weights.swap(new_node_weights);
                // old Graph is removed
                contractor_graph.reset();

                // create new graph
                tbb::parallel_sort(new_edge_set.begin(), new_edge_set.end());
                contractor_graph =
                    std::make_shared<ContractorGraph>(remaining_nodes.size(), new_edge_set);

                new_edge_set.clear();
                flushed_contractor = true;

                // INFO: MAKE SURE THIS IS THE LAST OPERATION OF THE FLUSH!
                // reinitialize heaps and ThreadData objects with appropriate size
                thread_data_list.number_of_nodes = contractor_graph->GetNumberOfNodes();
            }

            tbb::parallel_for(
                tbb::blocked_range<std::size_t>(0, remaining_nodes.size(), IndependentGrainSize),
                [this, &node_priorities, &remaining_nodes, &thread_data_list](
                    const tbb::blocked_range<std::size_t> &range) {
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
            const auto begin_independent_nodes = stable_partition(
                remaining_nodes.begin(), remaining_nodes.end(), [](RemainingNodeData node_data) {
                    return !node_data.is_independent;
                });
            auto begin_independent_nodes_idx =
                std::distance(remaining_nodes.begin(), begin_independent_nodes);
            auto end_independent_nodes_idx = remaining_nodes.size();

            if (!use_cached_node_priorities)
            {
                // write out contraction level
                tbb::parallel_for(
                    tbb::blocked_range<std::size_t>(
                        begin_independent_nodes_idx, end_independent_nodes_idx, ContractGrainSize),
                    [this, remaining_nodes, flushed_contractor, current_level](
                        const tbb::blocked_range<std::size_t> &range) {
                        if (flushed_contractor)
                        {
                            for (int position = range.begin(), end = range.end(); position != end;
                                 ++position)
                            {
                                const NodeID x = remaining_nodes[position].id;
                                node_levels[orig_node_id_from_new_node_id_map[x]] = current_level;
                            }
                        }
                        else
                        {
                            for (int position = range.begin(), end = range.end(); position != end;
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
                tbb::blocked_range<std::size_t>(
                    begin_independent_nodes_idx, end_independent_nodes_idx, ContractGrainSize),
                [this, &remaining_nodes, &thread_data_list](
                    const tbb::blocked_range<std::size_t> &range) {
                    ContractorThreadData *data = thread_data_list.GetThreadData();
                    for (int position = range.begin(), end = range.end(); position != end;
                         ++position)
                    {
                        const NodeID x = remaining_nodes[position].id;
                        this->ContractNode<false>(data, x);
                    }
                });

            tbb::parallel_for(
                tbb::blocked_range<int>(
                    begin_independent_nodes_idx, end_independent_nodes_idx, DeleteGrainSize),
                [this, &remaining_nodes, &thread_data_list](const tbb::blocked_range<int> &range) {
                    ContractorThreadData *data = thread_data_list.GetThreadData();
                    for (int position = range.begin(), end = range.end(); position != end;
                         ++position)
                    {
                        const NodeID x = remaining_nodes[position].id;
                        this->DeleteIncomingEdges(data, x);
                    }
                });

            // make sure we really sort each block
            tbb::parallel_for(
                thread_data_list.data.range(),
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
                    const EdgeID current_edge_ID =
                        contractor_graph->FindEdge(edge.source, edge.target);
                    if (current_edge_ID < contractor_graph->EndEdges(edge.source))
                    {
                        ContractorGraph::EdgeData &current_data =
                            contractor_graph->GetEdgeData(current_edge_ID);
                        if (current_data.shortcut && edge.data.forward == current_data.forward &&
                            edge.data.backward == current_data.backward &&
                            edge.data.weight < current_data.weight)
                        {
                            // found a duplicate edge with smaller weight, update it.
                            current_data = edge.data;
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
                    tbb::blocked_range<int>(begin_independent_nodes_idx,
                                            end_independent_nodes_idx,
                                            NeighboursGrainSize),
                    [this, &node_priorities, &remaining_nodes, &node_depth, &thread_data_list](
                        const tbb::blocked_range<int> &range) {
                        ContractorThreadData *data = thread_data_list.GetThreadData();
                        for (int position = range.begin(), end = range.end(); position != end;
                             ++position)
                        {
                            NodeID x = remaining_nodes[position].id;
                            this->UpdateNodeNeighbours(node_priorities, node_depth, data, x);
                        }
                    });
            }

            // remove contracted nodes from the pool
            number_of_contracted_nodes += end_independent_nodes_idx - begin_independent_nodes_idx;
            remaining_nodes.resize(begin_independent_nodes_idx);

            p.PrintStatus(number_of_contracted_nodes);
            ++current_level;
        }

        if (remaining_nodes.size() > 2)
        {
            if (orig_node_id_from_new_node_id_map.size() > 0)
            {
                tbb::parallel_for(tbb::blocked_range<int>(0, remaining_nodes.size(), InitGrainSize),
                                  [this, &remaining_nodes](const tbb::blocked_range<int> &range) {
                                      for (int x = range.begin(), end = range.end(); x != end; ++x)
                                      {
                                          const auto orig_id = remaining_nodes[x].id;
                                          is_core_node[orig_node_id_from_new_node_id_map[orig_id]] =
                                              true;
                                      }
                                  });
            }
            else
            {
                tbb::parallel_for(tbb::blocked_range<int>(0, remaining_nodes.size(), InitGrainSize),
                                  [this, &remaining_nodes](const tbb::blocked_range<int> &range) {
                                      for (int x = range.begin(), end = range.end(); x != end; ++x)
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

        util::SimpleLogger().Write() << "[core] " << remaining_nodes.size() << " nodes "
                                     << contractor_graph->GetNumberOfEdges() << " edges."
                                     << std::endl;

        thread_data_list.data.clear();
    }

    inline void GetCoreMarker(std::vector<bool> &out_is_core_node)
    {
        out_is_core_node.swap(is_core_node);
    }

    inline void GetNodeLevels(std::vector<float> &out_node_levels)
    {
        out_node_levels.swap(node_levels);
    }

    template <class Edge> inline void GetEdges(util::DeallocatingVector<Edge> &edges)
    {
        util::Percent p(contractor_graph->GetNumberOfNodes());
        util::SimpleLogger().Write() << "Getting edges of minimized graph";
        const NodeID number_of_nodes = contractor_graph->GetNumberOfNodes();
        if (contractor_graph->GetNumberOfNodes())
        {
            Edge new_edge;
            for (const auto node : util::irange(0u, number_of_nodes))
            {
                p.PrintStatus(node);
                for (auto edge : contractor_graph->GetAdjacentEdgeRange(node))
                {
                    const NodeID target = contractor_graph->GetTarget(edge);
                    const ContractorGraph::EdgeData &data = contractor_graph->GetEdgeData(edge);
                    if (!orig_node_id_from_new_node_id_map.empty())
                    {
                        new_edge.source = orig_node_id_from_new_node_id_map[node];
                        new_edge.target = orig_node_id_from_new_node_id_map[target];
                    }
                    else
                    {
                        new_edge.source = node;
                        new_edge.target = target;
                    }
                    BOOST_ASSERT_MSG(SPECIAL_NODEID != new_edge.source, "Source id invalid");
                    BOOST_ASSERT_MSG(SPECIAL_NODEID != new_edge.target, "Target id invalid");
                    new_edge.data.weight = data.weight;
                    new_edge.data.shortcut = data.shortcut;
                    if (!data.is_original_via_node_ID && !orig_node_id_from_new_node_id_map.empty())
                    {
                        // tranlate the _node id_ of the shortcutted node
                        new_edge.data.id = orig_node_id_from_new_node_id_map[data.id];
                    }
                    else
                    {
                        new_edge.data.id = data.id;
                    }
                    BOOST_ASSERT_MSG(new_edge.data.id != INT_MAX, // 2^31
                                     "edge id invalid");
                    new_edge.data.forward = data.forward;
                    new_edge.data.backward = data.backward;
                    edges.push_back(new_edge);
                }
            }
        }
        contractor_graph.reset();
        orig_node_id_from_new_node_id_map.clear();
        orig_node_id_from_new_node_id_map.shrink_to_fit();

        BOOST_ASSERT(0 == orig_node_id_from_new_node_id_map.capacity());

        edges.append(external_edge_list.begin(), external_edge_list.end());
        external_edge_list.clear();
    }

  private:
    inline void RelaxNode(const NodeID node,
                          const NodeID forbidden_node,
                          const int weight,
                          ContractorHeap &heap)
    {
        const short current_hop = heap.GetData(node).hop + 1;
        for (auto edge : contractor_graph->GetAdjacentEdgeRange(node))
        {
            const ContractorEdgeData &data = contractor_graph->GetEdgeData(edge);
            if (!data.forward)
            {
                continue;
            }
            const NodeID to = contractor_graph->GetTarget(edge);
            if (forbidden_node == to)
            {
                continue;
            }
            const int to_weight = weight + data.weight;

            // New Node discovered -> Add to Heap + Node Info Storage
            if (!heap.WasInserted(to))
            {
                heap.Insert(to, to_weight, ContractorHeapData{current_hop, false});
            }
            // Found a shorter Path -> Update weight
            else if (to_weight < heap.GetKey(to))
            {
                heap.DecreaseKey(to, to_weight);
                heap.GetData(to).hop = current_hop;
            }
        }
    }

    inline void Dijkstra(const int max_weight,
                         const unsigned number_of_targets,
                         const int max_nodes,
                         ContractorThreadData &data,
                         const NodeID middle_node)
    {

        ContractorHeap &heap = data.heap;

        int nodes = 0;
        unsigned number_of_targets_found = 0;
        while (!heap.Empty())
        {
            const NodeID node = heap.DeleteMin();
            const auto weight = heap.GetKey(node);
            if (++nodes > max_nodes)
            {
                return;
            }
            if (weight > max_weight)
            {
                return;
            }

            // Destination settled?
            if (heap.GetData(node).target)
            {
                ++number_of_targets_found;
                if (number_of_targets_found >= number_of_targets)
                {
                    return;
                }
            }

            RelaxNode(node, middle_node, weight, heap);
        }
    }

    inline float EvaluateNodePriority(ContractorThreadData *const data,
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
            result = 2.f * (((float)stats.edges_added_count) / stats.edges_deleted_count) +
                     4.f * (((float)stats.original_edges_added_count) /
                            stats.original_edges_deleted_count) +
                     1.f * node_depth;
        }
        BOOST_ASSERT(result >= 0);
        return result;
    }

    template <bool RUNSIMULATION>
    inline bool
    ContractNode(ContractorThreadData *data, const NodeID node, ContractionStats *stats = nullptr)
    {
        ContractorHeap &heap = data->heap;
        std::size_t inserted_edges_size = data->inserted_edges.size();
        std::vector<ContractorEdge> &inserted_edges = data->inserted_edges;
        const constexpr bool SHORTCUT_ARC = true;
        const constexpr bool FORWARD_DIRECTION_ENABLED = true;
        const constexpr bool FORWARD_DIRECTION_DISABLED = false;
        const constexpr bool REVERSE_DIRECTION_ENABLED = true;
        const constexpr bool REVERSE_DIRECTION_DISABLED = false;

        for (auto in_edge : contractor_graph->GetAdjacentEdgeRange(node))
        {
            const ContractorEdgeData &in_data = contractor_graph->GetEdgeData(in_edge);
            const NodeID source = contractor_graph->GetTarget(in_edge);
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
            int max_weight = 0;
            unsigned number_of_targets = 0;

            for (auto out_edge : contractor_graph->GetAdjacentEdgeRange(node))
            {
                const ContractorEdgeData &out_data = contractor_graph->GetEdgeData(out_edge);
                if (!out_data.forward)
                {
                    continue;
                }
                const NodeID target = contractor_graph->GetTarget(out_edge);
                if (node == target)
                    continue;

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
                                                        out_data.originalEdges +
                                                            in_data.originalEdges,
                                                        node,
                                                        SHORTCUT_ARC,
                                                        FORWARD_DIRECTION_ENABLED,
                                                        REVERSE_DIRECTION_DISABLED);

                            inserted_edges.emplace_back(target,
                                                        source,
                                                        path_weight,
                                                        out_data.originalEdges +
                                                            in_data.originalEdges,
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
                Dijkstra(max_weight, number_of_targets, SIMULATION_SEARCH_SPACE_SIZE, *data, node);
            }
            else
            {
                const int constexpr FULL_SEARCH_SPACE_SIZE = 2000;
                Dijkstra(max_weight, number_of_targets, FULL_SEARCH_SPACE_SIZE, *data, node);
            }
            for (auto out_edge : contractor_graph->GetAdjacentEdgeRange(node))
            {
                const ContractorEdgeData &out_data = contractor_graph->GetEdgeData(out_edge);
                if (!out_data.forward)
                {
                    continue;
                }
                const NodeID target = contractor_graph->GetTarget(out_edge);
                if (target == node)
                    continue;
                const int path_weight = in_data.weight + out_data.weight;
                const int weight = heap.GetKey(target);
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
                                                    out_data.originalEdges + in_data.originalEdges,
                                                    node,
                                                    SHORTCUT_ARC,
                                                    FORWARD_DIRECTION_ENABLED,
                                                    REVERSE_DIRECTION_DISABLED);

                        inserted_edges.emplace_back(target,
                                                    source,
                                                    path_weight,
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
        return true;
    }

    inline void DeleteIncomingEdges(ContractorThreadData *data, const NodeID node)
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

    inline bool UpdateNodeNeighbours(std::vector<float> &priorities,
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

    inline bool IsNodeIndependent(const std::vector<float> &priorities,
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
    inline bool Bias(const NodeID a, const NodeID b) const
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

    std::shared_ptr<ContractorGraph> contractor_graph;
    stxxl::vector<QueryEdge> external_edge_list;
    std::vector<NodeID> orig_node_id_from_new_node_id_map;
    std::vector<float> node_levels;

    // A list of weights for every node in the graph.
    // The weight represents the cost for a u-turn on the segment in the base-graph in addition to
    // its traversal.
    // During contraction, self-loops are checked against this node weight to ensure that necessary
    // self-loops are added.
    std::vector<EdgeWeight> node_weights;
    std::vector<bool> is_core_node;
    util::XORFastHash<> fast_hash;
};
}
}

#endif // CONTRACTOR_HPP
