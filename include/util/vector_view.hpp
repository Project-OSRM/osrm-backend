#ifndef UTIL_VECTOR_VIEW_HPP
#define UTIL_VECTOR_VIEW_HPP

#include "util/exception.hpp"
#include "util/log.hpp"

#include "storage/shared_memory_ownership.hpp"

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

#if USE_STXXL_LIBRARY
#include <stxxl/vector>
#endif

namespace osrm
{
namespace util
{

template <typename DataT>
class VectorViewIterator : public boost::iterator_facade<VectorViewIterator<DataT>,
                                                         DataT,
                                                         boost::random_access_traversal_tag,
                                                         DataT &>
{
    typedef boost::iterator_facade<VectorViewIterator<DataT>,
                                   DataT,
                                   boost::random_access_traversal_tag,
                                   DataT &>
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

    void resize(const size_t size)
    {
        if (size > m_size)
        {
            throw util::exception("Trying to resize a view to a larger size.");
        }
        m_size = size;
    }

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
  public:
    using Word = std::uint64_t;

  private:
    static constexpr std::size_t WORD_BITS = CHAR_BIT * sizeof(Word);

    Word *m_ptr;
    std::size_t m_size;

  public:
    using value_type = bool;
    struct reference
    {
        reference &operator=(bool value)
        {
            *m_ptr = (*m_ptr & ~mask) | (static_cast<unsigned>(value) * mask);
            return *this;
        }

        operator bool() const { return (*m_ptr) & mask; }

        bool operator==(const reference &other) const
        {
            return other.m_ptr == m_ptr && other.mask == mask;
        }

        friend std::ostream &operator<<(std::ostream &os, const reference &rhs)
        {
            return os << static_cast<bool>(rhs);
        }

        Word *m_ptr;
        const Word mask;
    };

    vector_view() : m_ptr(nullptr), m_size(0) {}

    vector_view(Word *ptr, std::size_t size) : m_ptr(ptr), m_size(size) {}

    bool at(const std::size_t index) const
    {
        BOOST_ASSERT_MSG(index < m_size, "invalid size");
        const std::size_t bucket = index / WORD_BITS;
        // Note: ordering of bits here should match packBits in storage/serialization.hpp
        //       so that directly mmap-ing data is possible
        const auto offset = index % WORD_BITS;
        BOOST_ASSERT(WORD_BITS > offset);
        return m_ptr[bucket] & (static_cast<Word>(1) << offset);
    }

    void reset(std::uint64_t *ptr, std::size_t size)
    {
        m_ptr = ptr;
        m_size = size;
    }

    void resize(const size_t size)
    {
        if (size > m_size)
        {
            throw util::exception("Trying to resize a view to a larger size.");
        }
        m_size = size;
    }

    std::size_t size() const { return m_size; }

    bool empty() const { return 0 == size(); }

    bool operator[](const std::size_t index) const { return at(index); }

    reference operator[](const std::size_t index)
    {
        BOOST_ASSERT(index < m_size);
        const auto bucket = index / WORD_BITS;
        // Note: ordering of bits here should match packBits in storage/serialization.hpp
        //       so that directly mmap-ing data is possible
        const auto offset = index % WORD_BITS;
        BOOST_ASSERT(WORD_BITS > offset);
        return reference{m_ptr + bucket, static_cast<Word>(1) << offset};
    }

    template <typename T> friend void swap(vector_view<T> &, vector_view<T> &) noexcept;

    friend std::ostream &operator<<(std::ostream &os, const vector_view<bool> &rhs)
    {
        for (std::size_t i = 0; i < rhs.size(); ++i)
        {
            os << (i > 0 ? " " : "") << rhs.at(i);
        }
        return os;
    }
};

// Both vector_view<T> and the vector_view<bool> specializations share this impl.
template <typename DataT> void swap(vector_view<DataT> &lhs, vector_view<DataT> &rhs) noexcept
{
    std::swap(lhs.m_ptr, rhs.m_ptr);
    std::swap(lhs.m_size, rhs.m_size);
}

#if USE_STXXL_LIBRARY
template <typename T> using ExternalVector = stxxl::vector<T>;
#else
template <typename T> using ExternalVector = std::vector<T>;
#endif

template <typename DataT, storage::Ownership Ownership>
using InternalOrExternalVector =
    typename std::conditional<Ownership == storage::Ownership::External,
                              ExternalVector<DataT>,
                              std::vector<DataT>>::type;

template <typename DataT, storage::Ownership Ownership>
using ViewOrVector = typename std::conditional<Ownership == storage::Ownership::View,
                                               vector_view<DataT>,
                                               InternalOrExternalVector<DataT, Ownership>>::type;

// We can use this for compile time assertions
template <typename ValueT, typename VectorT>
struct is_view_or_vector
    : std::integral_constant<bool,
                             std::is_same<std::vector<ValueT>, VectorT>::value ||
                                 std::is_same<util::vector_view<ValueT>, VectorT>::value>
{
};
}
}

#endif // SHARED_MEMORY_VECTOR_WRAPPER_HPP
