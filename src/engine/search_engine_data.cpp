#include "engine/search_engine_data.hpp"

namespace osrm
{
namespace engine
{

// CH heaps
using CH = routing_algorithms::ch::Algorithm;
SearchEngineData<CH>::SearchEngineHeapPtr SearchEngineData<CH>::forward_heap_1;
SearchEngineData<CH>::SearchEngineHeapPtr SearchEngineData<CH>::reverse_heap_1;
SearchEngineData<CH>::SearchEngineHeapPtr SearchEngineData<CH>::forward_heap_2;
SearchEngineData<CH>::SearchEngineHeapPtr SearchEngineData<CH>::reverse_heap_2;
SearchEngineData<CH>::SearchEngineHeapPtr SearchEngineData<CH>::forward_heap_3;
SearchEngineData<CH>::SearchEngineHeapPtr SearchEngineData<CH>::reverse_heap_3;
SearchEngineData<CH>::ManyToManyHeapPtr SearchEngineData<CH>::many_to_many_heap;

void SearchEngineData<CH>::InitializeOrClearFirstThreadLocalStorage(unsigned number_of_nodes)
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

void SearchEngineData<CH>::InitializeOrClearSecondThreadLocalStorage(unsigned number_of_nodes)
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

void SearchEngineData<CH>::InitializeOrClearThirdThreadLocalStorage(unsigned number_of_nodes)
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

void SearchEngineData<CH>::InitializeOrClearManyToManyThreadLocalStorage(unsigned number_of_nodes)
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

// MLD
using MLD = routing_algorithms::mld::Algorithm;
SearchEngineData<MLD>::SearchEngineHeapPtr SearchEngineData<MLD>::forward_heap_1;
SearchEngineData<MLD>::SearchEngineHeapPtr SearchEngineData<MLD>::reverse_heap_1;
SearchEngineData<MLD>::ManyToManyHeapPtr SearchEngineData<MLD>::many_to_many_heap;

void SearchEngineData<MLD>::InitializeOrClearFirstThreadLocalStorage(
    unsigned number_of_nodes, unsigned number_of_boundary_nodes)
{
    if (forward_heap_1.get())
    {
        forward_heap_1->Clear();
    }
    else
    {
        forward_heap_1.reset(new QueryHeap(number_of_nodes, number_of_boundary_nodes));
    }

    if (reverse_heap_1.get())
    {
        reverse_heap_1->Clear();
    }
    else
    {
        reverse_heap_1.reset(new QueryHeap(number_of_nodes, number_of_boundary_nodes));
    }
}

void SearchEngineData<MLD>::InitializeOrClearManyToManyThreadLocalStorage(
    unsigned number_of_nodes, unsigned number_of_boundary_nodes)
{
    if (many_to_many_heap.get())
    {
        many_to_many_heap->Clear();
    }
    else
    {
        many_to_many_heap.reset(new ManyToManyQueryHeap(number_of_nodes, number_of_boundary_nodes));
    }
}
} // namespace engine
} // namespace osrm
