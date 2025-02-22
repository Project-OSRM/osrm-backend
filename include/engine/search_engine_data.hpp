#ifndef SEARCH_ENGINE_DATA_HPP
#define SEARCH_ENGINE_DATA_HPP

#include "engine/algorithm.hpp"
#include "util/query_heap.hpp"
#include "util/typedefs.hpp"

namespace osrm::engine
{

// Algorithm-dependent heaps
// - CH algorithms use CH heaps
// - MLD algorithms use MLD heaps

template <typename Algorithm> struct SearchEngineData
{
};

struct HeapData
{
    NodeID parent;
    /* explicit */ HeapData(NodeID p) : parent(p) {}
};

struct ManyToManyHeapData : HeapData
{
    EdgeDuration duration;
    EdgeDistance distance;
    ManyToManyHeapData(NodeID p, EdgeDuration duration, EdgeDistance distance)
        : HeapData(p), duration(duration), distance(distance)
    {
    }
};

template <> struct SearchEngineData<routing_algorithms::ch::Algorithm>
{
    using QueryHeap = util::
        QueryHeap<NodeID, NodeID, EdgeWeight, HeapData, util::UnorderedMapStorage<NodeID, int>>;

    using ManyToManyQueryHeap = util::QueryHeap<NodeID,
                                                NodeID,
                                                EdgeWeight,
                                                ManyToManyHeapData,
                                                util::UnorderedMapStorage<NodeID, int>>;

    using SearchEngineHeapPtr = std::unique_ptr<QueryHeap>;

    using ManyToManyHeapPtr = std::unique_ptr<ManyToManyQueryHeap>;

    static thread_local SearchEngineHeapPtr forward_heap_1;
    static thread_local SearchEngineHeapPtr reverse_heap_1;
    static thread_local SearchEngineHeapPtr forward_heap_2;
    static thread_local SearchEngineHeapPtr reverse_heap_2;
    static thread_local SearchEngineHeapPtr forward_heap_3;
    static thread_local SearchEngineHeapPtr reverse_heap_3;
    static thread_local ManyToManyHeapPtr many_to_many_heap;
    static thread_local SearchEngineHeapPtr map_matching_forward_heap_1;
    static thread_local SearchEngineHeapPtr map_matching_reverse_heap_1;

    void InitializeOrClearMapMatchingThreadLocalStorage(unsigned number_of_nodes);

    void InitializeOrClearFirstThreadLocalStorage(unsigned number_of_nodes);

    void InitializeOrClearSecondThreadLocalStorage(unsigned number_of_nodes);

    void InitializeOrClearThirdThreadLocalStorage(unsigned number_of_nodes);

    void InitializeOrClearManyToManyThreadLocalStorage(unsigned number_of_nodes);
};

struct MultiLayerDijkstraHeapData
{
    NodeID parent;
    bool from_clique_arc;
    MultiLayerDijkstraHeapData(NodeID p) : parent(p), from_clique_arc(false) {}
    MultiLayerDijkstraHeapData(NodeID p, bool from) : parent(p), from_clique_arc(from) {}
};

struct MapMatchingMultiLayerDijkstraHeapData
{
    NodeID parent;
    bool from_clique_arc;
    EdgeDistance distance = {0};
    MapMatchingMultiLayerDijkstraHeapData(NodeID p) : parent(p), from_clique_arc(false) {}
    MapMatchingMultiLayerDijkstraHeapData(NodeID p, bool from) : parent(p), from_clique_arc(from) {}
    MapMatchingMultiLayerDijkstraHeapData(NodeID p, bool from, EdgeDistance d)
        : parent(p), from_clique_arc(from), distance(d)
    {
    }
};

struct ManyToManyMultiLayerDijkstraHeapData : MultiLayerDijkstraHeapData
{
    EdgeDuration duration;
    EdgeDistance distance;
    ManyToManyMultiLayerDijkstraHeapData(NodeID p, EdgeDuration duration, EdgeDistance distance)
        : MultiLayerDijkstraHeapData(p), duration(duration), distance(distance)
    {
    }
    ManyToManyMultiLayerDijkstraHeapData(NodeID p,
                                         bool from,
                                         EdgeDuration duration,
                                         EdgeDistance distance)
        : MultiLayerDijkstraHeapData(p, from), duration(duration), distance(distance)
    {
    }
};

template <> struct SearchEngineData<routing_algorithms::mld::Algorithm>
{
    using QueryHeap = util::QueryHeap<NodeID,
                                      NodeID,
                                      EdgeWeight,
                                      MultiLayerDijkstraHeapData,
                                      util::TwoLevelStorage<NodeID, int>>;

    using ManyToManyQueryHeap = util::QueryHeap<NodeID,
                                                NodeID,
                                                EdgeWeight,
                                                ManyToManyMultiLayerDijkstraHeapData,
                                                util::TwoLevelStorage<NodeID, int>>;
    using MapMatchingQueryHeap = util::QueryHeap<NodeID,
                                                 NodeID,
                                                 EdgeWeight,
                                                 MapMatchingMultiLayerDijkstraHeapData,
                                                 util::TwoLevelStorage<NodeID, int>>;

    using SearchEngineHeapPtr = std::unique_ptr<QueryHeap>;
    using ManyToManyHeapPtr = std::unique_ptr<ManyToManyQueryHeap>;
    using MapMatchingHeapPtr = std::unique_ptr<MapMatchingQueryHeap>;

    static thread_local SearchEngineHeapPtr forward_heap_1;
    static thread_local SearchEngineHeapPtr reverse_heap_1;
    static thread_local MapMatchingHeapPtr map_matching_forward_heap_1;
    static thread_local MapMatchingHeapPtr map_matching_reverse_heap_1;

    static thread_local ManyToManyHeapPtr many_to_many_heap;

    void InitializeOrClearFirstThreadLocalStorage(unsigned number_of_nodes,
                                                  unsigned number_of_boundary_nodes);
    void InitializeOrClearMapMatchingThreadLocalStorage(unsigned number_of_nodes,
                                                        unsigned number_of_boundary_nodes);

    void InitializeOrClearManyToManyThreadLocalStorage(unsigned number_of_nodes,
                                                       unsigned number_of_boundary_nodes);
};
} // namespace osrm::engine

#endif // SEARCH_ENGINE_DATA_HPP