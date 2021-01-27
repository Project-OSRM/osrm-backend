#ifndef OSRM_UTIL_BIT_RANGE_HPP
#define OSRM_UTIL_BIT_RANGE_HPP

#include "util/msb.hpp"

#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>

namespace osrm
{
namespace util
{

namespace detail
{
template <typename T> std::size_t countOnes(T value)
{
    static_assert(std::is_unsigned<T>::value, "Only unsigned types allowed");
    std::size_t number_of_ones = 0;
    while (value > 0)
    {
        auto index = msb(value);
        value = value & ~(T{1} << index);
        number_of_ones++;
    }
    return number_of_ones;
}

#if (defined(__clang__) || defined(__GNUC__) || defined(__GNUG__))
inline std::size_t countOnes(std::uint8_t value)
{
    return __builtin_popcount(std::uint32_t{value});
}
inline std::size_t countOnes(std::uint16_t value)
{
    return __builtin_popcount(std::uint32_t{value});
}
inline std::size_t countOnes(unsigned int value) { return __builtin_popcount(value); }
inline std::size_t countOnes(unsigned long value) { return __builtin_popcountl(value); }
inline std::size_t countOnes(unsigned long long value) { return __builtin_popcountll(value); }
#endif
} // namespace detail

// Investigate if we can replace this with
// http://www.boost.org/doc/libs/1_64_0/libs/dynamic_bitset/dynamic_bitset.html
template <typename DataT>
class BitIterator : public boost::iterator_facade<BitIterator<DataT>,
                                                  const std::size_t,
                                                  boost::forward_traversal_tag,
                                                  const std::size_t>
{
    typedef boost::iterator_facade<BitIterator<DataT>,
                                   const std::size_t,
                                   boost::forward_traversal_tag,
                                   const std::size_t>
        base_t;

  public:
    typedef typename base_t::value_type value_type;
    typedef typename base_t::difference_type difference_type;
    typedef typename base_t::reference reference;
    typedef std::random_access_iterator_tag iterator_category;

    explicit BitIterator() : m_value(0) {}
    explicit BitIterator(const DataT x) : m_value(std::move(x)) {}

  private:
    void increment()
    {
        auto index = msb(m_value);
        m_value = m_value & ~(DataT{1} << index);
    }

    difference_type distance_to(const BitIterator &other) const
    {
        return detail::countOnes(m_value) - detail::countOnes(other.m_value);
    }

    bool equal(const BitIterator &other) const { return m_value == other.m_value; }

    reference dereference() const
    {
        BOOST_ASSERT(m_value > 0);
        return msb(m_value);
    }

    friend class ::boost::iterator_core_access;
    DataT m_value;
};

// Returns range over all 1 bits of value
template <typename T> auto makeBitRange(const T value)
{
    return boost::make_iterator_range(BitIterator<T>{value}, BitIterator<T>{});
}
} // namespace util
} // namespace osrm

#endif
