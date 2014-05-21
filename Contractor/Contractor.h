/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef CONTRACTOR_H
#define CONTRACTOR_H

#include "TemporaryStorage.h"
#include "../DataStructures/BinaryHeap.h"
#include "../DataStructures/DeallocatingVector.h"
#include "../DataStructures/DynamicGraph.h"
#include "../DataStructures/Percent.h"
#include "../DataStructures/XORFastHash.h"
#include "../DataStructures/XORFastHashStorage.h"
#include "../Util/OpenMPWrapper.h"
#include "../Util/SimpleLogger.h"
#include "../Util/StringUtil.h"

#include <boost/assert.hpp>

#include <algorithm>
#include <limits>
#include <vector>

class Contractor
{

  private:
    struct ContractorEdgeData
    {
        ContractorEdgeData()
            : distance(0), id(0), originalEdges(0), shortcut(0), forward(0), backward(0),
              is_original_via_node_ID(false)
        {
        }
        ContractorEdgeData(unsigned _distance,
                           unsigned _originalEdges,
                           unsigned _id,
                           bool _shortcut,
                           bool _forward,
                           bool _backward)
            : distance(_distance), id(_id),
              originalEdges(std::min((unsigned)1 << 28, _originalEdges)), shortcut(_shortcut),
              forward(_forward), backward(_backward), is_original_via_node_ID(false)
        {
        }
        unsigned distance;
        unsigned id;
        unsigned originalEdges : 28;
        bool shortcut : 1;
        bool forward : 1;
        bool backward : 1;
        bool is_original_via_node_ID : 1;
    } data;

    struct ContractorHeapData
    {
        short hop;
        bool target;
        ContractorHeapData() : hop(0), target(false) {}
        ContractorHeapData(short h, bool t) : hop(h), target(t) {}
    };

    typedef DynamicGraph<ContractorEdgeData> ContractorGraph;
    //    typedef BinaryHeap< NodeID, NodeID, int, ContractorHeapData, ArrayStorage<NodeID, NodeID>
    //    > ContractorHeap;
    typedef BinaryHeap<NodeID, NodeID, int, ContractorHeapData, XORFastHashStorage<NodeID, NodeID>>
    ContractorHeap;
    typedef ContractorGraph::InputEdge ContractorEdge;

    struct ContractorThreadData
    {
        ContractorHeap heap;
        std::vector<ContractorEdge> inserted_edges;
        std::vector<NodeID> neighbours;
        ContractorThreadData(NodeID nodes) : heap(nodes) {}
    };

    struct NodePriorityData
    {
        int depth;
        NodePriorityData() : depth(0) {}
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
        RemainingNodeData() : id(0), is_independent(false) {}
        NodeID id : 31;
        bool is_independent : 1;
    };

