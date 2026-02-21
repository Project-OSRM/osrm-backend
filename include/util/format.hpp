#ifndef OSRM_FORMAT_HPP
#define OSRM_FORMAT_HPP

// Compatibility layer for std::format and fmt::format

#ifdef OSRM_HAS_STD_FORMAT

#include <cmath>
#include <format>
#include <string>
#include <string_view>

namespace osrm::util::compat
{
// Use C++20 std::format when available
using std::format;
using std::to_string;

// std::runtime_format is only available in C++26; provide a compatible wrapper using std::vformat
struct RuntimeFormatString
{
    std::string_view str;
};

inline RuntimeFormatString runtime_format(std::string_view s) { return {s}; }

template <typename... Args> std::string format(RuntimeFormatString fmt, Args &&...args)
{
    return std::vformat(fmt.str, std::make_format_args(args...));
}
} // namespace osrm::util::compat

#else // Fallback to fmt library

#include <fmt/chrono.h>
#include <fmt/format.h>

namespace osrm::util::compat
{
// Use fmt library for backward compatibility
using fmt::format;
using fmt::runtime;
using fmt::to_string;

inline auto runtime_format(std::string_view s) { return fmt::runtime(s); }
} // namespace osrm::util::compat

#endif // OSRM_HAS_STD_FORMAT

#endif // OSRM_FORMAT_HPP
