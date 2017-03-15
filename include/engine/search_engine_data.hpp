#ifndef SEARCH_ENGINE_DATA_HPP
#define SEARCH_ENGINE_DATA_HPP

#include <boost/thread/tss.hpp>

#include "partition/multi_level_partition.hpp"
#include "util/binary_heap.hpp"
#include "util/typedefs.hpp"

namespace osrm
{
namespace engine
{

struct HeapData
{
    NodeID parent;
    /* explicit */ HeapData(NodeID p) : parent(p) {}
};

struct ManyToManyHeapData : HeapData
{
    EdgeWeight duration;
    ManyToManyHeapData(NodeID p, EdgeWeight duration) : HeapData(p), duration(duration) {}
};

struct MultiLayerDijkstraHeapData : HeapData
{
    EdgeID edge_id; // edge id if parent -> node is a boundary edge
    MultiLayerDijkstraHeapData(NodeID p) : HeapData(p), edge_id(SPECIAL_EDGEID) {}
    MultiLayerDijkstraHeapData(NodeID p, EdgeID edge_id) : HeapData(p), edge_id(edge_id) {}
};

struct SearchEngineData
{
    using QueryHeap = util::
        BinaryHeap<NodeID, NodeID, EdgeWeight, HeapData, util::UnorderedMapStorage<NodeID, int>>;
    using SearchEngineHeapPtr = boost::thread_specific_ptr<QueryHeap>;

    using ManyToManyQueryHeap = util::BinaryHeap<NodeID,
                                                 NodeID,
                                                 EdgeWeight,
                                                 ManyToManyHeapData,
                                                 util::UnorderedMapStorage<NodeID, int>>;

    using ManyToManyHeapPtr = boost::thread_specific_ptr<ManyToManyQueryHeap>;

    using MultiLayerDijkstraHeap = util::BinaryHeap<NodeID,
                                                    NodeID,
                                                    EdgeWeight,
                                                    MultiLayerDijkstraHeapData,
                                                    util::UnorderedMapStorage<NodeID, int>>;

    using MultiLayerDijkstraHeapPtr = boost::thread_specific_ptr<MultiLayerDijkstraHeap>;

    static SearchEngineHeapPtr forward_heap_1;
    static SearchEngineHeapPtr reverse_heap_1;
    static SearchEngineHeapPtr forward_heap_2;
    static SearchEngineHeapPtr reverse_heap_2;
    static SearchEngineHeapPtr forward_heap_3;
    static SearchEngineHeapPtr reverse_heap_3;
    static ManyToManyHeapPtr many_to_many_heap;
    static MultiLayerDijkstraHeapPtr mld_forward_heap;
    static MultiLayerDijkstraHeapPtr mld_reverse_heap;

    void InitializeOrClearFirstThreadLocalStorage(const unsigned number_of_nodes);

    void InitializeOrClearSecondThreadLocalStorage(const unsigned number_of_nodes);

    void InitializeOrClearThirdThreadLocalStorage(const unsigned number_of_nodes);

    void InitializeOrClearManyToManyThreadLocalStorage(const unsigned number_of_nodes);

    void InitializeOrClearMultiLayerDijkstraThreadLocalStorage(const unsigned number_of_nodes);
};
}
}

#endif // SEARCH_ENGINE_DATA_HPP