  public:
    template <class ContainerT> Contractor(int nodes, ContainerT &input_edge_list)
    {
        std::vector<ContractorEdge> edges;
        edges.reserve(input_edge_list.size() * 2);
        temp_edge_counter = 0;

        auto diter = input_edge_list.dbegin();
        auto dend = input_edge_list.dend();

        ContractorEdge new_edge;
        while (diter != dend)
        {
            new_edge.source = diter->source();
            new_edge.target = diter->target();
            new_edge.data = ContractorEdgeData((std::max)((int)diter->weight(), 1),
                                               1,
                                               diter->id(),
                                               false,
                                               diter->isForward(),
                                               diter->isBackward());
            BOOST_ASSERT_MSG(new_edge.data.distance > 0, "edge distance < 1");
#ifndef NDEBUG
            if (new_edge.data.distance > 24 * 60 * 60 * 10)
            {
                SimpleLogger().Write(logWARNING) << "Edge weight large -> "
                                                 << new_edge.data.distance;
            }
#endif
            edges.push_back(new_edge);
            std::swap(new_edge.source, new_edge.target);
            new_edge.data.forward = diter->isBackward();
            new_edge.data.backward = diter->isForward();
            edges.push_back(new_edge);
            ++diter;
        }
        // clear input vector and trim the current set of edges with the well-known swap trick
        input_edge_list.clear();
        sort(edges.begin(), edges.end());
        NodeID edge = 0;
        for (NodeID i = 0; i < edges.size();)
        {
            const NodeID source = edges[i].source;
            const NodeID target = edges[i].target;
            const NodeID id = edges[i].data.id;
            // remove eigenloops
            if (source == target)
            {
                i++;
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
            forward_edge.data.distance = reverse_edge.data.distance =
                std::numeric_limits<int>::max();
            // remove parallel edges
            while (i < edges.size() && edges[i].source == source && edges[i].target == target)
            {
                if (edges[i].data.forward)
                {
                    forward_edge.data.distance =
                        std::min(edges[i].data.distance, forward_edge.data.distance);
                }
                if (edges[i].data.backward)
                {
                    reverse_edge.data.distance =
                        std::min(edges[i].data.distance, reverse_edge.data.distance);
                }
                ++i;
            }
            // merge edges (s,t) and (t,s) into bidirectional edge
            if (forward_edge.data.distance == reverse_edge.data.distance)
            {
                if ((int)forward_edge.data.distance != std::numeric_limits<int>::max())
                {
                    forward_edge.data.backward = true;
                    edges[edge++] = forward_edge;
                }
            }
            else
            { // insert seperate edges
                if (((int)forward_edge.data.distance) != std::numeric_limits<int>::max())
                {
                    edges[edge++] = forward_edge;
                }
                if ((int)reverse_edge.data.distance != std::numeric_limits<int>::max())
                {
                    edges[edge++] = reverse_edge;
                }
            }
        }
        std::cout << "merged " << edges.size() - edge << " edges out of " << edges.size()
                  << std::endl;
        edges.resize(edge);
        contractor_graph = std::make_shared<ContractorGraph>(nodes, edges);
        edges.clear();
        edges.shrink_to_fit();

        BOOST_ASSERT(0 == edges.capacity());
        //        unsigned maxdegree = 0;
        //        NodeID highestNode = 0;
        //
        //        for(unsigned i = 0; i < contractor_graph->GetNumberOfNodes(); ++i) {
        //            unsigned degree = contractor_graph->EndEdges(i) -
        //            contractor_graph->BeginEdges(i);
        //            if(degree > maxdegree) {
        //                maxdegree = degree;
        //                highestNode = i;
        //            }
        //        }
        //
        //        SimpleLogger().Write() << "edges at node with id " << highestNode << " has degree
        //        " << maxdegree;
        //        for(unsigned i = contractor_graph->BeginEdges(highestNode); i <
        //        contractor_graph->EndEdges(highestNode); ++i) {
        //            SimpleLogger().Write() << " ->(" << highestNode << "," <<
        //            contractor_graph->GetTarget(i)
        //            << "); via: " << contractor_graph->GetEdgeData(i).via;
        //        }

        // Create temporary file

        edge_storage_slot = TemporaryStorage::GetInstance().AllocateSlot();
        std::cout << "contractor finished initalization" << std::endl;
    }

    ~Contractor() { TemporaryStorage::GetInstance().DeallocateSlot(edge_storage_slot); }

    void Run()
    {
        const NodeID number_of_nodes = contractor_graph->GetNumberOfNodes();
        Percent p(number_of_nodes);

        const unsigned thread_count = omp_get_max_threads();
        std::vector<ContractorThreadData *> thread_data_list;
        for (unsigned thread_id = 0; thread_id < thread_count; ++thread_id)
        {
            thread_data_list.push_back(new ContractorThreadData(number_of_nodes));
        }
        std::cout << "Contractor is using " << thread_count << " threads" << std::endl;

        NodeID number_of_contracted_nodes = 0;
        std::vector<RemainingNodeData> remaining_nodes(number_of_nodes);
        std::vector<float> node_priorities(number_of_nodes);
        std::vector<NodePriorityData> node_data(number_of_nodes);

// initialize priorities in parallel
#pragma omp parallel for schedule(guided)
        for (int x = 0; x < (int)number_of_nodes; ++x)
        {
            remaining_nodes[x].id = x;
        }

        std::cout << "initializing elimination PQ ..." << std::flush;
#pragma omp parallel
        {
            ContractorThreadData *data = thread_data_list[omp_get_thread_num()];
#pragma omp parallel for schedule(guided)
            for (int x = 0; x < (int)number_of_nodes; ++x)
            {
                node_priorities[x] = EvaluateNodePriority(data, &node_data[x], x);
            }
        }
        std::cout << "ok" << std::endl << "preprocessing " << number_of_nodes << " nodes ..."
                  << std::flush;

        bool flushed_contractor = false;
        while (number_of_nodes > 2 && number_of_contracted_nodes < number_of_nodes)
        {
            if (!flushed_contractor && (number_of_contracted_nodes > (number_of_nodes * 0.65)))
            {
                DeallocatingVector<ContractorEdge> new_edge_set; // this one is not explicitely
                                                                 // cleared since it goes out of
                                                                 // scope anywa
                std::cout << " [flush " << number_of_contracted_nodes << " nodes] " << std::flush;

                // Delete old heap data to free memory that we need for the coming operations
                for (ContractorThreadData *data : thread_data_list)
                {
                    delete data;
                }
                thread_data_list.clear();

                // Create new priority array
                std::vector<float> new_node_priority(remaining_nodes.size());
                // this map gives the old IDs from the new ones, necessary to get a consistent graph
                // at the end of contraction
                orig_node_id_to_new_id_map.resize(remaining_nodes.size());
                // this map gives the new IDs from the old ones, necessary to remap targets from the
                // remaining graph
                std::vector<NodeID> new_node_id_from_orig_id_map(number_of_nodes, UINT_MAX);

                // build forward and backward renumbering map and remap ids in remaining_nodes and
                // Priorities.
                for (unsigned new_node_id = 0; new_node_id < remaining_nodes.size(); ++new_node_id)
                {
                    // create renumbering maps in both directions
                    orig_node_id_to_new_id_map[new_node_id] = remaining_nodes[new_node_id].id;
                    new_node_id_from_orig_id_map[remaining_nodes[new_node_id].id] = new_node_id;
                    new_node_priority[new_node_id] =
                        node_priorities[remaining_nodes[new_node_id].id];
                    remaining_nodes[new_node_id].id = new_node_id;
                }
                TemporaryStorage &temporary_storage = TemporaryStorage::GetInstance();
                // walk over all nodes
                for (unsigned i = 0; i < contractor_graph->GetNumberOfNodes(); ++i)
                {
                    const NodeID start = i;
                    for (auto current_edge : contractor_graph->GetAdjacentEdgeRange(start))
                    {
                        ContractorGraph::EdgeData &data =
                            contractor_graph->GetEdgeData(current_edge);
                        const NodeID target = contractor_graph->GetTarget(current_edge);
                        if (UINT_MAX == new_node_id_from_orig_id_map[i])
                        {
                            // Save edges of this node w/o renumbering.
                            temporary_storage.WriteToSlot(
                                edge_storage_slot, (char *)&start, sizeof(NodeID));
                            temporary_storage.WriteToSlot(
                                edge_storage_slot, (char *)&target, sizeof(NodeID));
                            temporary_storage.WriteToSlot(edge_storage_slot,
                                                          (char *)&data,
                                                          sizeof(ContractorGraph::EdgeData));
                            ++temp_edge_counter;
                        }
                        else
                        {
                            // node is not yet contracted.
                            // add (renumbered) outgoing edges to new DynamicGraph.
                            ContractorEdge new_edge;
                            new_edge.source = new_node_id_from_orig_id_map[start];
                            new_edge.target = new_node_id_from_orig_id_map[target];
                            new_edge.data = data;
                            new_edge.data.is_original_via_node_ID = true;
                            BOOST_ASSERT_MSG(UINT_MAX != new_node_id_from_orig_id_map[start],
                                             "new start id not resolveable");
                            BOOST_ASSERT_MSG(UINT_MAX != new_node_id_from_orig_id_map[target],
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
                std::vector<float>().swap(new_node_priority);
                // old Graph is removed
                contractor_graph.reset();

                // create new graph
                std::sort(new_edge_set.begin(), new_edge_set.end());
                contractor_graph =
                    std::make_shared<ContractorGraph>(remaining_nodes.size(), new_edge_set);

                new_edge_set.clear();
                flushed_contractor = true;

                // INFO: MAKE SURE THIS IS THE LAST OPERATION OF THE FLUSH!
                // reinitialize heaps and ThreadData objects with appropriate size
                for (unsigned thread_id = 0; thread_id < thread_count; ++thread_id)
                {
                    thread_data_list.push_back(
                        new ContractorThreadData(contractor_graph->GetNumberOfNodes()));
                }
            }

            const int last = (int)remaining_nodes.size();
#pragma omp parallel
            {
                // determine independent node set
                ContractorThreadData *const data = thread_data_list[omp_get_thread_num()];
#pragma omp for schedule(guided)
                for (int i = 0; i < last; ++i)
                {
                    const NodeID node = remaining_nodes[i].id;
                    remaining_nodes[i].is_independent =
                        IsNodeIndependent(node_priorities, data, node);
                }
            }
            const auto first = stable_partition(remaining_nodes.begin(),
                                                remaining_nodes.end(),
                                                [](RemainingNodeData node_data)
                                                { return !node_data.is_independent; });
            const int first_independent_node = first - remaining_nodes.begin();
// contract independent nodes
#pragma omp parallel
            {
                ContractorThreadData *data = thread_data_list[omp_get_thread_num()];
#pragma omp for schedule(guided) nowait
                for (int position = first_independent_node; position < last; ++position)
                {
                    NodeID x = remaining_nodes[position].id;
                    ContractNode<false>(data, x);
                }

                std::sort(data->inserted_edges.begin(), data->inserted_edges.end());
            }
#pragma omp parallel
            {
                ContractorThreadData *data = thread_data_list[omp_get_thread_num()];
#pragma omp for schedule(guided) nowait
                for (int position = first_independent_node; position < last; ++position)
                {
                    NodeID x = remaining_nodes[position].id;
                    DeleteIncomingEdges(data, x);
                }
            }
            // insert new edges
            for (unsigned thread_id = 0; thread_id < thread_count; ++thread_id)
            {
                ContractorThreadData &data = *thread_data_list[thread_id];
                for (const ContractorEdge &edge : data.inserted_edges)
                {
                    auto current_edge_ID = contractor_graph->FindEdge(edge.source, edge.target);
                    if (current_edge_ID < contractor_graph->EndEdges(edge.source))
                    {
                        ContractorGraph::EdgeData &current_data =
                            contractor_graph->GetEdgeData(current_edge_ID);
                        if (current_data.shortcut && edge.data.forward == current_data.forward &&
                            edge.data.backward == current_data.backward &&
                            edge.data.distance < current_data.distance)
                        {
                            // found a duplicate edge with smaller weight, update it.
                            current_data = edge.data;
                            continue;
                        }
                    }
                    contractor_graph->InsertEdge(edge.source, edge.target, edge.data);
                }
                data.inserted_edges.clear();
            }
// update priorities
#pragma omp parallel
            {
                ContractorThreadData *data = thread_data_list[omp_get_thread_num()];
#pragma omp for schedule(guided) nowait
                for (int position = first_independent_node; position < last; ++position)
                {
                    NodeID x = remaining_nodes[position].id;
                    UpdateNodeNeighbours(node_priorities, node_data, data, x);
                }
            }
            // remove contracted nodes from the pool
            number_of_contracted_nodes += last - first_independent_node;
            remaining_nodes.resize(first_independent_node);
            std::vector<RemainingNodeData>(remaining_nodes).swap(remaining_nodes);
            //            unsigned maxdegree = 0;
            //            unsigned avgdegree = 0;
            //            unsigned mindegree = UINT_MAX;
            //            unsigned quaddegree = 0;
            //
            //            for(unsigned i = 0; i < remaining_nodes.size(); ++i) {
            //                unsigned degree = contractor_graph->EndEdges(remaining_nodes[i].first)
            //                -
            //                contractor_graph->BeginEdges(remaining_nodes[i].first);
            //                if(degree > maxdegree)
            //                    maxdegree = degree;
            //                if(degree < mindegree)
            //                    mindegree = degree;
            //
            //                avgdegree += degree;
            //                quaddegree += (degree*degree);
            //            }
            //
            //            avgdegree /= std::max((unsigned)1,(unsigned)remaining_nodes.size() );
            //            quaddegree /= std::max((unsigned)1,(unsigned)remaining_nodes.size() );
            //
            //            SimpleLogger().Write() << "rest: " << remaining_nodes.size() << ", max: "
            //            << maxdegree << ", min: " << mindegree << ", avg: " << avgdegree << ",
            //            quad: " << quaddegree;

            p.printStatus(number_of_contracted_nodes);
        }
        for (ContractorThreadData *data : thread_data_list)
        {
            delete data;
        }
        thread_data_list.clear();
    }

    template <class Edge> inline void GetEdges(DeallocatingVector<Edge> &edges)
    {
        Percent p(contractor_graph->GetNumberOfNodes());
        SimpleLogger().Write() << "Getting edges of minimized graph";
        NodeID number_of_nodes = contractor_graph->GetNumberOfNodes();
        if (contractor_graph->GetNumberOfNodes())
        {
            Edge new_edge;
            for (NodeID node = 0; node < number_of_nodes; ++node)
            {
                p.printStatus(node);
                for (auto edge : contractor_graph->GetAdjacentEdgeRange(node))
                {
                    const NodeID target = contractor_graph->GetTarget(edge);
                    const ContractorGraph::EdgeData &data = contractor_graph->GetEdgeData(edge);
                    if (!orig_node_id_to_new_id_map.empty())
                    {
                        new_edge.source = orig_node_id_to_new_id_map[node];
                        new_edge.target = orig_node_id_to_new_id_map[target];
                    }
                    else
                    {
                        new_edge.source = node;
                        new_edge.target = target;
                    }
                    BOOST_ASSERT_MSG(UINT_MAX != new_edge.source, "Source id invalid");
                    BOOST_ASSERT_MSG(UINT_MAX != new_edge.target, "Target id invalid");
                    new_edge.data.distance = data.distance;
                    new_edge.data.shortcut = data.shortcut;
                    if (!data.is_original_via_node_ID && !orig_node_id_to_new_id_map.empty())
                    {
                        new_edge.data.id = orig_node_id_to_new_id_map[data.id];
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
        orig_node_id_to_new_id_map.clear();
        orig_node_id_to_new_id_map.shrink_to_fit();

        BOOST_ASSERT(0 == orig_node_id_to_new_id_map.capacity());
        TemporaryStorage &temporary_storage = TemporaryStorage::GetInstance();
        // loads edges of graph before renumbering, no need for further numbering action.
        NodeID start;
        NodeID target;
        ContractorGraph::EdgeData data;

        Edge restored_edge;
        for (unsigned i = 0; i < temp_edge_counter; ++i)
        {
            temporary_storage.ReadFromSlot(edge_storage_slot, (char *)&start, sizeof(NodeID));
            temporary_storage.ReadFromSlot(edge_storage_slot, (char *)&target, sizeof(NodeID));
            temporary_storage.ReadFromSlot(
                edge_storage_slot, (char *)&data, sizeof(ContractorGraph::EdgeData));
            restored_edge.source = start;
            restored_edge.target = target;
            restored_edge.data.distance = data.distance;
            restored_edge.data.shortcut = data.shortcut;
            restored_edge.data.id = data.id;
            restored_edge.data.forward = data.forward;
            restored_edge.data.backward = data.backward;
            edges.push_back(restored_edge);
        }
        temporary_storage.DeallocateSlot(edge_storage_slot);
    }

  private:
    inline void Dijkstra(const int max_distance,
                         const unsigned number_of_targets,
                         const int maxNodes,
                         ContractorThreadData *const data,
                         const NodeID middleNode)
    {

        ContractorHeap &heap = data->heap;

        int nodes = 0;
        unsigned number_of_targets_found = 0;
        while (heap.Size() > 0)
        {
            const NodeID node = heap.DeleteMin();
            const int distance = heap.GetKey(node);
            const short current_hop = heap.GetData(node).hop + 1;

            if (++nodes > maxNodes)
            {
                return;
            }
            // Destination settled?
            if (distance > max_distance)
            {
                return;
            }

            if (heap.GetData(node).target)
            {
                ++number_of_targets_found;
                if (number_of_targets_found >= number_of_targets)
                {
                    return;
                }
            }

            // iterate over all edges of node
            for (auto edge : contractor_graph->GetAdjacentEdgeRange(node))
            {
                const ContractorEdgeData &data = contractor_graph->GetEdgeData(edge);
                if (!data.forward)
                {
                    continue;
                }
                const NodeID to = contractor_graph->GetTarget(edge);
                if (middleNode == to)
                {
                    continue;
                }
                const int to_distance = distance + data.distance;

                // New Node discovered -> Add to Heap + Node Info Storage
                if (!heap.WasInserted(to))
                {
                    heap.Insert(to, to_distance, ContractorHeapData(current_hop, false));
                }
                // Found a shorter Path -> Update distance
                else if (to_distance < heap.GetKey(to))
                {
                    heap.DecreaseKey(to, to_distance);
                    heap.GetData(to).hop = current_hop;
                }
            }
        }
    }

    inline float EvaluateNodePriority(ContractorThreadData *const data,
                                      NodePriorityData *const node_data,
                                      const NodeID node)
    {
        ContractionStats stats;

        // perform simulated contraction
        ContractNode<true>(data, node, &stats);

        // Result will contain the priority
        float result;
        if (0 == (stats.edges_deleted_count * stats.original_edges_deleted_count))
        {
            result = 1 * node_data->depth;
        }
        else
        {
            result = 2 * (((float)stats.edges_added_count) / stats.edges_deleted_count) +
                     4 * (((float)stats.original_edges_added_count) /
                          stats.original_edges_deleted_count) +
                     1 * node_data->depth;
        }
        BOOST_ASSERT(result >= 0);
        return result;
    }

    template <bool RUNSIMULATION>
    inline bool
    ContractNode(ContractorThreadData *data, NodeID node, ContractionStats *stats = NULL)
    {
        ContractorHeap &heap = data->heap;
        int inserted_edges_size = data->inserted_edges.size();
        std::vector<ContractorEdge> &inserted_edges = data->inserted_edges;

        for (auto in_edge : contractor_graph->GetAdjacentEdgeRange(node))
        {
            const ContractorEdgeData &in_data = contractor_graph->GetEdgeData(in_edge);
            const NodeID source = contractor_graph->GetTarget(in_edge);
            if (RUNSIMULATION)
            {
                BOOST_ASSERT(stats != NULL);
                ++stats->edges_deleted_count;
                stats->original_edges_deleted_count += in_data.originalEdges;
            }
            if (!in_data.backward)
            {
                continue;
            }

            heap.Clear();
            heap.Insert(source, 0, ContractorHeapData());
            int max_distance = 0;
            unsigned number_of_targets = 0;

            for (auto out_edge : contractor_graph->GetAdjacentEdgeRange(node))
            {
                const ContractorEdgeData &out_data = contractor_graph->GetEdgeData(out_edge);
                if (!out_data.forward)
                {
                    continue;
                }
                const NodeID target = contractor_graph->GetTarget(out_edge);
                const int path_distance = in_data.distance + out_data.distance;
                max_distance = std::max(max_distance, path_distance);
                if (!heap.WasInserted(target))
                {
                    heap.Insert(target, INT_MAX, ContractorHeapData(0, true));
                    ++number_of_targets;
                }
            }

            if (RUNSIMULATION)
            {
                Dijkstra(max_distance, number_of_targets, 1000, data, node);
            }
            else
            {
                Dijkstra(max_distance, number_of_targets, 2000, data, node);
            }
            for (auto out_edge : contractor_graph->GetAdjacentEdgeRange(node))
            {
                const ContractorEdgeData &out_data = contractor_graph->GetEdgeData(out_edge);
                if (!out_data.forward)
                {
                    continue;
                }
                const NodeID target = contractor_graph->GetTarget(out_edge);
                const int path_distance = in_data.distance + out_data.distance;
                const int distance = heap.GetKey(target);
                if (path_distance < distance)
                {
                    if (RUNSIMULATION)
                    {
                        BOOST_ASSERT(stats != NULL);
                        stats->edges_added_count += 2;
                        stats->original_edges_added_count +=
                            2 * (out_data.originalEdges + in_data.originalEdges);
                    }
                    else
                    {
                        ContractorEdge new_edge;
                        new_edge.source = source;
                        new_edge.target = target;
                        new_edge.data =
                            ContractorEdgeData(path_distance,
                                               out_data.originalEdges + in_data.originalEdges,
                                               node /*, 0, in_data.turnInstruction*/,
                                               true,
                                               true,
                                               false);
                        ;
                        inserted_edges.push_back(new_edge);
                        std::swap(new_edge.source, new_edge.target);
                        new_edge.data.forward = false;
                        new_edge.data.backward = true;
                        inserted_edges.push_back(new_edge);
                    }
                }
            }
        }
        if (!RUNSIMULATION)
        {
            int iend = inserted_edges.size();
            for (int i = inserted_edges_size; i < iend; ++i)
            {
                bool found = false;
                for (int other = i + 1; other < iend; ++other)
                {
                    if (inserted_edges[other].source != inserted_edges[i].source)
                    {
                        continue;
                    }
                    if (inserted_edges[other].target != inserted_edges[i].target)
                    {
                        continue;
                    }
                    if (inserted_edges[other].data.distance != inserted_edges[i].data.distance)
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

        for (int i = 0, e = (int)neighbours.size(); i < e; ++i)
        {
            contractor_graph->DeleteEdgesTo(neighbours[i], node);
        }
    }

    inline bool UpdateNodeNeighbours(std::vector<float> &priorities,
                                     std::vector<NodePriorityData> &node_data,
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
            node_data[u].depth = (std::max)(node_data[node].depth + 1, node_data[u].depth);
        }
        // eliminate duplicate entries ( forward + backward edges )
        std::sort(neighbours.begin(), neighbours.end());
        neighbours.resize(std::unique(neighbours.begin(), neighbours.end()) - neighbours.begin());

        // re-evaluate priorities of neighboring nodes
        for (const NodeID u : neighbours)
        {
            priorities[u] = EvaluateNodePriority(data, &(node_data)[u], u);
        }
        return true;
    }

    inline bool IsNodeIndependent(
        const std::vector<float> &priorities /*, const std::vector< NodePriorityData >& node_data*/,
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
                bias(node, target))
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
                assert(target_priority >= 0);
                // found a neighbour with lower priority?
                if (priority > target_priority)
                {
                    return false;
                }
                // tie breaking
                if (std::abs(priority - target_priority) < std::numeric_limits<float>::epsilon() &&
                    bias(node, target))
                {
                    return false;
                }
            }
        }
        return true;
    }

    // This bias function takes up 22 assembly instructions in total on X86
    inline bool bias(const NodeID a, const NodeID b) const
    {
        unsigned short hasha = fast_hash(a);
        unsigned short hashb = fast_hash(b);

        // The compiler optimizes that to conditional register flags but without branching
        // statements!
        if (hasha != hashb)
        {
            return hasha < hashb;
        }
        return a < b;
    }

    std::shared_ptr<ContractorGraph> contractor_graph;
    std::vector<ContractorGraph::InputEdge> contracted_edge_list;
    unsigned edge_storage_slot;
    uint64_t temp_edge_counter;
    std::vector<NodeID> orig_node_id_to_new_id_map;
    XORFastHash fast_hash;
};

#endif // CONTRACTOR_H
