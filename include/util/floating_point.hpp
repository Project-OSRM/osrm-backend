#ifndef FLOATING_POINT_HPP
#define FLOATING_POINT_HPP

#include <cmath>

#include <limits>
#include <type_traits>

namespace osrm
{
namespace util
{

template <typename FloatT> bool epsilon_compare(const FloatT number1, const FloatT number2)
{
    static_assert(std::is_floating_point<FloatT>::value, "type must be floating point");
    return (std::abs(number1 - number2) < std::numeric_limits<FloatT>::epsilon());
}
}
}

#endif // FLOATING_POINT_HPP
