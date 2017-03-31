#ifndef SEARCH_ENGINE_DATA_HPP
#define SEARCH_ENGINE_DATA_HPP

#include <boost/thread/tss.hpp>

#include "engine/algorithm.hpp"
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
    bool from_clique_arc;
    MultiLayerDijkstraHeapData(NodeID p) : HeapData(p), from_clique_arc(false) {}
    MultiLayerDijkstraHeapData(NodeID p, bool from) : HeapData(p), from_clique_arc(from) {}
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

private:
    static SearchEngineHeapPtr forward_heap_1;
    static SearchEngineHeapPtr reverse_heap_1;
    static MultiLayerDijkstraHeapPtr mld_forward_heap;
    static MultiLayerDijkstraHeapPtr mld_reverse_heap;

public:
    static SearchEngineHeapPtr forward_heap_2;
    static SearchEngineHeapPtr reverse_heap_2;
    static SearchEngineHeapPtr forward_heap_3;
    static SearchEngineHeapPtr reverse_heap_3;
    static ManyToManyHeapPtr many_to_many_heap;

    template <typename Algorithm>
    void InitializeOrClearFirstThreadLocalStorage(Algorithm, const unsigned number_of_nodes);

    template <typename Algorithm> auto GetForwardHeapPtr(Algorithm) const
    {
        return forward_heap_1.get();
    }

    template <typename Algorithm> auto GetReverseHeapPtr(Algorithm) const
    {
        return reverse_heap_1.get();
    }

    void InitializeOrClearFirstThreadLocalStorage(routing_algorithms::mld::Algorithm,
                                                  const unsigned number_of_nodes);

    auto GetForwardHeapPtr(routing_algorithms::mld::Algorithm) const
    {
        return mld_forward_heap.get();
    }

    auto GetReverseHeapPtr(routing_algorithms::mld::Algorithm) const
    {
        return mld_reverse_heap.get();
    }

    void InitializeOrClearSecondThreadLocalStorage(const unsigned number_of_nodes);

    void InitializeOrClearThirdThreadLocalStorage(const unsigned number_of_nodes);

    void InitializeOrClearManyToManyThreadLocalStorage(const unsigned number_of_nodes);
};
}
}

#endif // SEARCH_ENGINE_DATA_HPP
