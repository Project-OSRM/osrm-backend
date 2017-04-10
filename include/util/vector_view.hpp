#ifndef UTIL_VECTOR_VIEW_HPP
#define UTIL_VECTOR_VIEW_HPP

#include "util/exception.hpp"
#include "util/log.hpp"

#include "storage/shared_memory_ownership.hpp"

#include <stxxl/vector>

#include <boost/assert.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/reverse_iterator.hpp>

#include <climits>
#include <cstddef>

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace osrm
{
namespace util
{

template <typename DataT>
class VectorViewIterator : public boost::iterator_facade<VectorViewIterator<DataT>,
                                                         DataT,
                                                         boost::random_access_traversal_tag>
{
    typedef boost::iterator_facade<VectorViewIterator<DataT>,
                                   DataT,
                                   boost::random_access_traversal_tag>
        base_t;

  public:
    typedef typename base_t::value_type value_type;
    typedef typename base_t::difference_type difference_type;
    typedef typename base_t::reference reference;
    typedef std::random_access_iterator_tag iterator_category;

    explicit VectorViewIterator() : m_value(nullptr) {}
    explicit VectorViewIterator(DataT *x) : m_value(x) {}

  private:
    void increment() { ++m_value; }
    void decrement() { --m_value; }
    void advance(difference_type offset) { m_value += offset; }
    bool equal(const VectorViewIterator &other) const { return m_value == other.m_value; }
    reference dereference() const { return *m_value; }
    difference_type distance_to(const VectorViewIterator &other) const
    {
        return other.m_value - m_value;
    }

    friend class ::boost::iterator_core_access;
    DataT *m_value;
};

template <typename DataT> class vector_view
{
  private:
    DataT *m_ptr;
    std::size_t m_size;

  public:
    using value_type = DataT;
    using iterator = VectorViewIterator<DataT>;
    using const_iterator = VectorViewIterator<const DataT>;
    using reverse_iterator = boost::reverse_iterator<iterator>;

    vector_view() : m_ptr(nullptr), m_size(0) {}

    vector_view(DataT *ptr, std::size_t size) : m_ptr(ptr), m_size(size) {}

    void reset(DataT *ptr, std::size_t size)
    {
        m_ptr = ptr;
        m_size = size;
    }

    void reset(void *ptr, std::size_t size)
    {
        m_ptr = reinterpret_cast<DataT *>(ptr);
        m_size = size;
    }

    DataT &at(const std::size_t index) { return m_ptr[index]; }

    const DataT &at(const std::size_t index) const { return m_ptr[index]; }

    auto begin() const { return iterator(m_ptr); }

    auto end() const { return iterator(m_ptr + m_size); }

    auto cbegin() const { return const_iterator(m_ptr); }

    auto cend() const { return const_iterator(m_ptr + m_size); }

    auto rbegin() const { return reverse_iterator(iterator(m_ptr + m_size)); }

    auto rend() const { return reverse_iterator(iterator(m_ptr)); }

    std::size_t size() const { return m_size; }

    bool empty() const { return 0 == size(); }

    DataT &operator[](const unsigned index)
    {
        BOOST_ASSERT_MSG(index < m_size, "invalid size");
        return m_ptr[index];
    }

    const DataT &operator[](const unsigned index) const
    {
        BOOST_ASSERT_MSG(index < m_size, "invalid size");
        return m_ptr[index];
    }

    const DataT &front() const
    {
        BOOST_ASSERT_MSG(m_size > 0, "invalid size");
        return m_ptr[0];
    }

    const DataT &back() const
    {
        BOOST_ASSERT_MSG(m_size > 0, "invalid size");
        return m_ptr[m_size - 1];
    }

    auto data() const { return m_ptr; }

    template <typename T> friend void swap(vector_view<T> &, vector_view<T> &) noexcept;
};

template <> class vector_view<bool>
{
  private:
    unsigned *m_ptr;
    std::size_t m_size;

  public:
    using value_type = bool;

    vector_view() : m_ptr(nullptr), m_size(0) {}

    vector_view(unsigned *ptr, std::size_t size) : m_ptr(ptr), m_size(size) {}

    bool at(const std::size_t index) const
    {
        BOOST_ASSERT_MSG(index < m_size, "invalid size");
        const std::size_t bucket = index / (CHAR_BIT * sizeof(unsigned));
        const unsigned offset = index % (CHAR_BIT * sizeof(unsigned));
        return m_ptr[bucket] & (1u << offset);
    }

    void reset(unsigned *, std::size_t size) { m_size = size; }

    std::size_t size() const { return m_size; }

    bool empty() const { return 0 == size(); }

    bool operator[](const unsigned index) const { return at(index); }

    template <typename T> friend void swap(vector_view<T> &, vector_view<T> &) noexcept;
};

// Both vector_view<T> and the vector_view<bool> specializations share this impl.
template <typename DataT> void swap(vector_view<DataT> &lhs, vector_view<DataT> &rhs) noexcept
{
    std::swap(lhs.m_ptr, rhs.m_ptr);
    std::swap(lhs.m_size, rhs.m_size);
}

template <typename DataT, storage::Ownership Ownership>
using InternalOrExternalVector =
    typename std::conditional<Ownership == storage::Ownership::External,
                              stxxl::vector<DataT>,
                              std::vector<DataT>>::type;

template <typename DataT, storage::Ownership Ownership>
using ViewOrVector = typename std::conditional<Ownership == storage::Ownership::View,
                                               vector_view<DataT>,
                                               InternalOrExternalVector<DataT, Ownership>>::type;
}
}

#endif // SHARED_MEMORY_VECTOR_WRAPPER_HPP
