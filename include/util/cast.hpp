#ifndef CAST_HPP
#define CAST_HPP

#include <iomanip>
#include <sstream>
#include <string>
#include <type_traits>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace osrm
{
namespace util
{

namespace cast
{
template <typename Enumeration>
inline auto enum_to_underlying(Enumeration const value) ->
    typename std::underlying_type<Enumeration>::type
{
    return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

template <typename T, int Precision = 6> inline std::string to_string_with_precision(const T x)
{
    static_assert(std::is_arithmetic<T>::value, "integral or floating point type required");

    std::ostringstream out;
    out << std::fixed << std::setprecision(Precision) << x;
    auto rv = out.str();

    // Javascript has no separation of float / int, digits without a '.' are integral typed
    // X.Y.0 -> X.Y
    // X.0 -> X
    boost::trim_right_if(rv, boost::is_any_of("0"));
    boost::trim_right_if(rv, boost::is_any_of("."));
    // Note:
    //  - assumes the locale to use '.' as digit separator
    //  - this is not identical to:  trim_right_if(rv, is_any_of('0 .'))

    return rv;
}
} // namespace cast
} // namespace util
} // namespace osrm

#endif // CAST_HPP
