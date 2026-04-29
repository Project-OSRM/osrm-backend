#ifndef INTEGER_RANGE_HPP
#define INTEGER_RANGE_HPP

#include <iterator>
#include <type_traits>

namespace osrm::util
{

template <typename Integer> class integer_iterator
{
  public:
    using value_type = Integer;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using iterator_category = std::random_access_iterator_tag;

    integer_iterator() : m_value() {}
    explicit integer_iterator(value_type x) : m_value(x) {}

    reference operator*() const { return m_value; }

    integer_iterator &operator++()
    {
        ++m_value;
        return *this;
    }
    integer_iterator operator++(int)
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    integer_iterator &operator--()
    {
        --m_value;
        return *this;
    }
    integer_iterator operator--(int)
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    integer_iterator &operator+=(difference_type n)
    {
        m_value += n;
        return *this;
    }
    integer_iterator &operator-=(difference_type n)
    {
        m_value -= n;
        return *this;
    }

    friend integer_iterator operator+(integer_iterator it, difference_type n)
    {
        it += n;
        return it;
    }
    friend integer_iterator operator-(integer_iterator it, difference_type n)
    {
        it -= n;
        return it;
    }

    friend difference_type operator-(const integer_iterator &a, const integer_iterator &b)
    {
        // a - b
        return static_cast<difference_type>(a.m_value) - static_cast<difference_type>(b.m_value);
    }

    friend bool operator==(const integer_iterator &a, const integer_iterator &b)
    {
        return a.m_value == b.m_value;
    }
    friend bool operator!=(const integer_iterator &a, const integer_iterator &b)
    {
        return !(a == b);
    }

  private:
    value_type m_value;
};

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
