#ifndef OSRM_EXTRACTOR_CLASSES_DATA_HPP
#define OSRM_EXTRACTOR_CLASSES_DATA_HPP

#include "util/bit_range.hpp"

#include <cstdint>

namespace osrm
{
namespace extractor
{

using ClassData = std::uint8_t;
static const std::uint8_t MAX_CLASS_INDEX = 8 - 1;
static const std::uint8_t MAX_AVOIDABLE_CLASSES = 8;

inline bool isSubset(const ClassData lhs, const ClassData rhs) { return (lhs & rhs) == lhs; }

inline auto getClassIndexes(const ClassData data) { return util::makeBitRange<ClassData>(data); }
}
}

#endif
