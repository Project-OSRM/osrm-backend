#ifndef OSRM_UTIL_GUIDANCE_NAME_ANNOUNCEMENT_HPP_
#define OSRM_UTIL_GUIDANCE_NAME_ANNOUNCEMENT_HPP_

/* A set of tools required for guidance in both pre and post-processing */

#include "extractor/suffix_table.hpp"
#include "util/attributes.hpp"
#include "util/name_table.hpp"
#include "util/typedefs.hpp"

#include <algorithm>
#include <string>
#include <tuple>
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

inline std::string longest_common_substring(const std::string &lhs, const std::string &rhs)
{
    if (lhs.empty() || rhs.empty())
        return "";

    // array for dynamic programming
    std::vector<std::vector<std::uint32_t>> dp(lhs.size(),
                                               std::vector<std::uint32_t>(rhs.size(), 0));

    // to remember the best location
    std::uint32_t best = 0;
    std::uint32_t best_pos = 0;

    for (std::uint32_t i = 0; i < lhs.size(); ++i)
    {
        for (std::uint32_t j = 0; j < rhs.size(); ++j)
        {
            if (lhs[i] == rhs[j])
            {
                dp[i][j] = (i == 0 || j == 0) ? 1 : (dp[i - 1][j - 1] + 1);
                if (dp[i][j] > best)
                {
                    best = dp[i][j];
                    best_pos = i + 1;
                }
            }
        }
    }
    // the best position marks the end of the string
    return lhs.substr(best_pos - best, best);
}

inline auto decompose(const std::string &lhs, const std::string &rhs)
{
    auto const lcs = longest_common_substring(lhs, rhs);

    // trim spaces, transform to lower
    const auto trim = [](auto str) {
        boost::to_lower(str);
        auto front = str.find_first_not_of(" ");

        if (front == std::string::npos)
            return str;

        auto back = str.find_last_not_of(" ");
        return str.substr(front, back - front + 1);
    };

    if (lcs.empty())
    {
        std::string empty = "";
        return std::make_tuple(trim(lhs), trim(rhs), empty, empty);
    }

    // find the common substring in both
    auto lhs_pos = lhs.find(lcs);
    auto rhs_pos = rhs.find(lcs);

    BOOST_ASSERT(lhs_pos + lcs.size() <= lhs.size());
    BOOST_ASSERT(rhs_pos + lcs.size() <= rhs.size());

    // prefixes
    std::string lhs_prefix = (lhs_pos > 0) ? lhs.substr(0, lhs_pos) : "";
    std::string rhs_prefix = (rhs_pos > 0) ? rhs.substr(0, rhs_pos) : "";

    // suffices
    std::string lhs_suffix = lhs.substr(lhs_pos + lcs.size());
    std::string rhs_suffix = rhs.substr(rhs_pos + lcs.size());

    lhs_prefix = trim(std::move(lhs_prefix));
    lhs_suffix = trim(std::move(lhs_suffix));
    rhs_prefix = trim(std::move(rhs_prefix));
    rhs_suffix = trim(std::move(rhs_suffix));

    return std::make_tuple(lhs_prefix, lhs_suffix, rhs_prefix, rhs_suffix);
}

