#ifndef FILTERED_INTEGER_RANGE_HPP
#define FILTERED_INTEGER_RANGE_HPP

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace osrm::util
{

template <typename Integer, typename Filter> class filtered_integer_iterator
{
  public:
    using value_type = Integer;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using iterator_category = std::input_iterator_tag;

    filtered_integer_iterator() : value(), end_value(), filter(nullptr) {}
    explicit filtered_integer_iterator(value_type x, value_type end_value, const Filter *filter)
        : value(x), end_value(end_value), filter(filter)
    {
    }

    reference operator*() const { return value; }

    filtered_integer_iterator &operator++()
    {
        do
        {
            ++value;
        } while (value < end_value && !(*filter)[value]);
        return *this;
    }

    filtered_integer_iterator operator++(int)
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    friend bool operator==(const filtered_integer_iterator &a, const filtered_integer_iterator &b)
    {
        return a.value == b.value;
    }
    friend bool operator!=(const filtered_integer_iterator &a, const filtered_integer_iterator &b)
    {
        return !(a == b);
    }

  private:
    value_type value;
    value_type end_value;
    const Filter *filter;
};

template <typename Integer, typename Filter> class filtered_range
{
  public:
    using const_iterator = filtered_integer_iterator<Integer, Filter>;
    using iterator = filtered_integer_iterator<Integer, Filter>;

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

template <typename Integer, typename Filter>
filtered_range<Integer, Filter> filtered_irange(
    const Integer first,
    const Integer last,
    const Filter &filter,
    typename std::enable_if<std::is_integral<Integer>::value>::type * = nullptr) noexcept
{
    return filtered_range<Integer, Filter>(first, last, filter);
}

} // namespace osrm::util

#endif // FILTERED_INTEGER_RANGE_HPP
