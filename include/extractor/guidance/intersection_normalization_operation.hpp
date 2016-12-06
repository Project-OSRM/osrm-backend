#ifndef OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_NORMALIZATION_OPERATION_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_NORMALIZATION_OPERATION_HPP_

#include "util/typedefs.hpp"

namespace osrm
{
namespace extractor
{
namespace guidance
{

struct IntersectionNormalizationOperation
{
    // the source of the merge, not part of the intersection after the merge is performed.
    EdgeID merged_eid;
    // the edge that is covering the `merged_eid`
    EdgeID into_eid;
};

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /*OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_NORMALIZATION_OPERATION_HPP_*/
