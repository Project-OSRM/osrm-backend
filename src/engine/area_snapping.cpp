#include "engine/area_snapping.hpp"

#include "engine/area_visibility.hpp"

#include "util/coordinate_calculation.hpp"

#include <boost/numeric/conversion/cast.hpp>

#include <algorithm>
#include <cmath>
#include <span>
#include <vector>

namespace osrm::engine::area
{

namespace
{

/**
 * A vertex is a graph node only if the mesher gave it an edge.  Rather than store which
 * ones did, we ask: the phantom found at a vertex sits on the vertex itself exactly when
 * the vertex is an endpoint of some way.  Anything further away is a different place.
 */
constexpr double SAME_PLACE_METRES = 0.5;

struct ProjectedRings
{
    std::vector<std::vector<Point>> storage;
    std::vector<Ring> views;
};

ProjectedRings project_rings(const std::vector<std::span<const util::Coordinate>> &rings)
{
    ProjectedRings projected;
    projected.storage.reserve(rings.size());
    for (const auto &ring : rings)
    {
        std::vector<Point> points;
        points.reserve(ring.size());
        for (const auto coordinate : ring)
        {
            points.push_back(project(coordinate));
        }
        projected.storage.push_back(std::move(points));
    }
    // only now that storage has stopped growing, so the spans stay valid
    projected.views.reserve(projected.storage.size());
    for (const auto &points : projected.storage)
    {
        projected.views.emplace_back(points);
    }
    return projected;
}

/** The flat vertex index addresses the rings in order, so walk them to find it. */
util::Coordinate vertex_at(const std::vector<std::span<const util::Coordinate>> &rings,
                           std::size_t index)
{
    for (const auto &ring : rings)
    {
        if (index < ring.size())
        {
            return ring[index];
        }
        index -= ring.size();
    }
    return util::Coordinate{};
}

/**
 * Turn a phantom found at a vertex into a candidate that only ever leaves that vertex, or
 * only ever arrives at it, in one direction, with the straight line from the request
 * charged to it.
 *
 * Restricting it to one direction is what makes the candidate identifiable.  OSRM matches
 * a finished path back to the candidate it started from by segment id
 * (`endpointsFromCandidates`), and considers both of a phantom's ids; the two ends of one
 * meshed way carry the same pair, so two visible vertices would be indistinguishable.
 * With the unused direction disabled, every candidate has exactly one live id, and no two
 * candidates share it -- an edge-based node has only one start and only one end.  The id
 * itself stays put: the API reads `forward_segment_id.id` unconditionally to name a
 * waypoint.
 *
 * The cost of the walk goes on the phantom itself, not into the direction's offset.  A
 * search seeds a source with the negation of its offset and a target with the offset
 * itself, so an offset would have to carry both signs at once for a coordinate that is
 * both, which every coordinate of a table is and so is a via point.  Carried separately,
 * it is added in either role; see `PhantomNode::approach_weight`.  Leg assembly never
 * reads it, and does not need to: the walk is drawn into the leg as a stretch of its own
 * (engine/area_route.hpp).
 */
PhantomNode at_vertex(PhantomNode phantom,
                      const bool forward,
                      const util::Coordinate from,
                      const util::Coordinate vertex,
                      const double walking_speed,
                      const double weight_multiplier)
{
    const auto metres = util::coordinate_calculation::greatCircleDistance(from, vertex);
    const auto seconds = metres / walking_speed;

    // durations are stored in deci-seconds, weights in the profile's own unit
    phantom.approach_weight = to_alias<EdgeWeight>(std::lround(seconds * weight_multiplier));
    phantom.approach_duration = to_alias<EdgeDuration>(std::lround(seconds * 10.));
    phantom.approach_distance = to_alias<EdgeDistance>(metres);

    if (forward)
    {
        phantom.reverse_segment_id.enabled = false;
    }
    else
    {
        phantom.forward_segment_id.enabled = false;
    }

    // where the traveller actually asked to be
    phantom.input_location = from;
    return phantom;
}

} // namespace

std::optional<PhantomNodeCandidates> SnapInsideOpenArea(const datafacade::BaseDataFacade &facade,
                                                        const util::Coordinate coordinate,
                                                        const Approach approach,
                                                        const ApproachRole role)
{
    auto areas = facade.GetOpenAreasAt(coordinate);
    if (areas.empty())
    {
        return std::nullopt;
    }

    // the r-tree hands them back in packing order, which is not a property of the input
    std::sort(areas.begin(),
              areas.end(),
              [](const auto &lhs, const auto &rhs)
              { return lhs.vertices_offset < rhs.vertices_offset; });

    const auto point = project(coordinate);
    const auto weight_multiplier = facade.GetWeightMultiplier();

    for (const auto &area : areas)
    {
        const auto rings = facade.GetOpenAreaRings(area);
        if (rings.empty())
        {
            continue;
        }

        const auto projected = project_rings(rings);
        if (!inside_area(point, projected.views))
        {
            // the bounding box is not the area
            continue;
        }

        PhantomNodeCandidates candidates;

        std::vector<NodeID> claimed_nodes;
        const auto claim = [&claimed_nodes](const NodeID node)
        {
            if (std::find(claimed_nodes.begin(), claimed_nodes.end(), node) != claimed_nodes.end())
                return false;
            claimed_nodes.push_back(node);
            return true;
        };

        for (const auto index : visible_vertices(point, projected.views))
        {
            const auto vertex = vertex_at(rings, index);

            // Which edge-based nodes stand here is recorded at extraction, so this is a
            // lookup.  Asking the r-tree for the nearest few instead meant one traversal
            // per visible vertex -- most of the cost of snapping into an area -- and it
            // had to hand back things that merely pass close by for the tests below to
            // throw away again.
            for (const auto &found : facade.PhantomNodesOnAreaVertex(
                     area, boost::numeric_cast<std::uint32_t>(index), approach))
            {
                const auto &phantom = found.phantom_node;
                if (util::coordinate_calculation::greatCircleDistance(phantom.location, vertex) >
                    SAME_PLACE_METRES)
                {
                    // the mesher gave this vertex no edge, so it is not a place to set
                    // off from
                    continue;
                }

                // A direction a traveller departs by has to begin at the vertex, so that
                // travelling it leaves the vertex; standing at the far end of a way is
                // not a way of leaving by it.  A direction they arrive by has to end
                // there, since the journey stops on reaching the vertex.  One phantom
                // answers both, because sitting at a segment's first node means carrying
                // a weight of zero forwards and the whole of the segment in reverse.
                //
                // Getting this wrong does not merely cost accuracy.  A candidate that
                // stands at a node's start and is then used as a target sits *before* a
                // source that is further along the same node, and the journey between
                // them comes out short by the stretch in between.
                const bool arriving = role == ApproachRole::Arrival;
                {
                    const auto forward_usable =
                        arriving ? phantom.IsValidForwardTarget() : phantom.IsValidForwardSource();
                    const auto reverse_usable =
                        arriving ? phantom.IsValidReverseTarget() : phantom.IsValidReverseSource();
                    const auto forward_at_the_vertex =
                        (arriving ? phantom.reverse_weight : phantom.forward_weight) ==
                        EdgeWeight{0};
                    const auto reverse_at_the_vertex =
                        (arriving ? phantom.forward_weight : phantom.reverse_weight) ==
                        EdgeWeight{0};

                    if (forward_usable && forward_at_the_vertex &&
                        claim(phantom.forward_segment_id.id))
                    {
                        candidates.push_back(at_vertex(phantom,
                                                       true,
                                                       coordinate,
                                                       vertex,
                                                       area.walking_speed,
                                                       weight_multiplier));
                    }
                    if (reverse_usable && reverse_at_the_vertex &&
                        claim(phantom.reverse_segment_id.id))
                    {
                        candidates.push_back(at_vertex(phantom,
                                                       false,
                                                       coordinate,
                                                       vertex,
                                                       area.walking_speed,
                                                       weight_multiplier));
                    }
                }
            }
        }

        if (candidates.empty())
        {
            // Nothing here to set off from, which does not mean there is nowhere.  An
            // area the mesher declined -- AreaMesher::max_vertices -- is recorded all the
            // same, and no vertex of it carries a way, so it yields nothing however long
            // it is walked over.  Areas overlap, so another one may hold this coordinate
            // and be meshed.
            //
            // Returning here instead of looking sent the request back to ordinary
            // snapping, which projects onto the nearest edge of the perimeter, and the
            // route then leaves the plaza by a corner and comes back.  The `return` after
            // the loop is what answers "none of them had anything".
            continue;
        }
        return candidates;
    }

    return std::nullopt;
}

} // namespace osrm::engine::area
