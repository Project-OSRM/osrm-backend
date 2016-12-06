#ifndef OSRM_UTIL_GUIDANCE_NAME_ANNOUNCEMENT_HPP_
#define OSRM_UTIL_GUIDANCE_NAME_ANNOUNCEMENT_HPP_

/* A set of tools required for guidance in both pre and post-processing */

#include "extractor/suffix_table.hpp"
#include "util/attributes.hpp"
#include "util/name_table.hpp"
#include "util/typedefs.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace osrm
{
namespace util
{
namespace guidance
{
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
                                  const std::string &from_pronunciation,
                                  const std::string &to_name,
                                  const std::string &to_ref,
                                  const std::string &to_pronunciation,
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

    const auto pronunciation_changes = from_pronunciation != to_pronunciation;

    return !obvious_change || needs_announce || pronunciation_changes;
}

// Overload without suffix checking
inline bool requiresNameAnnounced(const std::string &from_name,
                                  const std::string &from_ref,
                                  const std::string &from_pronunciation,
                                  const std::string &to_name,
                                  const std::string &to_ref,
                                  const std::string &to_pronunciation)
{
    // Dummy since we need to provide a SuffixTable but do not have the data for it.
    // (Guidance Post-Processing does not keep the suffix table around at the moment)
    struct NopSuffixTable final
    {
        NopSuffixTable() {}
        bool isSuffix(const std::string &) const { return false; }
    } static const table;

    return requiresNameAnnounced(
        from_name, from_ref, from_pronunciation, to_name, to_ref, to_pronunciation, table);
}

inline bool requiresNameAnnounced(const NameID from_name_id,
                                  const NameID to_name_id,
                                  const util::NameTable &name_table,
                                  const extractor::SuffixTable &suffix_table)
{
    if (from_name_id == to_name_id)
        return false;
    else
        return requiresNameAnnounced(name_table.GetNameForID(from_name_id),
                                     name_table.GetRefForID(from_name_id),
                                     name_table.GetPronunciationForID(from_name_id),
                                     name_table.GetNameForID(to_name_id),
                                     name_table.GetRefForID(to_name_id),
                                     name_table.GetPronunciationForID(to_name_id),
                                     suffix_table);
}

inline bool requiresNameAnnounced(const NameID from_name_id,
                                  const NameID to_name_id,
                                  const util::NameTable &name_table)
{
    if (from_name_id == to_name_id)
        return false;
    else
        return requiresNameAnnounced(name_table.GetNameForID(from_name_id),
                                     name_table.GetRefForID(from_name_id),
                                     name_table.GetPronunciationForID(from_name_id),
                                     name_table.GetNameForID(to_name_id),
                                     name_table.GetRefForID(to_name_id),
                                     name_table.GetPronunciationForID(to_name_id));
}

} // namespace guidance
} // namespace util
} // namespace osrm

#endif /* OSRM_UTIL_GUIDANCE_NAME_ANNOUNCEMENT_HPP_ */
