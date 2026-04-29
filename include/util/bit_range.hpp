#ifndef OSRM_UTIL_BIT_RANGE_HPP
#define OSRM_UTIL_BIT_RANGE_HPP

#include "util/msb.hpp"
#include <bit>
#include <ranges>

namespace osrm::util
{

template <typename DataT> class BitIterator
{
  public:
    using value_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using iterator_category = std::forward_iterator_tag;

    explicit BitIterator() : m_value(0) {}
    explicit BitIterator(const DataT x) : m_value(std::move(x)) {}

    reference operator*() const
    {
        // assumes m_value > 0
        return msb(m_value);
    }

    BitIterator &operator++()
    {
        auto index = msb(m_value);
        m_value = m_value & ~(DataT{1} << index);
        return *this;
    }

    BitIterator operator++(int)
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    friend bool operator==(const BitIterator &a, const BitIterator &b)
    {
        return a.m_value == b.m_value;
    }
    friend bool operator!=(const BitIterator &a, const BitIterator &b) { return !(a == b); }

  private:
    DataT m_value;
};

// Returns range over all 1 bits of value
template <typename T> struct BitRange
{
    explicit BitRange(T v) : value(v) {}
    BitIterator<T> begin() const { return BitIterator<T>(value); }
    BitIterator<T> end() const { return BitIterator<T>(0); }
    std::size_t size() const { return std::popcount(static_cast<std::make_unsigned_t<T>>(value)); }
    bool empty() const { return value == 0; }
    std::size_t front() const { return msb(value); }
    T value;
};

template <typename T> auto makeBitRange(const T value) { return BitRange<T>(value); }
} // namespace osrm::util

#endif
