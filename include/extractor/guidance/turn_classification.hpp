#ifndef OSRM_GUIDANCE_TURN_CLASSIFICATION_HPP_
#define OSRM_GUIDANCE_TURN_CLASSIFICATION_HPP_

#include "extractor/guidance/intersection.hpp"

#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"

#include <utility>

namespace osrm
{
namespace extractor
{
namespace guidance
{

std::pair<util::guidance::EntryClass, util::guidance::BearingClass>
classifyIntersection(Intersection intersection);

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif // OSRM_GUIDANCE_TURN_CLASSIFICATION_HPP_
