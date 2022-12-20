#include "extractor/intersection/have_identical_names.hpp"

#include "util/guidance/name_announcements.hpp"

namespace osrm::extractor::intersection
{

// check if two name ids can be seen as identical (in presence of refs/others)
// in our case this translates into no name announcement in either direction (lhs->rhs and
// rhs->lhs)
bool HaveIdenticalNames(const NameID lhs,
                        const NameID rhs,
                        const NameTable &name_table,
                        const extractor::SuffixTable &street_name_suffix_table)
{
    const auto non_empty = (lhs != EMPTY_NAMEID) && (rhs != EMPTY_NAMEID);

    // symmetrical check for announcements
    return non_empty &&
           !util::guidance::requiresNameAnnounced(lhs, rhs, name_table, street_name_suffix_table) &&
           !util::guidance::requiresNameAnnounced(rhs, lhs, name_table, street_name_suffix_table);
}

} // namespace osrm::extractor::intersection
