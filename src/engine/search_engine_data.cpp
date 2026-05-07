#include "engine/search_engine_data.hpp"

namespace osrm::engine
{

// CH heaps
using CH = routing_algorithms::ch::Algorithm;
thread_local SearchEngineData<CH>::SearchEngineHeapPtr SearchEngineData<CH>::forward_heap_1;
thread_local SearchEngineData<CH>::SearchEngineHeapPtr SearchEngineData<CH>::reverse_heap_1;
thread_local SearchEngineData<CH>::SearchEngineHeapPtr SearchEngineData<CH>::forward_heap_2;
thread_local SearchEngineData<CH>::SearchEngineHeapPtr SearchEngineData<CH>::reverse_heap_2;
thread_local SearchEngineData<CH>::SearchEngineHeapPtr SearchEngineData<CH>::forward_heap_3;
thread_local SearchEngineData<CH>::SearchEngineHeapPtr SearchEngineData<CH>::reverse_heap_3;
thread_local SearchEngineData<CH>::SearchEngineHeapPtr
    SearchEngineData<CH>::map_matching_forward_heap_1;
thread_local SearchEngineData<CH>::SearchEngineHeapPtr
    SearchEngineData<CH>::map_matching_reverse_heap_1;

thread_local SearchEngineData<CH>::ManyToManyHeapPtr SearchEngineData<CH>::many_to_many_heap;

void SearchEngineData<CH>::InitializeOrClearMapMatchingThreadLocalStorage(unsigned number_of_nodes)
{
    if (map_matching_forward_heap_1.get())
    {
        map_matching_forward_heap_1->Clear();
    }
    else
    {
        map_matching_forward_heap_1.reset(new QueryHeap(number_of_nodes));
    }

    if (map_matching_reverse_heap_1.get())
    {
        map_matching_reverse_heap_1->Clear();
    }
    else
    {
        map_matching_reverse_heap_1.reset(new QueryHeap(number_of_nodes));
    }
}

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
thread_local SearchEngineData<MLD>::SearchEngineHeapPtr SearchEngineData<MLD>::forward_heap_1;
thread_local SearchEngineData<MLD>::SearchEngineHeapPtr SearchEngineData<MLD>::reverse_heap_1;
thread_local SearchEngineData<MLD>::MapMatchingHeapPtr
    SearchEngineData<MLD>::map_matching_forward_heap_1;
thread_local SearchEngineData<MLD>::MapMatchingHeapPtr
    SearchEngineData<MLD>::map_matching_reverse_heap_1;
thread_local SearchEngineData<MLD>::ManyToManyHeapPtr SearchEngineData<MLD>::many_to_many_heap;
thread_local SearchEngineData<MLD>::UnpackingCachePtr SearchEngineData<MLD>::unpacking_cache;

void SearchEngineData<MLD>::InitializeOrClearMapMatchingThreadLocalStorage(
    unsigned number_of_nodes, unsigned number_of_boundary_nodes)
{
    if (map_matching_forward_heap_1.get())
    {
        map_matching_forward_heap_1->Clear();
    }
    else
    {
        map_matching_forward_heap_1.reset(
            new MapMatchingQueryHeap(number_of_nodes, number_of_boundary_nodes));
    }

    if (map_matching_reverse_heap_1.get())
    {
        map_matching_reverse_heap_1->Clear();
    }
    else
    {
        map_matching_reverse_heap_1.reset(
            new MapMatchingQueryHeap(number_of_nodes, number_of_boundary_nodes));
    }
}

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
void SearchEngineData<MLD>::InitializeUnpackingCache(unsigned number_of_nodes,
                                                     unsigned number_of_edges)
{
    if (unpacking_cache)
        return;

    constexpr double kCacheBudgetFraction = 0.10;
    constexpr double kL1Fraction = 0.20;
    constexpr double kL2Fraction = 0.80;
    constexpr size_t kBytesPerNode = 16;
    constexpr size_t kBytesPerEdge = 10;

    const size_t graph_memory =
        static_cast<size_t>(number_of_nodes) * kBytesPerNode +
        static_cast<size_t>(number_of_edges) * kBytesPerEdge;
    const size_t total_budget = static_cast<size_t>(graph_memory * kCacheBudgetFraction);
    const size_t l1_budget = static_cast<size_t>(total_budget * kL1Fraction);
    const size_t l2_budget = static_cast<size_t>(total_budget * kL2Fraction);

    unpacking_cache.reset(new MLDUnpackingCache(l1_budget, l2_budget, MLDUnpackingCacheCostFn{}));
}
} // namespace osrm::engine