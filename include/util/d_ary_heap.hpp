#pragma once

#include <boost/assert.hpp>
#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

namespace osrm::util
{
template <typename HeapData, int Arity, typename Comparator = std::less<HeapData>> class DAryHeap
{
  public:
    using HeapHandle = size_t;

    static constexpr HeapHandle INVALID_HANDLE = std::numeric_limits<size_t>::max();

  public:
    const HeapData &top() const { return heap[0]; }

    size_t size() const { return heap.size(); }

    bool empty() const { return heap.empty(); }

    const HeapData &operator[](HeapHandle handle) const { return heap[handle]; }

    template <typename ReorderHandler>
    void emplace(HeapData &&data, ReorderHandler &&reorderHandler)
    {
        heap.emplace_back(std::forward<HeapData>(data));
        heapifyUp(heap.size() - 1, std::forward<ReorderHandler>(reorderHandler));
    }

    template <typename ReorderHandler>
    void decrease(HeapHandle handle, HeapData &&data, ReorderHandler &&reorderHandler)
    {
        BOOST_ASSERT(handle < heap.size());

        heap[handle] = std::forward<HeapData>(data);
        heapifyUp(handle, std::forward<ReorderHandler>(reorderHandler));
    }

    void clear() { heap.clear(); }

    template <typename ReorderHandler> void pop(ReorderHandler &&reorderHandler)
    {
        BOOST_ASSERT(!heap.empty());
        heap[0] = std::move(heap.back());
        heap.pop_back();
        if (!heap.empty())
        {
            heapifyDown(0, std::forward<ReorderHandler>(reorderHandler));
        }
    }

  private:
    size_t parent(size_t index) { return (index - 1) / Arity; }

    size_t kthChild(size_t index, size_t k) { return Arity * index + k + 1; }

    template <typename ReorderHandler> void heapifyUp(size_t index, ReorderHandler &&reorderHandler)
    {
        HeapData temp = std::move(heap[index]);
        while (index > 0 && comp(temp, heap[parent(index)]))
        {
            size_t parentIndex = parent(index);
            heap[index] = std::move(heap[parentIndex]);
            reorderHandler(heap[index], index);
            index = parentIndex;
        }
        heap[index] = std::move(temp);
        reorderHandler(heap[index], index);
    }

    template <typename ReorderHandler>
    void heapifyDown(size_t index, ReorderHandler &&reorderHandler)
    {
        HeapData temp = std::move(heap[index]);
        size_t child;
        while (kthChild(index, 0) < heap.size())
        {
            child = minChild(index);
            if (!comp(heap[child], temp))
            {
                break;
            }
            heap[index] = std::move(heap[child]);
            reorderHandler(heap[index], index);
            index = child;
        }
        heap[index] = std::move(temp);
        reorderHandler(heap[index], index);
    }

    size_t minChild(size_t index)
    {
        size_t bestChild = kthChild(index, 0);
        for (size_t k = 1; k < Arity; ++k)
        {
            size_t pos = kthChild(index, k);
            if (pos < heap.size() && comp(heap[pos], heap[bestChild]))
            {
                bestChild = pos;
            }
        }
        return bestChild;
    }

  private:
    Comparator comp;
    std::vector<HeapData> heap;
};
} // namespace osrm::util
