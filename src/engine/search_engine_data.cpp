#include "engine/search_engine_data.hpp"

#include "util/binary_heap.hpp"

namespace osrm
{
namespace engine
{

template <typename Algorithm>
typename SearchEngineData<Algorithm>::SearchEngineHeapPtr
    SearchEngineData<Algorithm>::forward_heap_1;
template <typename Algorithm>
typename SearchEngineData<Algorithm>::SearchEngineHeapPtr
    SearchEngineData<Algorithm>::reverse_heap_1;
template <typename Algorithm>
typename SearchEngineData<Algorithm>::SearchEngineHeapPtr
    SearchEngineData<Algorithm>::forward_heap_2;
template <typename Algorithm>
typename SearchEngineData<Algorithm>::SearchEngineHeapPtr
    SearchEngineData<Algorithm>::reverse_heap_2;
template <typename Algorithm>
typename SearchEngineData<Algorithm>::SearchEngineHeapPtr
    SearchEngineData<Algorithm>::forward_heap_3;
template <typename Algorithm>
typename SearchEngineData<Algorithm>::SearchEngineHeapPtr
    SearchEngineData<Algorithm>::reverse_heap_3;
template <typename Algorithm>
typename SearchEngineData<Algorithm>::ManyToManyHeapPtr
    SearchEngineData<Algorithm>::many_to_many_heap;

template <typename Algorithm>
void SearchEngineData<Algorithm>::InitializeOrClearFirstThreadLocalStorage(unsigned number_of_nodes)
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

template <typename Algorithm>
void SearchEngineData<Algorithm>::InitializeOrClearSecondThreadLocalStorage(
    unsigned number_of_nodes)
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

template <typename Algorithm>
void SearchEngineData<Algorithm>::InitializeOrClearThirdThreadLocalStorage(unsigned number_of_nodes)
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

template <typename Algorithm>
void SearchEngineData<Algorithm>::InitializeOrClearManyToManyThreadLocalStorage(
    unsigned number_of_nodes)
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

// CH
using CH = routing_algorithms::ch::Algorithm;
template SearchEngineData<CH>::SearchEngineHeapPtr SearchEngineData<CH>::forward_heap_1;
template SearchEngineData<CH>::SearchEngineHeapPtr SearchEngineData<CH>::reverse_heap_1;
template SearchEngineData<CH>::SearchEngineHeapPtr SearchEngineData<CH>::forward_heap_2;
template SearchEngineData<CH>::SearchEngineHeapPtr SearchEngineData<CH>::reverse_heap_2;
template SearchEngineData<CH>::SearchEngineHeapPtr SearchEngineData<CH>::forward_heap_3;
template SearchEngineData<CH>::SearchEngineHeapPtr SearchEngineData<CH>::reverse_heap_3;
template SearchEngineData<CH>::ManyToManyHeapPtr SearchEngineData<CH>::many_to_many_heap;

template void
SearchEngineData<routing_algorithms::ch::Algorithm>::InitializeOrClearFirstThreadLocalStorage(
    unsigned number_of_nodes);

template void
SearchEngineData<CH>::InitializeOrClearSecondThreadLocalStorage(unsigned number_of_nodes);

template void
SearchEngineData<CH>::InitializeOrClearThirdThreadLocalStorage(unsigned number_of_nodes);

template void
SearchEngineData<CH>::InitializeOrClearManyToManyThreadLocalStorage(unsigned number_of_nodes);

// CoreCH
using CoreCH = routing_algorithms::corech::Algorithm;
template SearchEngineData<CoreCH>::SearchEngineHeapPtr SearchEngineData<CoreCH>::forward_heap_1;
template SearchEngineData<CoreCH>::SearchEngineHeapPtr SearchEngineData<CoreCH>::reverse_heap_1;
template SearchEngineData<CoreCH>::SearchEngineHeapPtr SearchEngineData<CoreCH>::forward_heap_2;
template SearchEngineData<CoreCH>::SearchEngineHeapPtr SearchEngineData<CoreCH>::reverse_heap_2;
template SearchEngineData<CoreCH>::SearchEngineHeapPtr SearchEngineData<CoreCH>::forward_heap_3;
template SearchEngineData<CoreCH>::SearchEngineHeapPtr SearchEngineData<CoreCH>::reverse_heap_3;
template SearchEngineData<CoreCH>::ManyToManyHeapPtr SearchEngineData<CoreCH>::many_to_many_heap;

template void
SearchEngineData<CoreCH>::InitializeOrClearFirstThreadLocalStorage(unsigned number_of_nodes);

template void
SearchEngineData<CoreCH>::InitializeOrClearSecondThreadLocalStorage(unsigned number_of_nodes);

template void
SearchEngineData<CoreCH>::InitializeOrClearThirdThreadLocalStorage(unsigned number_of_nodes);

template void
SearchEngineData<CoreCH>::InitializeOrClearManyToManyThreadLocalStorage(unsigned number_of_nodes);

// MLD
using MLD = routing_algorithms::mld::Algorithm;
SearchEngineData<MLD>::SearchEngineHeapPtr SearchEngineData<MLD>::forward_heap_1;
SearchEngineData<MLD>::SearchEngineHeapPtr SearchEngineData<MLD>::reverse_heap_1;
SearchEngineData<MLD>::SearchEngineHeapPtr SearchEngineData<MLD>::forward_heap_2;
SearchEngineData<MLD>::SearchEngineHeapPtr SearchEngineData<MLD>::reverse_heap_2;

void SearchEngineData<MLD>::InitializeOrClearFirstThreadLocalStorage(unsigned number_of_nodes)
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

void SearchEngineData<MLD>::InitializeOrClearSecondThreadLocalStorage(unsigned)
{
    if (forward_heap_2.get())
    {
        forward_heap_2->Clear();
    }
    else
    {
        forward_heap_2.reset(new QueryHeap(1));
    }

    if (reverse_heap_2.get())
    {
        reverse_heap_2->Clear();
    }
    else
    {
        reverse_heap_2.reset(new QueryHeap(1));
    }
}
}
}
