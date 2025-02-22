#ifndef INTEGER_RANGE_HPP
#define INTEGER_RANGE_HPP

#include <boost/assert.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include <type_traits>

#include <cstdint>

namespace osrm::util
{

// Ported from Boost.Range 1.56 due to required fix
// https://github.com/boostorg/range/commit/9e6bdc13ba94af4e150afae547557a2fbbfe3bf0
// Can be removed after dropping support of Boost < 1.56
template <typename Integer>
class integer_iterator : public boost::iterator_facade<integer_iterator<Integer>,
                                                       Integer,
                                                       boost::random_access_traversal_tag,
                                                       Integer>
{
    using base_t = boost::iterator_facade<integer_iterator<Integer>,
                                          Integer,
                                          boost::random_access_traversal_tag,
                                          Integer>;

  public:
    using value_type = typename base_t::value_type;
    using difference_type = typename base_t::difference_type;
    using reference = typename base_t::reference;
    using iterator_category = std::random_access_iterator_tag;

    integer_iterator() : m_value() {}
    explicit integer_iterator(value_type x) : m_value(x) {}

  private:
    void increment() { ++m_value; }
    void decrement() { --m_value; }
    void advance(difference_type offset) { m_value += offset; }
    bool equal(const integer_iterator &other) const { return m_value == other.m_value; }
    reference dereference() const { return m_value; }

    difference_type distance_to(const integer_iterator &other) const
    {
        return std::is_signed<value_type>::value ? (other.m_value - m_value)
               : (other.m_value >= m_value)
                   ? static_cast<difference_type>(other.m_value - m_value)
                   : -static_cast<difference_type>(m_value - other.m_value);
    }

    friend class ::boost::iterator_core_access;
    value_type m_value;
};

// Warning: do not try to replace this with Boost's irange, as it is broken on Boost 1.55:
//     auto r = boost::irange<unsigned int>(0, 15);
//     std::cout << r.size() << std::endl;
// results in -4294967281. Latest Boost versions fix this, but we still support older ones.

template <typename Integer> class range
{
  public:
    using const_iterator = integer_iterator<Integer>;
    using iterator = integer_iterator<Integer>;

    range(Integer begin, Integer end) : iter(begin), last(end) {}

    iterator begin() const noexcept { return iter; }
    iterator end() const noexcept { return last; }
    Integer front() const noexcept { return *iter; }
    Integer back() const noexcept { return *last - 1; }
    std::size_t size() const noexcept { return static_cast<std::size_t>(last - iter); }

  private:
    iterator iter;
    iterator last;
};

// convenience function to construct an integer range with type deduction
template <typename Integer>
range<Integer>
irange(const Integer first,
       const Integer last,
       typename std::enable_if<std::is_integral<Integer>::value>::type * = nullptr) noexcept
{
    return range<Integer>(first, last);
}
} // namespace osrm::util

#endif // INTEGER_RANGE_HPP
