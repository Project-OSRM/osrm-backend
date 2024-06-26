#pragma once
#include <algorithm>
#include <boost/assert.hpp>
#include <vector>

namespace osrm::util
{

// in its essence it is std::priority_queue, but with `clear` method
template <typename T> class BinaryHeap
{
  public:
    bool empty() const { return heap_.empty(); }

    const T &top() const
    {
        BOOST_ASSERT(!heap_.empty());
        return heap_.front();
    }

    void pop()
    {
        BOOST_ASSERT(!heap_.empty());
        std::pop_heap(heap_.begin(), heap_.end());
        heap_.pop_back();
    }

    template <typename... Args> void emplace(Args &&...args)
    {
        heap_.emplace_back(std::forward<Args>(args)...);
        std::push_heap(heap_.begin(), heap_.end());
    }

    void clear() { heap_.clear(); }

  private:
    std::vector<T> heap_;
};

} // namespace osrm::util