#ifndef OSRM_FORMAT_HPP
#define OSRM_FORMAT_HPP

#include <cmath>
#include <format>
#include <string>
#include <string_view>

namespace osrm::util::compat
{

// std::runtime_format is only available in C++26; provide a compatible wrapper using std::vformat
struct RuntimeFormatString
{
    std::string_view str;
};

inline RuntimeFormatString runtime_format(std::string_view s) { return {s}; }

template <typename... Args> std::string format(RuntimeFormatString fmt, Args &&...args)
{ return std::vformat(fmt.str, std::make_format_args(args...)); }
} // namespace osrm::util::compat

#endif // OSRM_FORMAT_HPP
