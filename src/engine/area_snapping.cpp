#include "engine/area_snapping.hpp"

#include "engine/area_visibility.hpp"

#include "util/coordinate_calculation.hpp"

#include <algorithm>
#include <cmath>
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

/** Nothing is visible from further than this, so do not look. */
constexpr double VERTEX_LOOKUP_RADIUS_METRES = 1.0;

/**
 * How many phantoms to ask for at a vertex.  Several ways meet at a plaza vertex and we
 * want the ones that *leave* it, so one is not enough.
 */
constexpr std::size_t VERTEX_LOOKUP_RESULTS = 8;

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
 * The cost of the walk goes into that direction's weight offset.
 * `GetForwardWeightPlusOffset()` is the cost from the start of the edge-based node to the
 * phantom, and the search seeds a source with its negation but a target with the value
 * itself -- so the same walk has to be subtracted in one case and added in the other,
 * which is what @p role decides.  The result accounting never reads the offsets;
 * `assembleLeg` works from `forward_weight` and the per-segment values.  So this charges
 * the search and nothing else.  The types are signed, and a departure going below zero is
 * exactly what is meant: the journey begins before the graph does.
 */
PhantomNode at_vertex(PhantomNode phantom,
                      const bool forward,
                      const util::Coordinate from,
                      const util::Coordinate vertex,
                      const double walking_speed,
                      const double weight_multiplier,
                      const ApproachRole role)
{
    const auto metres = util::coordinate_calculation::greatCircleDistance(from, vertex);
    const auto seconds = metres / walking_speed;

    // durations are stored in deci-seconds, weights in the profile's own unit.
    // A via point is charged nothing: see ApproachRole.
    const auto sign = role == ApproachRole::Via ? 0 : (role == ApproachRole::Departure ? -1 : 1);
    const auto duration = to_alias<EdgeDuration>(sign * std::lround(seconds * 10.));
    const auto weight = to_alias<EdgeWeight>(sign * std::lround(seconds * weight_multiplier));

    if (forward)
    {
        phantom.forward_weight_offset = phantom.forward_weight_offset + weight;
        phantom.forward_duration_offset = phantom.forward_duration_offset + duration;
        phantom.reverse_segment_id.enabled = false;
    }
    else
    {
        phantom.reverse_weight_offset = phantom.reverse_weight_offset + weight;
        phantom.reverse_duration_offset = phantom.reverse_duration_offset + duration;
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
            for (const auto &found : facade.NearestPhantomNodes(vertex,
                                                                VERTEX_LOOKUP_RESULTS,
                                                                VERTEX_LOOKUP_RADIUS_METRES,
                                                                std::nullopt,
                                                                approach))
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
                // A via point takes the departure convention.  It is both ends at once
                // and something has to give; see ApproachRole.
                const bool arriving = role == ApproachRole::Arrival;
                const auto forward_usable =
                    arriving ? phantom.IsValidForwardTarget() : phantom.IsValidForwardSource();
                const auto reverse_usable =
                    arriving ? phantom.IsValidReverseTarget() : phantom.IsValidReverseSource();
                const auto forward_at_the_vertex =
                    (arriving ? phantom.reverse_weight : phantom.forward_weight) == EdgeWeight{0};
                const auto reverse_at_the_vertex =
                    (arriving ? phantom.forward_weight : phantom.reverse_weight) == EdgeWeight{0};

                if (forward_usable && forward_at_the_vertex && claim(phantom.forward_segment_id.id))
                {
                    candidates.push_back(at_vertex(phantom,
                                                   true,
                                                   coordinate,
                                                   vertex,
                                                   area.walking_speed,
                                                   weight_multiplier,
                                                   role));
                }
                if (reverse_usable && reverse_at_the_vertex && claim(phantom.reverse_segment_id.id))
                {
                    candidates.push_back(at_vertex(phantom,
                                                   false,
                                                   coordinate,
                                                   vertex,
                                                   area.walking_speed,
                                                   weight_multiplier,
                                                   role));
                }
            }
        }

        if (candidates.empty())
        {
            return std::nullopt;
        }
        return candidates;
    }

    return std::nullopt;
}

} // namespace osrm::engine::area
