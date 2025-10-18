#ifndef OSRM_FORMAT_HPP
#define OSRM_FORMAT_HPP

// Compatibility layer for std::format and fmt::format

#ifdef OSRM_HAS_STD_FORMAT

// Compile-time diagnostic: using std::format
#if defined(__GNUC__) || defined(__clang__)
#pragma message("OSRM: Using std::format from C++20 standard library")
#elif defined(_MSC_VER)
#pragma message("OSRM: Using std::format from C++20 standard library")
#endif

#include <cmath>   // Ensure std::isfinite and other math functions are available
#include <format>
#include <string>

namespace osrm::util::compat
{
// Use C++20 std::format when available
using std::format;
using std::to_string;
} // namespace osrm::util::compat

#else // Fallback to fmt library

// Compile-time diagnostic: using fmt::format
#if defined(__GNUC__) || defined(__clang__)
#pragma message("OSRM: Using fmt::format as fallback (std::format not available)")
#elif defined(_MSC_VER)
#pragma message("OSRM: Using fmt::format as fallback (std::format not available)")
#endif

#include <fmt/chrono.h>
#include <fmt/format.h>

namespace osrm::util::compat
{
// Use fmt library for backward compatibility
using fmt::format;
using fmt::to_string;
} // namespace osrm::util::compat

#endif // OSRM_HAS_STD_FORMAT

#endif // OSRM_FORMAT_HPP
