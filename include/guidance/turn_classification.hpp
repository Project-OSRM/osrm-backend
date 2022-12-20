#ifndef OSRM_GUIDANCE_TURN_CLASSIFICATION_HPP_
#define OSRM_GUIDANCE_TURN_CLASSIFICATION_HPP_

#include "guidance/intersection.hpp"

#include "util/coordinate.hpp"
#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"

#include <utility>

namespace osrm::guidance
{

std::pair<util::guidance::EntryClass, util::guidance::BearingClass>
classifyIntersection(Intersection intersection, const osrm::util::Coordinate &location);

} // namespace osrm::guidance

#endif // OSRM_GUIDANCE_TURN_CLASSIFICATION_HPP_
