#ifndef OSRM_EXTRACTOR_AREA_INDEX_PRIORITY_QUEUE_HPP
#define OSRM_EXTRACTOR_AREA_INDEX_PRIORITY_QUEUE_HPP

#include "util/log.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

namespace osrm::extractor::area
{

/**
 * @brief An indexed priority queue with key update operations.
 *
 * Usage: Store your data items in a vector external to this class.  Instantiate this
 * class with the size of that vector and a function that compares two items given their
 * indices.  Manipulate items in this queue by their index in the vector.
 *
 * By using an appropriate compare function you can build either a min-queue or a
 * max-queue.  To build a min-queue you must pass a function that behaves like
 * {@code std::less} and to build a max-queue you must pass a function that behaves like
 * {@code std::greater}.
 *
 * @code
 *   std::vector<double> distances = {69, 42, ...};
 *   IndexPriorityQueue min_pq(
 *       distances.size(),
 *       [&distances](size_t u, size_t v) { return distances[u] < distances[v]; }
 *   );
 *   min_pq.insert(0); // 69
 *   min_pq.insert(1); // 42
 *   assert(min_pq.pop() == 1) // 42
 *   assert(min_pq.pop() == 0) // 69
 * @endcode
 *
 * Adapted from *Sedgewick and Wayne. Algorithms, Fourth Edition. Addison Wesley. 2011.*
 * Chapter 2.4
 */
template <typename comp_t> class IndexPriorityQueue
{
  public:
    /**
     * @brief Construct a new IndexPriorityQueue object
     *
     * @param size The number of items in the external vector.
     * @param comp A function with signature {@code bool(size_t u, size_t v)} that
     *             compares the item referenced by {@code u} with the item referenced by
     *             {@code v}. To construct a priority queue that has the lowest item on
     *             top the function should behave like {@code std::less}.
     */
    IndexPriorityQueue(size_t size, comp_t comp) : vector_size{size}, comp{comp}
    {
        pq.resize(size + 1);
        qp.resize(size + 1);
        for (size_t i = 0; i <= size; ++i)
        {
            qp[i] = none;
        }
    };

    /**
     * @brief Insert the item with the index {@code i}.
     *
     * @param i The index of the item
     */
    void insert(size_t i)
    {
        assert(!contains(i));
        // util::Log(logDEBUG) << "Heap::insert(" << i << ")";
        ++n;
        qp[i] = n;
        pq[n] = i;
        swim(n);
    }

    /**
     * @brief Remove the item on top of the queue and return its index.
     *
     * @return size_t The index of the item on top.
     */
    size_t pop()
    {
        assert(!empty());
        size_t min = pq[1];
        exchange(1, n--);
        sink(1);
        qp[min] = none;
        pq[n + 1] = none;
        // util::Log(logDEBUG) << "Heap::pop() yields " << min;
        return min;
    }

    /**
     * @brief Remove the item with index {@code i}.
     *
     * @param i The index of the item to remove
     */
    void remove(size_t i)
    {
        size_t index = qp[i];
        exchange(index, n--);
        swim(index);
        sink(index);
        qp[i] = none;
    }

    /**
     * @brief Adjust the heap after the item at index {@code i} had its key decreased.
     *
     * @param i The index of the item
     */
    void decrease(size_t i)
    {
        // util::Log(logDEBUG) << "Heap::decrease(" << i << ")";
        assert(contains(i));
        swim(qp[i]);
    }

    /**
     * @brief Adjust the heap after the item at index {@code i} had its key increased.
     *
     * @param i The index of the item
     */
    void increase(size_t i) { sink(qp[i]); }

    /**
     * @brief Adjust the heap after the item at index {@code i} had its key updated.
     *
     * @param i The index of the item
     */
    void update(size_t i)
    {
        swim(qp[i]);
        sink(qp[i]);
    }

    /**
     * @brief Insert or decreas the item at index {@code i}.
     *
     * @param i The index of the item
     */
    void insert_or_decrease(size_t i)
    {
        if (contains(i))
        {
            decrease(i);
        }
        else
        {
            insert(i);
        }
    }

    /**
     * @brief Return the index of the item on top.
     */
    size_t top() { return pq[1]; }

    /**
     * @brief Return true if {@code i} is a valid index.
     */
    bool contains(size_t i) { return qp[i] != none; }

    /**
     * @brief Return true if the queue is empty.
     */

    bool empty() { return n == 0; }
    /**
     * @brief Return the number of items in the queue.
     */
    size_t size() { return n; }

  private:
    const size_t vector_size;
    comp_t comp;

    /**
     * Binary heap of the indexes into an external vector of items.
     *
     * The index is 1-based (for easier math).
     * {@code external_vector[pq[1]]} is the element on top of the queue.
     */
    std::vector<size_t> pq{};
    /** Inverse of pq: qp[pq[i]] = pq[qp[i]] = i */
    std::vector<size_t> qp{};
    /** The number of items on the heap */
    size_t n{0};

    const size_t none{~0UL};

    bool compare(size_t i, size_t j) { return comp(pq[j], pq[i]); }

    void swim(size_t i)
    {
        while (i > 1 && compare(i / 2, i))
        {
            exchange(i, i / 2);
            i = i / 2;
        }
    }
    void sink(size_t i)
    {
        while (i * 2 <= n)
        {
            size_t j = i * 2;
            if (j < n && compare(j, j + 1))
                ++j;
            if (!compare(i, j))
                break;
            exchange(i, j);
            i = j;
        }
    }
    void exchange(size_t i, size_t j)
    {
        std::swap(pq[i], pq[j]);
        qp[pq[i]] = i;
        qp[pq[j]] = j;
    }
};

} // namespace osrm::extractor::area

#endif
