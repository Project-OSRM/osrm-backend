#ifndef DEALLOCATING_VECTOR_HPP
#define DEALLOCATING_VECTOR_HPP

#include "storage/io_fwd.hpp"
#include "util/integer_range.hpp"

#include <boost/iterator/iterator_facade.hpp>

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

namespace osrm
{
namespace util
{
template <typename ElementT> struct ConstDeallocatingVectorIteratorState
{
    ConstDeallocatingVectorIteratorState()
        : index(std::numeric_limits<std::size_t>::max()), bucket_list(nullptr)
    {
    }
    explicit ConstDeallocatingVectorIteratorState(const ConstDeallocatingVectorIteratorState &r)
        : index(r.index), bucket_list(r.bucket_list)
    {
    }
    explicit ConstDeallocatingVectorIteratorState(const std::size_t idx,
                                                  const std::vector<ElementT *> *input_list)
        : index(idx), bucket_list(input_list)
    {
    }
    std::size_t index;
    const std::vector<ElementT *> *bucket_list;

    ConstDeallocatingVectorIteratorState &
    operator=(const ConstDeallocatingVectorIteratorState &other)
    {
        index = other.index;
        bucket_list = other.bucket_list;
        return *this;
    }
};

template <typename ElementT> struct DeallocatingVectorIteratorState
{
    DeallocatingVectorIteratorState()
        : index(std::numeric_limits<std::size_t>::max()), bucket_list(nullptr)
    {
    }
    explicit DeallocatingVectorIteratorState(const DeallocatingVectorIteratorState &r)
        : index(r.index), bucket_list(r.bucket_list)
    {
    }
    explicit DeallocatingVectorIteratorState(const std::size_t idx,
                                             std::vector<ElementT *> *input_list)
        : index(idx), bucket_list(input_list)
    {
    }
    std::size_t index;
    std::vector<ElementT *> *bucket_list;

    DeallocatingVectorIteratorState &operator=(const DeallocatingVectorIteratorState &other)
    {
        index = other.index;
        bucket_list = other.bucket_list;
        return *this;
    }
};

template <typename ElementT, std::size_t ELEMENTS_PER_BLOCK>
class ConstDeallocatingVectorIterator
    : public boost::iterator_facade<ConstDeallocatingVectorIterator<ElementT, ELEMENTS_PER_BLOCK>,
                                    ElementT,
                                    std::random_access_iterator_tag>
{
    ConstDeallocatingVectorIteratorState<ElementT> current_state;

  public:
    ConstDeallocatingVectorIterator() {}
    ConstDeallocatingVectorIterator(std::size_t idx, const std::vector<ElementT *> *input_list)
        : current_state(idx, input_list)
    {
    }

    friend class boost::iterator_core_access;

    void advance(std::size_t n) { current_state.index += n; }

    void increment() { advance(1); }

    void decrement() { advance(-1); }

    bool equal(ConstDeallocatingVectorIterator const &other) const
    {
        return current_state.index == other.current_state.index;
    }

    std::ptrdiff_t distance_to(ConstDeallocatingVectorIterator const &other) const
    {
        // it is important to implement it 'other minus this'. otherwise sorting breaks
        return other.current_state.index - current_state.index;
    }

    ElementT &dereference() const
    {
        const std::size_t current_bucket = current_state.index / ELEMENTS_PER_BLOCK;
        const std::size_t current_index = current_state.index % ELEMENTS_PER_BLOCK;
        return (current_state.bucket_list->at(current_bucket)[current_index]);
    }

    ElementT &operator[](const std::size_t index) const
    {
        const std::size_t current_bucket = (index + current_state.index) / ELEMENTS_PER_BLOCK;
        const std::size_t current_index = (index + current_state.index) % ELEMENTS_PER_BLOCK;
        return (current_state.bucket_list->at(current_bucket)[current_index]);
    }
};

template <typename ElementT, std::size_t ELEMENTS_PER_BLOCK>
class DeallocatingVectorIterator
    : public boost::iterator_facade<DeallocatingVectorIterator<ElementT, ELEMENTS_PER_BLOCK>,
                                    ElementT,
                                    std::random_access_iterator_tag>
{
    DeallocatingVectorIteratorState<ElementT> current_state;

  public:
    DeallocatingVectorIterator() {}
    DeallocatingVectorIterator(std::size_t idx, std::vector<ElementT *> *input_list)
        : current_state(idx, input_list)
    {
    }

    friend class boost::iterator_core_access;

    void advance(std::size_t n) { current_state.index += n; }

    void increment() { advance(1); }

    void decrement() { advance(-1); }

    bool equal(DeallocatingVectorIterator const &other) const
    {
        return current_state.index == other.current_state.index;
    }

    std::ptrdiff_t distance_to(DeallocatingVectorIterator const &other) const
    {
        // it is important to implement it 'other minus this'. otherwise sorting breaks
        return other.current_state.index - current_state.index;
    }

    ElementT &dereference() const
    {
        const std::size_t current_bucket = current_state.index / ELEMENTS_PER_BLOCK;
        const std::size_t current_index = current_state.index % ELEMENTS_PER_BLOCK;
        return (current_state.bucket_list->at(current_bucket)[current_index]);
    }

    ElementT &operator[](const std::size_t index) const
    {
        const std::size_t current_bucket = (index + current_state.index) / ELEMENTS_PER_BLOCK;
        const std::size_t current_index = (index + current_state.index) % ELEMENTS_PER_BLOCK;
        return (current_state.bucket_list->at(current_bucket)[current_index]);
    }
};

template <typename ElementT> class DeallocatingVector;

template <typename T> void swap(DeallocatingVector<T> &lhs, DeallocatingVector<T> &rhs);

template <typename ElementT> class DeallocatingVector
{
    static constexpr std::size_t ELEMENTS_PER_BLOCK = 8388608 / sizeof(ElementT);
    std::size_t current_size;
    std::vector<ElementT *> bucket_list;

  public:
    using value_type = ElementT;
    using iterator = DeallocatingVectorIterator<ElementT, ELEMENTS_PER_BLOCK>;
    using const_iterator = ConstDeallocatingVectorIterator<ElementT, ELEMENTS_PER_BLOCK>;

    DeallocatingVector() : current_size(0)
    {
        bucket_list.emplace_back(new ElementT[ELEMENTS_PER_BLOCK]);
    }

    // Performs a deep copy of the buckets
    DeallocatingVector(const DeallocatingVector &other)
    {
        bucket_list.resize(other.bucket_list.size());
        for (const auto index : util::irange<std::size_t>(0, bucket_list.size()))
        {
            bucket_list[index] = new ElementT[ELEMENTS_PER_BLOCK];
            std::copy_n(other.bucket_list[index], ELEMENTS_PER_BLOCK, bucket_list[index]);
        }
        current_size = other.current_size;
    }
    // Note we capture other by value
    DeallocatingVector &operator=(const DeallocatingVector &other)
    {
        auto copy_other = other;
        swap(copy_other);
        return *this;
    }

    // moving is fine
    DeallocatingVector(DeallocatingVector &&other) { swap(other); }
    DeallocatingVector &operator=(DeallocatingVector &&other)
    {
        swap(other);
        return *this;
    }

    DeallocatingVector(std::initializer_list<ElementT> elements) : DeallocatingVector()
    {
        for (auto &&elem : elements)
        {
            emplace_back(std::move(elem));
        }
    }

    ~DeallocatingVector() { clear(); }

    friend void swap<>(DeallocatingVector<ElementT> &lhs, DeallocatingVector<ElementT> &rhs);

    void swap(DeallocatingVector<ElementT> &other)
    {
        std::swap(current_size, other.current_size);
        bucket_list.swap(other.bucket_list);
    }

    void clear()
    {
        // Delete[]'ing ptr's to all Buckets
        for (auto bucket : bucket_list)
        {
            if (nullptr != bucket)
            {
                delete[] bucket;
                bucket = nullptr;
            }
        }
        bucket_list.clear();
        bucket_list.shrink_to_fit();
        current_size = 0;
    }

    void push_back(const ElementT &element)
    {
        const std::size_t current_capacity = capacity();
        if (current_size == current_capacity)
        {
            bucket_list.push_back(new ElementT[ELEMENTS_PER_BLOCK]);
        }

        std::size_t current_index = size() % ELEMENTS_PER_BLOCK;
        bucket_list.back()[current_index] = element;
        ++current_size;
    }

    template <typename... Ts> void emplace_back(Ts &&... element)
    {
        const std::size_t current_capacity = capacity();
        if (current_size == current_capacity)
        {
            bucket_list.push_back(new ElementT[ELEMENTS_PER_BLOCK]);
        }

        const std::size_t current_index = size() % ELEMENTS_PER_BLOCK;
        bucket_list.back()[current_index] = ElementT(std::forward<Ts>(element)...);
        ++current_size;
    }

    void reserve(const std::size_t) const
    { /* don't do anything */
    }

    void resize(const std::size_t new_size)
    {
        if (new_size >= current_size)
        {
            while (capacity() < new_size)
            {
                bucket_list.push_back(new ElementT[ELEMENTS_PER_BLOCK]);
            }
        }
        else
        { // down-size
            const std::size_t number_of_necessary_buckets = 1 + (new_size / ELEMENTS_PER_BLOCK);
            for (const auto bucket_index : irange(number_of_necessary_buckets, bucket_list.size()))
            {
                if (nullptr != bucket_list[bucket_index])
                {
                    delete[] bucket_list[bucket_index];
                }
            }
            bucket_list.resize(number_of_necessary_buckets);
        }
        current_size = new_size;
    }

    std::size_t size() const { return current_size; }

    std::size_t capacity() const { return bucket_list.size() * ELEMENTS_PER_BLOCK; }

    iterator begin() { return iterator(static_cast<std::size_t>(0), &bucket_list); }

    iterator end() { return iterator(size(), &bucket_list); }

    const_iterator begin() const
    {
        return const_iterator(static_cast<std::size_t>(0), &bucket_list);
    }

    const_iterator end() const { return const_iterator(size(), &bucket_list); }

    ElementT &operator[](const std::size_t index)
    {
        const std::size_t _bucket = index / ELEMENTS_PER_BLOCK;
        const std::size_t _index = index % ELEMENTS_PER_BLOCK;
        return (bucket_list[_bucket][_index]);
    }

    ElementT &operator[](const std::size_t index) const
    {
        const std::size_t _bucket = index / ELEMENTS_PER_BLOCK;
        const std::size_t _index = index % ELEMENTS_PER_BLOCK;
        return (bucket_list[_bucket][_index]);
    }

    ElementT &back() const
    {
        const std::size_t _bucket = (current_size - 1) / ELEMENTS_PER_BLOCK;
        const std::size_t _index = (current_size - 1) % ELEMENTS_PER_BLOCK;
        return (bucket_list[_bucket][_index]);
    }

    template <class InputIterator> void append(InputIterator first, const InputIterator last)
    {
        InputIterator position = first;
        while (position != last)
        {
            push_back(*position);
            ++position;
        }
    }
};

template <typename T> void swap(DeallocatingVector<T> &lhs, DeallocatingVector<T> &rhs)
{
    lhs.swap(rhs);
}
} // namespace util
} // namespace osrm

#endif /* DEALLOCATING_VECTOR_HPP */
