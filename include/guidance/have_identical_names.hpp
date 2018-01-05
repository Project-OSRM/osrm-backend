#ifndef OSRM_EXTRACTOR_GUIDANCE_HAVE_IDENTICAL_NAMES_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_HAVE_IDENTICAL_NAMES_HPP_

#include "guidance/constants.hpp"
#include "extractor/suffix_table.hpp"
#include "util/name_table.hpp"

namespace osrm
{
namespace extractor
{
namespace guidance
{

// check if two name ids can be seen as identical (in presence of refs/others)
// in our case this translates into no name announcement in either direction (lhs->rhs and
// rhs->lhs)
bool HaveIdenticalNames(const NameID lhs,
                        const NameID rhs,
                        const util::NameTable &name_table,
                        const SuffixTable &street_name_suffix_table);

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /*OSRM_EXTRACTOR_GUIDANCE_HAVE_IDENTICAL_NAMES_HPP_*/