// Note: there is an overload without suffix checking below.
// (that's the reason we template the suffix table here)
template <typename SuffixTable>
inline bool requiresNameAnnounced(const std::string &from_name,
                                  const std::string &from_ref,
                                  const std::string &from_pronunciation,
                                  const std::string &from_exits,
                                  const std::string &to_name,
                                  const std::string &to_ref,
                                  const std::string &to_pronunciation,
                                  const std::string &to_exits,
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

    const auto checkForPrefixOrSuffixChange =
        [](const std::string &first, const std::string &second, const SuffixTable &suffix_table) {
            std::string first_prefix, first_suffix, second_prefix, second_suffix;
            std::tie(first_prefix, first_suffix, second_prefix, second_suffix) =
                decompose(first, second);

            const auto checkTable = [&](const std::string &str) {
                // workaround for cucumber tests:
                if (str.length() == 1 && (first.length() == 2 || second.length() == 2))
                    return false;

                return str.empty() || suffix_table.isSuffix(str);
            };

            return checkTable(first_prefix) && checkTable(first_suffix) &&
                   checkTable(second_prefix) && checkTable(second_suffix);
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
        // " (Ref)" -> "Name " and reverse
        (from_name.empty() && !from_ref.empty() && !to_name.empty() && to_ref.empty()) ||
        (!from_name.empty() && from_ref.empty() && to_name.empty() && !to_ref.empty());

    const auto pronunciation_changes = from_pronunciation != to_pronunciation;

    // when exiting onto ramps, we need to be careful about exit numbers. These can often be only
    // assigned to the first part of the ramp
    //
    //  a . . b . c . . d
    //         ` e . . f
    //
    // could assign the exit number to `be` when exiting `abcd` instead of the full ramp.
    //
    // Issuing a new-name instruction here would result in the turn assuming the short segment to be
    // irrelevant and remove the exit number in a collapse scenario. We don't want to issue any
    // instruction from be-ef, since we only loose the exit number. So we want to make sure that we
    // don't just loose an exit number, when exits change
    const auto exits_change = from_exits != to_exits;
    const auto looses_exit = (names_are_equal && !from_exits.empty() && to_exits.empty());

    return !obvious_change || needs_announce || pronunciation_changes ||
           (exits_change && !looses_exit);
}

// Overload without suffix checking
inline bool requiresNameAnnounced(const std::string &from_name,
                                  const std::string &from_ref,
                                  const std::string &from_pronunciation,
                                  const std::string &from_exits,
                                  const std::string &to_name,
                                  const std::string &to_ref,
                                  const std::string &to_pronunciation,
                                  const std::string &to_exits)
{
    // Dummy since we need to provide a SuffixTable but do not have the data for it.
    // (Guidance Post-Processing does not keep the suffix table around at the moment)
    struct NopSuffixTable final
    {
        NopSuffixTable() {}
        bool isSuffix(const std::string &) const { return false; }
    } static const table;

    return requiresNameAnnounced(from_name,
                                 from_ref,
                                 from_pronunciation,
                                 from_exits,
                                 to_name,
                                 to_ref,
                                 to_pronunciation,
                                 to_exits,
                                 table);
}

inline bool requiresNameAnnounced(const NameID from_name_id,
                                  const NameID to_name_id,
                                  const util::NameTable &name_table,
                                  const extractor::SuffixTable &suffix_table)
{
    if (from_name_id == to_name_id)
        return false;
    else
        return requiresNameAnnounced(name_table.GetNameForID(from_name_id).to_string(),
                                     name_table.GetRefForID(from_name_id).to_string(),
                                     name_table.GetPronunciationForID(from_name_id).to_string(),
                                     name_table.GetExitsForID(from_name_id).to_string(),
                                     //
                                     name_table.GetNameForID(to_name_id).to_string(),
                                     name_table.GetRefForID(to_name_id).to_string(),
                                     name_table.GetPronunciationForID(to_name_id).to_string(),
                                     name_table.GetExitsForID(to_name_id).to_string(),
                                     //
                                     suffix_table);
    // FIXME: converts StringViews to strings since the name change heuristics mutates in place
}

inline bool requiresNameAnnounced(const NameID from_name_id,
                                  const NameID to_name_id,
                                  const util::NameTable &name_table)
{
    if (from_name_id == to_name_id)
        return false;
    else
        return requiresNameAnnounced(name_table.GetNameForID(from_name_id).to_string(),
                                     name_table.GetRefForID(from_name_id).to_string(),
                                     name_table.GetPronunciationForID(from_name_id).to_string(),
                                     name_table.GetExitsForID(from_name_id).to_string(),
                                     //
                                     name_table.GetNameForID(to_name_id).to_string(),
                                     name_table.GetRefForID(to_name_id).to_string(),
                                     name_table.GetExitsForID(to_name_id).to_string(),
                                     name_table.GetPronunciationForID(to_name_id).to_string());
    // FIXME: converts StringViews to strings since the name change heuristics mutates in place
}

} // namespace guidance
} // namespace util
} // namespace osrm

#endif /* OSRM_UTIL_GUIDANCE_NAME_ANNOUNCEMENT_HPP_ */
