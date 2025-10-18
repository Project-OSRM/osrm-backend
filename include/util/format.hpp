#ifndef OSRM_FORMAT_HPP
#define OSRM_FORMAT_HPP

// Compatibility layer for std::format and fmt::format

#ifdef OSRM_HAS_STD_FORMAT

#include <cmath>
#include <format>
#include <string>

namespace osrm::util::compat
{
// Use C++20 std::format when available
using std::format;
using std::to_string;
} // namespace osrm::util::compat

#else // Fallback to fmt library

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
