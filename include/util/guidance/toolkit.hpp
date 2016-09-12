#ifndef OSRM_UTIL_GUIDANCE_TOOLKIT_HPP_
#define OSRM_UTIL_GUIDANCE_TOOLKIT_HPP_

/* A set of tools required for guidance in both pre and post-processing */

#include "extractor/guidance/turn_instruction.hpp"
#include "engine/guidance/route_step.hpp"
#include "engine/phantom_node.hpp"
#include "util/attributes.hpp"
#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"
#include "util/simple_logger.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash.hpp>

namespace osrm
{
namespace util
{
namespace guidance
{

inline double angularDeviation(const double angle, const double from)
{
    const double deviation = std::abs(angle - from);
    return std::min(360 - deviation, deviation);
}

inline extractor::guidance::DirectionModifier::Enum getTurnDirection(const double angle)
{
    // An angle of zero is a u-turn
    // 180 goes perfectly straight
    // 0-180 are right turns
    // 180-360 are left turns
    if (angle > 0 && angle < 60)
        return extractor::guidance::DirectionModifier::SharpRight;
    if (angle >= 60 && angle < 140)
        return extractor::guidance::DirectionModifier::Right;
    if (angle >= 140 && angle < 170)
        return extractor::guidance::DirectionModifier::SlightRight;
    if (angle >= 165 && angle <= 195)
        return extractor::guidance::DirectionModifier::Straight;
    if (angle > 190 && angle <= 220)
        return extractor::guidance::DirectionModifier::SlightLeft;
    if (angle > 220 && angle <= 300)
        return extractor::guidance::DirectionModifier::Left;
    if (angle > 300 && angle < 360)
        return extractor::guidance::DirectionModifier::SharpLeft;
    return extractor::guidance::DirectionModifier::UTurn;
}

// swaps left <-> right modifier types
OSRM_ATTR_WARN_UNUSED
inline extractor::guidance::DirectionModifier::Enum
mirrorDirectionModifier(const extractor::guidance::DirectionModifier::Enum modifier)
{
    const constexpr extractor::guidance::DirectionModifier::Enum results[] = {
        extractor::guidance::DirectionModifier::UTurn,
        extractor::guidance::DirectionModifier::SharpLeft,
        extractor::guidance::DirectionModifier::Left,
        extractor::guidance::DirectionModifier::SlightLeft,
        extractor::guidance::DirectionModifier::Straight,
        extractor::guidance::DirectionModifier::SlightRight,
        extractor::guidance::DirectionModifier::Right,
        extractor::guidance::DirectionModifier::SharpRight};
    return results[modifier];
}

inline bool hasLeftModifier(const extractor::guidance::TurnInstruction instruction)
{
    return instruction.direction_modifier == extractor::guidance::DirectionModifier::SharpLeft ||
           instruction.direction_modifier == extractor::guidance::DirectionModifier::Left ||
           instruction.direction_modifier == extractor::guidance::DirectionModifier::SlightLeft;
}

inline bool hasRightModifier(const extractor::guidance::TurnInstruction instruction)
{
    return instruction.direction_modifier == extractor::guidance::DirectionModifier::SharpRight ||
           instruction.direction_modifier == extractor::guidance::DirectionModifier::Right ||
           instruction.direction_modifier == extractor::guidance::DirectionModifier::SlightRight;
}

inline bool isLeftTurn(const extractor::guidance::TurnInstruction instruction)
{
    switch (instruction.type)
    {
    case extractor::guidance::TurnType::Merge:
        return hasRightModifier(instruction);
    default:
        return hasLeftModifier(instruction);
    }
}

inline bool isRightTurn(const extractor::guidance::TurnInstruction instruction)
{
    switch (instruction.type)
    {
    case extractor::guidance::TurnType::Merge:
        return hasLeftModifier(instruction);
    default:
        return hasRightModifier(instruction);
    }
}

inline bool entersRoundabout(const extractor::guidance::TurnInstruction instruction)
{
    return (instruction.type == extractor::guidance::TurnType::EnterRoundabout ||
            instruction.type == extractor::guidance::TurnType::EnterRotary ||
            instruction.type == extractor::guidance::TurnType::EnterRoundaboutIntersection ||
            instruction.type == extractor::guidance::TurnType::EnterRoundaboutAtExit ||
            instruction.type == extractor::guidance::TurnType::EnterRotaryAtExit ||
            instruction.type == extractor::guidance::TurnType::EnterRoundaboutIntersectionAtExit ||
            instruction.type == extractor::guidance::TurnType::EnterAndExitRoundabout ||
            instruction.type == extractor::guidance::TurnType::EnterAndExitRotary ||
            instruction.type == extractor::guidance::TurnType::EnterAndExitRoundaboutIntersection);
}

inline bool leavesRoundabout(const extractor::guidance::TurnInstruction instruction)
{
    return (instruction.type == extractor::guidance::TurnType::ExitRoundabout ||
            instruction.type == extractor::guidance::TurnType::ExitRotary ||
            instruction.type == extractor::guidance::TurnType::ExitRoundaboutIntersection ||
            instruction.type == extractor::guidance::TurnType::EnterAndExitRoundabout ||
            instruction.type == extractor::guidance::TurnType::EnterAndExitRotary ||
            instruction.type == extractor::guidance::TurnType::EnterAndExitRoundaboutIntersection);
}

inline bool staysOnRoundabout(const extractor::guidance::TurnInstruction instruction)
{
    return instruction.type == extractor::guidance::TurnType::StayOnRoundabout;
}

// Name Change Logic
// Used both during Extraction as well as during Post-Processing

inline std::pair<std::string, std::string> getPrefixAndSuffix(const std::string &data)
{
    const auto suffix_pos = data.find_last_of(' ');
    if (suffix_pos == std::string::npos)
        return {};

    const auto prefix_pos = data.find_first_of(' ');
    auto result = std::make_pair(data.substr(0, prefix_pos), data.substr(suffix_pos + 1));
    boost::to_lower(result.first);
    boost::to_lower(result.second);
    return result;
}

// Note: there is an overload without suffix checking below.
// (that's the reason we template the suffix table here)
template <typename SuffixTable>
inline bool requiresNameAnnounced(const std::string &from_name,
                                  const std::string &from_ref,
                                  const std::string &to_name,
                                  const std::string &to_ref,
                                  const SuffixTable &suffix_table)
{
    // first is empty and the second is not
    if ((from_name.empty() && from_ref.empty()) && !(to_name.empty() && to_ref.empty()))
        return true;

    // FIXME, handle in profile to begin with?
    // Input for this function should be a struct separating streetname, suffix (e.g. road,
    // boulevard, North, West ...), and a list of references

    // check similarity of names
    const auto names_are_empty = from_name.empty() && to_name.empty();
    const auto name_is_contained =
        boost::starts_with(from_name, to_name) || boost::starts_with(to_name, from_name);

    const auto checkForPrefixOrSuffixChange = [](
        const std::string &first, const std::string &second, const SuffixTable &suffix_table) {

        const auto first_prefix_and_suffixes = getPrefixAndSuffix(first);
        const auto second_prefix_and_suffixes = getPrefixAndSuffix(second);

        // reverse strings, get suffices and reverse them to get prefixes
        const auto checkTable = [&](const std::string &str) {
            return str.empty() || suffix_table.isSuffix(str);
        };

        const auto getOffset = [](const std::string &str) -> std::size_t {
            if (str.empty())
                return 0;
            else
                return str.length() + 1;
        };

        const bool is_prefix_change = [&]() -> bool {
            if (!checkTable(first_prefix_and_suffixes.first))
                return false;
            if (!checkTable(second_prefix_and_suffixes.first))
                return false;
            return !first.compare(getOffset(first_prefix_and_suffixes.first),
                                  std::string::npos,
                                  second,
                                  getOffset(second_prefix_and_suffixes.first),
                                  std::string::npos);
        }();

        const bool is_suffix_change = [&]() -> bool {
            if (!checkTable(first_prefix_and_suffixes.second))
                return false;
            if (!checkTable(second_prefix_and_suffixes.second))
                return false;
            return !first.compare(0,
                                  first.length() - getOffset(first_prefix_and_suffixes.second),
                                  second,
                                  0,
                                  second.length() - getOffset(second_prefix_and_suffixes.second));
        }();

        return is_prefix_change || is_suffix_change;
    };

    const auto is_suffix_change = checkForPrefixOrSuffixChange(from_name, to_name, suffix_table);
    const auto names_are_equal = from_name == to_name || name_is_contained || is_suffix_change;
    const auto name_is_removed = !from_name.empty() && to_name.empty();
    // references are contained in one another
    const auto refs_are_empty = from_ref.empty() && to_ref.empty();
    const auto ref_is_contained =
        from_ref.empty() || to_ref.empty() ||
        (from_ref.find(to_ref) != std::string::npos || to_ref.find(from_ref) != std::string::npos);
    const auto ref_is_removed = !from_ref.empty() && to_ref.empty();

    const auto obvious_change =
        (names_are_empty && refs_are_empty) || (names_are_equal && ref_is_contained) ||
        (names_are_equal && refs_are_empty) || (ref_is_contained && name_is_removed) ||
        (names_are_equal && ref_is_removed) || is_suffix_change;

    const auto needs_announce =
        // " (Ref)" -> "Name "
        (from_name.empty() && !from_ref.empty() && !to_name.empty() && to_ref.empty());

    return !obvious_change || needs_announce;
}

// Overload without suffix checking
inline bool requiresNameAnnounced(const std::string &from_name,
                                  const std::string &from_ref,
                                  const std::string &to_name,
                                  const std::string &to_ref)
{
    // Dummy since we need to provide a SuffixTable but do not have the data for it.
    // (Guidance Post-Processing does not keep the suffix table around at the moment)
    struct NopSuffixTable final
    {
        NopSuffixTable(){}
        bool isSuffix(const std::string &) const { return false; }
    } static const table;

    return requiresNameAnnounced(from_name, from_ref, to_name, to_ref, table);
}

} // namespace guidance
} // namespace util
} // namespace osrm

#endif /* OSRM_UTIL_GUIDANCE_TOOLKIT_HPP_ */
