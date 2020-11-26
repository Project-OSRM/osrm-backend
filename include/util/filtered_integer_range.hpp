#ifndef FILTERED_INTEGER_RANGE_HPP
#define FILTERED_INTEGER_RANGE_HPP

#include <boost/assert.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include <type_traits>

#include <cstdint>

namespace osrm
{
namespace util
{

// This implements a single-pass integer range.
// We need our own implementation here because using boost::adaptor::filtered() has
// the problem that the return-type depends on the lambda-type you pass into the function.
// That makes it unsuitable to use in interface where we would expect all filtered ranges
// to be off the same type.

template <typename Integer, typename Filter>
class filtered_integer_iterator
    : public boost::iterator_facade<filtered_integer_iterator<Integer, Filter>,
                                    Integer,
                                    boost::single_pass_traversal_tag,
                                    Integer>
{
    typedef boost::iterator_facade<filtered_integer_iterator<Integer, Filter>,
                                   Integer,
                                   boost::single_pass_traversal_tag,
                                   Integer>
        base_t;

  public:
    typedef typename base_t::value_type value_type;
    typedef typename base_t::difference_type difference_type;
    typedef typename base_t::reference reference;
    typedef std::random_access_iterator_tag iterator_category;

    filtered_integer_iterator() : value(), filter(nullptr) {}
    explicit filtered_integer_iterator(value_type x, value_type end_value, const Filter *filter)
        : value(x), end_value(end_value), filter(filter)
    {
    }

  private:
    void increment()
    {
        do
        {
            ++value;
        } while (value < end_value && !(*filter)[value]);
    }
    bool equal(const filtered_integer_iterator &other) const { return value == other.value; }
    reference dereference() const { return value; }

    friend class ::boost::iterator_core_access;
    value_type value;
    value_type end_value;
    const Filter *filter;
};

template <typename Integer, typename Filter> class filtered_range
{
  public:
    typedef filtered_integer_iterator<Integer, Filter> const_iterator;
    typedef filtered_integer_iterator<Integer, Filter> iterator;

    filtered_range(Integer begin, Integer end, const Filter &filter) : last(end, end, &filter)
    {
        while (begin < end && !filter[begin])
        {
            begin++;
        }

        iter = iterator(begin, end, &filter);
    }

    iterator begin() const noexcept { return iter; }
    iterator end() const noexcept { return last; }

  private:
    iterator iter;
    iterator last;
};

// convenience function to construct an integer range with type deduction
template <typename Integer, typename Filter>
filtered_range<Integer, Filter>
filtered_irange(const Integer first,
                const Integer last,
                const Filter &filter,
                typename std::enable_if<std::is_integral<Integer>::value>::type * = 0) noexcept
{
    return filtered_range<Integer, Filter>(first, last, filter);
}
} // namespace util
} // namespace osrm

#endif // INTEGER_RANGE_HPP
