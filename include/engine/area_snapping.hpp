#ifndef OSRM_ENGINE_AREA_SNAPPING_HPP
#define OSRM_ENGINE_AREA_SNAPPING_HPP

#include "engine/approach.hpp"
#include "engine/datafacade/datafacade_base.hpp"
#include "engine/phantom_node.hpp"

#include "util/coordinate.hpp"

#include <optional>

namespace osrm::engine::area
{

/**
 * @brief Which end of the journey a coordinate is, which decides the sign of the walk.
 *
 * The search seeds a source with the negation of `GetForwardWeightPlusOffset()` and a
 * target with the value itself, so the walk has to be subtracted from the offset in one
 * case and added in the other.  The stored offset can only hold one of the two, and the
 * plugin is where the role is known.
 */
/**
 * Which end of a journey a coordinate is, which decides how the walk into the area is
 * charged.  A `Via` point is both ends at once and so cannot be charged at all: the
 * search seeds a source with the negation of the offset and a target with the offset
 * itself, and one number cannot come out positive both ways.  Charging it as a departure
 * makes the leg that arrives there seed a negative key, which CH does not permit.
 */
enum class ApproachRole
{
    Departure,
    Arrival,
    Via
};

/**
 * @brief Snap a coordinate that falls inside a meshed open area.
 *
 * Ordinary snapping projects onto the nearest segment, which inside a plaza means the
 * nearest meshed chord -- and the route then sets off towards wherever that chord goes,
 * however little sense it makes from where the request actually was.
 *
 * Inside an area we do something else: we draw a straight line to every vertex of the
 * area that the coordinate can see, and offer each of those vertices as a candidate, at a
 * cost equal to walking the straight line.  The search relaxes all of them and picks the
 * one that actually leads somewhere, which is the same answer the visibility graph would
 * have given if the coordinate had been part of it when it was built.  See
 * plans/open-area-snapping.md, sections R11 and R12.
 *
 * @return the candidates, or nothing if the coordinate is not inside any open area, or
 *         is inside one whose vertices are all unreachable.
 */
std::optional<PhantomNodeCandidates> SnapInsideOpenArea(const datafacade::BaseDataFacade &facade,
                                                        const util::Coordinate coordinate,
                                                        const Approach approach,
                                                        const ApproachRole role);

} // namespace osrm::engine::area

#endif // OSRM_ENGINE_AREA_SNAPPING_HPP
