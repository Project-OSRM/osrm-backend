#ifndef SEARCH_ENGINE_DATA_HPP
#define SEARCH_ENGINE_DATA_HPP

#include <boost/thread/tss.hpp>

#include "util/typedefs.hpp"
#include "util/binary_heap.hpp"

namespace osrm
{
namespace engine
{

struct HeapData
{
    NodeID parent;
    /* explicit */ HeapData(NodeID p) : parent(p) {}
};

struct SearchEngineData
{
    using QueryHeap = util::BinaryHeap<NodeID, NodeID, int, HeapData, util::UnorderedMapStorage<NodeID, int>>;
    using SearchEngineHeapPtr = boost::thread_specific_ptr<QueryHeap>;

    static SearchEngineHeapPtr forward_heap_1;
    static SearchEngineHeapPtr reverse_heap_1;
    static SearchEngineHeapPtr forward_heap_2;
    static SearchEngineHeapPtr reverse_heap_2;
    static SearchEngineHeapPtr forward_heap_3;
    static SearchEngineHeapPtr reverse_heap_3;

    void InitializeOrClearFirstThreadLocalStorage(const unsigned number_of_nodes);

    void InitializeOrClearSecondThreadLocalStorage(const unsigned number_of_nodes);

    void InitializeOrClearThirdThreadLocalStorage(const unsigned number_of_nodes);
};

}
}

#endif // SEARCH_ENGINE_DATA_HPP
