#include "engine/search_engine_data.hpp"

#include "util/binary_heap.hpp"

namespace osrm
{
namespace engine
{

SearchEngineData::SearchEngineHeapPtr SearchEngineData::forward_heap_1;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::reverse_heap_1;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::forward_heap_2;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::reverse_heap_2;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::forward_heap_3;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::reverse_heap_3;
SearchEngineData::ManyToManyHeapPtr SearchEngineData::many_to_many_heap;

SearchEngineData::MultiLayerDijkstraHeapPtr SearchEngineData::mld_forward_heap;
SearchEngineData::MultiLayerDijkstraHeapPtr SearchEngineData::mld_reverse_heap;

void SearchEngineData::InitializeOrClearFirstThreadLocalStorage(const unsigned number_of_nodes)
{
    if (forward_heap_1.get())
    {
        forward_heap_1->Clear();
    }
    else
    {
        forward_heap_1.reset(new QueryHeap(number_of_nodes));
    }

    if (reverse_heap_1.get())
    {
        reverse_heap_1->Clear();
    }
    else
    {
        reverse_heap_1.reset(new QueryHeap(number_of_nodes));
    }
}

void SearchEngineData::InitializeOrClearSecondThreadLocalStorage(const unsigned number_of_nodes)
{
    if (forward_heap_2.get())
    {
        forward_heap_2->Clear();
    }
    else
    {
        forward_heap_2.reset(new QueryHeap(number_of_nodes));
    }

    if (reverse_heap_2.get())
    {
        reverse_heap_2->Clear();
    }
    else
    {
        reverse_heap_2.reset(new QueryHeap(number_of_nodes));
    }
}

void SearchEngineData::InitializeOrClearThirdThreadLocalStorage(const unsigned number_of_nodes)
{
    if (forward_heap_3.get())
    {
        forward_heap_3->Clear();
    }
    else
    {
        forward_heap_3.reset(new QueryHeap(number_of_nodes));
    }

    if (reverse_heap_3.get())
    {
        reverse_heap_3->Clear();
    }
    else
    {
        reverse_heap_3.reset(new QueryHeap(number_of_nodes));
    }
}

void SearchEngineData::InitializeOrClearManyToManyThreadLocalStorage(const unsigned number_of_nodes)
{
    if (many_to_many_heap.get())
    {
        many_to_many_heap->Clear();
    }
    else
    {
        many_to_many_heap.reset(new ManyToManyQueryHeap(number_of_nodes));
    }
}

void SearchEngineData::InitializeOrClearMultiLayerDijkstraThreadLocalStorage(
    const unsigned number_of_nodes)
{
    if (mld_forward_heap.get())
    {
        mld_forward_heap->Clear();
    }
    else
    {
        mld_forward_heap.reset(new MultiLayerDijkstraHeap(number_of_nodes));
    }

    if (mld_reverse_heap.get())
    {
        mld_reverse_heap->Clear();
    }
    else
    {
        mld_reverse_heap.reset(new MultiLayerDijkstraHeap(number_of_nodes));
    }
}
}
}
