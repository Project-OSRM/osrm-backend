#ifndef SHARED_MEMORY_VECTOR_WRAPPER_HPP
#define SHARED_MEMORY_VECTOR_WRAPPER_HPP

#include <boost/assert.hpp>

#include <cstddef>

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <vector>
#include <utility>

namespace osrm
{
namespace util
{

template <typename DataT> class ShMemIterator : public std::iterator<std::input_iterator_tag, DataT>
{
    DataT *p;

  public:
    explicit ShMemIterator(DataT *x) : p(x) {}
    ShMemIterator(const ShMemIterator &mit) : p(mit.p) {}
    ShMemIterator &operator++()
    {
        ++p;
        return *this;
    }
    ShMemIterator operator++(int)
    {
        ShMemIterator tmp(*this);
        operator++();
        return tmp;
    }
    ShMemIterator operator+(std::ptrdiff_t diff)
    {
        ShMemIterator tmp(p + diff);
        return tmp;
    }
    bool operator==(const ShMemIterator &rhs) { return p == rhs.p; }
    bool operator!=(const ShMemIterator &rhs) { return p != rhs.p; }
    DataT &operator*() { return *p; }
};

template <typename DataT> class SharedMemoryWrapper
{
  private:
    DataT *m_ptr;
    std::size_t m_size;

  public:
    SharedMemoryWrapper() : m_ptr(nullptr), m_size(0) {}

    SharedMemoryWrapper(DataT *ptr, std::size_t size) : m_ptr(ptr), m_size(size) {}

    void reset(DataT *ptr, std::size_t size)
    {
        m_ptr = ptr;
        m_size = size;
    }

    DataT &at(const std::size_t index) { return m_ptr[index]; }

    const DataT &at(const std::size_t index) const { return m_ptr[index]; }

    ShMemIterator<DataT> begin() const { return ShMemIterator<DataT>(m_ptr); }

    ShMemIterator<DataT> end() const { return ShMemIterator<DataT>(m_ptr + m_size); }

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
        const std::size_t bucket = index / 32;
        const unsigned offset = static_cast<unsigned>(index % 32);
        return m_ptr[bucket] & (1u << offset);
    }

    void reset(unsigned *ptr, std::size_t size)
    {
        m_ptr = ptr;
        m_size = size;
    }

    std::size_t size() const { return m_size; }

    bool empty() const { return 0 == size(); }

    bool operator[](const unsigned index)
    {
        BOOST_ASSERT_MSG(index < m_size, "invalid size");
        const unsigned bucket = index / 32;
        const unsigned offset = index % 32;
        return m_ptr[bucket] & (1u << offset);
    }

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
