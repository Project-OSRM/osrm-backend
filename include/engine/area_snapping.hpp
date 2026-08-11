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
 * @brief Which end of a journey a coordinate is.
 *
 * This decides where the candidate stands, not what it costs.  A candidate departed from
 * has to stand at the start of an edge-based node, so that travelling it leaves the
 * vertex; one arrived at has to stand at the node's end, or a leg contained in a single
 * segment ends before it begins and comes out with a negative weight.
 *
 * A via point is both at once and takes the departing shape.  What a coordinate costs
 * does not depend on its role at all; see `PhantomNode::approach_weight`.
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
