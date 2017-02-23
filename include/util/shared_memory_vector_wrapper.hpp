#ifndef SHARED_MEMORY_VECTOR_WRAPPER_HPP
#define SHARED_MEMORY_VECTOR_WRAPPER_HPP

#include "util/log.hpp"

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
class ShMemIterator
    : public boost::iterator_facade<ShMemIterator<DataT>, DataT, boost::random_access_traversal_tag>
{
    typedef boost::iterator_facade<ShMemIterator<DataT>, DataT, boost::random_access_traversal_tag>
        base_t;

  public:
    typedef typename base_t::value_type value_type;
    typedef typename base_t::difference_type difference_type;
    typedef typename base_t::reference reference;
    typedef std::random_access_iterator_tag iterator_category;

    explicit ShMemIterator(DataT *x) : m_value(x) {}

  private:
    void increment() { ++m_value; }
    void decrement() { --m_value; }
    void advance(difference_type offset) { m_value += offset; }
    bool equal(const ShMemIterator &other) const { return m_value == other.m_value; }
    reference dereference() const { return *m_value; }
    difference_type distance_to(const ShMemIterator &other) const
    {
        return other.m_value - m_value;
    }

    friend class ::boost::iterator_core_access;
    DataT *m_value;
};

template <typename DataT> class SharedMemoryWrapper
{
  private:
    DataT *m_ptr;
    std::size_t m_size;

  public:
    using value_type = DataT;
    using iterator = ShMemIterator<DataT>;
    using reverse_iterator = boost::reverse_iterator<iterator>;

    SharedMemoryWrapper() : m_ptr(nullptr), m_size(0) {}

    SharedMemoryWrapper(DataT *ptr, std::size_t size) : m_ptr(ptr), m_size(size) {}

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

    template <typename T>
    friend void swap(SharedMemoryWrapper<T> &, SharedMemoryWrapper<T> &) noexcept;
};

template <> class SharedMemoryWrapper<bool>
{
  private:
    unsigned *m_ptr;
    std::size_t m_size;

  public:
    SharedMemoryWrapper() : m_ptr(nullptr), m_size(0) {}

    SharedMemoryWrapper(unsigned *ptr, std::size_t size) : m_ptr(ptr), m_size(size) {}

    bool at(const std::size_t index) const
    {
        BOOST_ASSERT_MSG(index < m_size, "invalid size");
        const std::size_t bucket = index / (CHAR_BIT * sizeof(unsigned));
        const unsigned offset = index % (CHAR_BIT * sizeof(unsigned));
        return m_ptr[bucket] & (1u << offset);
    }

    void reset(unsigned *ptr, std::size_t size)
    {
        m_ptr = ptr;
        m_size = size;
    }

    std::size_t size() const { return m_size; }

    bool empty() const { return 0 == size(); }

    bool operator[](const unsigned index) const { return at(index); }

    template <typename T>
    friend void swap(SharedMemoryWrapper<T> &, SharedMemoryWrapper<T> &) noexcept;
};

// Both SharedMemoryWrapper<T> and the SharedMemoryWrapper<bool> specializations share this impl.
template <typename DataT>
void swap(SharedMemoryWrapper<DataT> &lhs, SharedMemoryWrapper<DataT> &rhs) noexcept
{
    std::swap(lhs.m_ptr, rhs.m_ptr);
    std::swap(lhs.m_size, rhs.m_size);
}

template <typename DataT, bool UseSharedMemory> struct ShM
{
    using vector = typename std::conditional<UseSharedMemory,
                                             SharedMemoryWrapper<DataT>,
                                             std::vector<DataT>>::type;
};
}
}

#endif // SHARED_MEMORY_VECTOR_WRAPPER_HPP
