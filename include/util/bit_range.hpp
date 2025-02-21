#ifndef OSRM_UTIL_BIT_RANGE_HPP
#define OSRM_UTIL_BIT_RANGE_HPP

#include "util/msb.hpp"
#include <bit>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>

namespace osrm::util
{

// Investigate if we can replace this with
// http://www.boost.org/doc/libs/1_64_0/libs/dynamic_bitset/dynamic_bitset.html
template <typename DataT>
class BitIterator : public boost::iterator_facade<BitIterator<DataT>,
                                                  const std::size_t,
                                                  boost::forward_traversal_tag,
                                                  const std::size_t>
{
    using base_t = boost::iterator_facade<BitIterator<DataT>,
                                          const std::size_t,
                                          boost::forward_traversal_tag,
                                          const std::size_t>;

  public:
    using value_type = typename base_t::value_type;
    using difference_type = typename base_t::difference_type;
    using reference = typename base_t::reference;
    using iterator_category = std::random_access_iterator_tag;

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
        return std::popcount(m_value) - std::popcount(other.m_value);
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
} // namespace osrm::util

#endif
