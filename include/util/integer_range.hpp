#ifndef INTEGER_RANGE_HPP
#define INTEGER_RANGE_HPP

#include <boost/assert.hpp>

#include <type_traits>

#include <cstdint>

namespace osrm
{
namespace util
{

template <typename Integer> class range
{
  private:
    const Integer last;
    Integer iter;

  public:
    range(Integer start, Integer end) noexcept : last(end), iter(start)
    {
        BOOST_ASSERT_MSG(start <= end, "backwards counting ranges not suppoted");
        static_assert(std::is_integral<Integer>::value, "range type must be integral");
    }

    // Iterable functions
    const range &begin() const noexcept { return *this; }
    const range &end() const noexcept { return *this; }
    Integer front() const noexcept { return iter; }
    Integer back() const noexcept { return last - 1; }
    std::size_t size() const noexcept { return static_cast<std::size_t>(last - iter); }

    // Iterator functions
    bool operator!=(const range &) const noexcept { return iter < last; }
    void operator++() noexcept { ++iter; }
    Integer operator*() const noexcept { return iter; }
};

// convenience function to construct an integer range with type deduction
template <typename Integer>
range<Integer>
irange(const Integer first,
       const Integer last,
       typename std::enable_if<std::is_integral<Integer>::value>::type * = 0) noexcept
{
    return range<Integer>(first, last);
}

}
}

#endif // INTEGER_RANGE_HPP
