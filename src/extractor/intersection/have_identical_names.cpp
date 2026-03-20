#include "extractor/intersection/have_identical_names.hpp"

#include "util/guidance/name_announcements.hpp"

namespace osrm::extractor::intersection
{

// check if two name ids can be seen as identical (in presence of refs/others)
// in our case this translates into no name announcement in either direction (lhs->rhs and
// rhs->lhs)
bool HaveIdenticalNames(const StringViewID lhs,
                        const StringViewID rhs,
                        const StringTable &string_table,
                        const extractor::SuffixTable &street_name_suffix_table)
{
    const auto non_empty = (lhs != EMPTY_STRINGVIEWID) && (rhs != EMPTY_STRINGVIEWID);

    // symmetrical check for announcements
    return non_empty &&
           !util::guidance::requiresNameAnnounced(lhs, rhs, string_table, street_name_suffix_table) &&
           !util::guidance::requiresNameAnnounced(rhs, lhs, string_table, street_name_suffix_table);
}

} // namespace osrm::extractor::intersection
