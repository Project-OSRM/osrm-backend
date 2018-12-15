#ifndef SEARCH_ENGINE_DATA_HPP
#define SEARCH_ENGINE_DATA_HPP

#include "engine/algorithm.hpp"
#include "util/query_heap.hpp"
#include "util/typedefs.hpp"

#include <boost/thread/tss.hpp>

namespace osrm
{
namespace engine
{

// Algorithm-dependent heaps
// - CH algorithms use CH heaps
// - CoreCH algorithms use CH
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
    EdgeWeight duration;
    EdgeDistance distance;
    ManyToManyHeapData(NodeID p, EdgeWeight duration, EdgeDistance distance)
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

    using SearchEngineHeapPtr = boost::thread_specific_ptr<QueryHeap>;
    using ManyToManyHeapPtr = boost::thread_specific_ptr<ManyToManyQueryHeap>;

    static SearchEngineHeapPtr forward_heap_1;
    static SearchEngineHeapPtr reverse_heap_1;
    static SearchEngineHeapPtr forward_heap_2;
    static SearchEngineHeapPtr reverse_heap_2;
    static SearchEngineHeapPtr forward_heap_3;
    static SearchEngineHeapPtr reverse_heap_3;
    static ManyToManyHeapPtr many_to_many_heap;

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

struct ManyToManyMultiLayerDijkstraHeapData : MultiLayerDijkstraHeapData
{
    EdgeWeight duration;
    EdgeDistance distance;
    ManyToManyMultiLayerDijkstraHeapData(NodeID p, EdgeWeight duration, EdgeDistance distance)
        : MultiLayerDijkstraHeapData(p), duration(duration), distance(distance)
    {
    }
    ManyToManyMultiLayerDijkstraHeapData(NodeID p,
                                         bool from,
                                         EdgeWeight duration,
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

    using SearchEngineHeapPtr = boost::thread_specific_ptr<QueryHeap>;
    using ManyToManyHeapPtr = boost::thread_specific_ptr<ManyToManyQueryHeap>;

    static SearchEngineHeapPtr forward_heap_1;
    static SearchEngineHeapPtr reverse_heap_1;
    static ManyToManyHeapPtr many_to_many_heap;

    void InitializeOrClearFirstThreadLocalStorage(unsigned number_of_nodes,
                                                  unsigned number_of_boundary_nodes);

    void InitializeOrClearManyToManyThreadLocalStorage(unsigned number_of_nodes,
                                                       unsigned number_of_boundary_nodes);
};
}
}

#endif // SEARCH_ENGINE_DATA_HPP
