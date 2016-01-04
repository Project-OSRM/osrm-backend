#include "engine/search_engine_data.hpp"

#include "util/binary_heap.hpp"

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
