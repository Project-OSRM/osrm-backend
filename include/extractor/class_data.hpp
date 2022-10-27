#ifndef OSRM_EXTRACTOR_CLASSES_DATA_HPP
#define OSRM_EXTRACTOR_CLASSES_DATA_HPP

#include "util/bit_range.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>

namespace osrm
{
namespace extractor
{

using ClassData = std::uint8_t;
constexpr ClassData INAVLID_CLASS_DATA = std::numeric_limits<ClassData>::max();
static const std::uint8_t MAX_CLASS_INDEX = 8 - 1;
static const std::uint8_t MAX_EXCLUDABLE_CLASSES = 8;

inline bool isSubset(const ClassData lhs, const ClassData rhs) { return (lhs & rhs) == lhs; }

inline auto getClassIndexes(const ClassData data) { return util::makeBitRange<ClassData>(data); }

inline auto getClassData(const std::size_t index)
{
    BOOST_ASSERT(index <= MAX_CLASS_INDEX);
    return uint8_t{1} << index;
}

inline bool isValidClassName(const std::string &name)
{
    return std::find_if_not(name.begin(), name.end(), [](const auto c) {
               return std::isalnum(c);
           }) == name.end();
}
} // namespace extractor
} // namespace osrm

#endif
