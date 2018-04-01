#ifndef OSRM_EXTRACTOR_INTERSECTION_HAVE_IDENTICAL_NAMES_HPP_
#define OSRM_EXTRACTOR_INTERSECTION_HAVE_IDENTICAL_NAMES_HPP_

#include "extractor/name_table.hpp"
#include "extractor/suffix_table.hpp"
#include "guidance/constants.hpp"

namespace osrm
{
namespace extractor
{
namespace intersection
{

// check if two name ids can be seen as identical (in presence of refs/others)
// in our case this translates into no name announcement in either direction (lhs->rhs and
// rhs->lhs)
bool HaveIdenticalNames(const NameID lhs,
                        const NameID rhs,
                        const NameTable &name_table,
                        const SuffixTable &street_name_suffix_table);

} // namespace intersection
} // namespace extractor
} // namespace osrm

#endif /*OSRM_EXTRACTOR_INTERSECTION_HAVE_IDENTICAL_NAMES_HPP_*/
