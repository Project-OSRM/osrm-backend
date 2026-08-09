#include "engine/area_route.hpp"

#include "engine/area_geodesic.hpp"

#include "util/coordinate_calculation.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

namespace osrm::engine::area
{

namespace
{

//! A bend has to be recognised by its coordinate, so allow for the fixed-point grid.
constexpr double SAME_PLACE_METRES = 0.5;

//! Nothing is worth replacing for less than this, and rounding alone can produce it.
constexpr double WORTH_REPLACING_METRES = 1.0;

double metres(const util::Coordinate a, const util::Coordinate b)
{ return util::coordinate_calculation::greatCircleDistance(a, b); }

/** The one area holding both coordinates, if there is one. */
std::optional<extractor::AreaPolygonSegment> commonArea(const datafacade::BaseDataFacade &facade,
                                                        const util::Coordinate from,
                                                        const util::Coordinate to)
{
    auto here = facade.GetOpenAreasAt(from);
    if (here.empty())
    {
        return std::nullopt;
    }
    const auto there = facade.GetOpenAreasAt(to);
    if (there.empty())
    {
        return std::nullopt;
    }

    // the r-tree hands them back in packing order, which is not a property of the input
    std::sort(here.begin(),
              here.end(),
              [](const auto &lhs, const auto &rhs)
              { return lhs.vertices_offset < rhs.vertices_offset; });

    for (const auto &area : here)
    {
        const auto shared = std::any_of(there.begin(),
                                        there.end(),
                                        [&](const auto &other)
                                        { return other.vertices_offset == area.vertices_offset; });
        if (!shared)
        {
            continue;
        }
        // a bounding box is not the area; geodesic_between checks containment properly
        return area;
    }
    return std::nullopt;
}

/**
 * @brief The graph node standing at a bend of the geodesic.
 *
 * The bends are vertices of the area, and every vertex of a meshed area carries a way, so
 * each is a node of the graph -- but the engine is given the area's geometry as
 * coordinates, not as node ids.  This asks the r-tree what is there, then picks the node
 * of that segment which stands on the vertex.
 */
std::optional<NodeID> nodeAt(const datafacade::BaseDataFacade &facade, const util::Coordinate bend)
{
    // Asking for one is not enough: a chord of the mesh can pass straight through a
    // vertex without ending there, and it snaps just as close as the ways that do meet
    // it.  Take a handful and keep the first that actually has a node standing here.
    for (const auto &found :
         facade.NearestPhantomNodes(bend, 8, 1.0, std::nullopt, Approach::UNRESTRICTED))
    {
        const auto &phantom = found.phantom_node;
        const auto segment = phantom.forward_segment_id.id != SPECIAL_SEGMENTID
                                 ? phantom.forward_segment_id.id
                                 : phantom.reverse_segment_id.id;
        if (segment == SPECIAL_SEGMENTID)
        {
            continue;
        }
        for (const auto node :
             facade.GetUncompressedForwardGeometry(facade.GetGeometryIndex(segment).id))
        {
            if (metres(facade.GetCoordinateOfNode(node), bend) < SAME_PLACE_METRES)
            {
                return node;
            }
        }
    }
    return std::nullopt;
}

/** The node assembleGeometry will name as the first of a leg starting at this phantom. */
NodeID startNode(const datafacade::BaseDataFacade &facade, const PhantomNode &phantom)
{
    const auto segment = phantom.forward_segment_id.id;
    if (segment == SPECIAL_SEGMENTID)
    {
        return SPECIAL_NODEID;
    }
    const auto geometry =
        facade.GetUncompressedForwardGeometry(facade.GetGeometryIndex(segment).id);
    if (phantom.fwd_segment_position >= geometry.size())
    {
        return SPECIAL_NODEID;
    }
    return geometry[phantom.fwd_segment_position];
}

} // namespace

void useGeodesicWhereShorter(const datafacade::BaseDataFacade &facade,
                             const std::vector<util::Coordinate> &coordinates,
                             InternalRouteResult &route,
                             std::vector<PhantomNodeCandidates> &waypoints)
{
    if (!route.is_valid() || route.leg_endpoints.size() + 1 != coordinates.size())
    {
        return;
    }

    const auto multiplier = facade.GetWeightMultiplier();
    bool replaced = false;
    for (std::size_t leg = 0; leg < route.leg_endpoints.size(); ++leg)
    {
        const auto from = coordinates[leg], to = coordinates[leg + 1];
        const auto area = commonArea(facade, from, to);
        if (!area)
        {
            continue;
        }

        const auto rings = facade.GetOpenAreaRings(*area);
        const auto geodesic =
            geodesic_between(facade.GetCheckSum(), area->vertices_offset, rings, from, to);
        if (!geodesic)
        {
            continue;
        }

        // No comparison against the routed leg: there is nothing to compare with.  The
        // leg runs between the two *snapped* points and leaves the walks to them out of
        // its distance, so it is not measuring the same journey -- and when both ends
        // snap to one vertex it reports nothing at all.  The geodesic is the shortest
        // path between the two coordinates through the area, and a route that leaves the
        // area to come back could only beat it on ways faster than the area itself,
        // which for a profile that meshes plazas is not a case that arises.
        if (geodesic->length < WORTH_REPLACING_METRES)
        {
            continue;
        }

        // Every bend has to resolve to a node, or the geometry cannot be written down.
        std::vector<NodeID> bends;
        bends.reserve(geodesic->bends.size());
        for (const auto bend : geodesic->bends)
        {
            const auto node = nodeAt(facade, bend);
            if (!node)
            {
                break;
            }
            bends.push_back(*node);
        }
        if (bends.size() != geodesic->bends.size())
        {
            continue;
        }

        // The path is a straight run from the request through each bend to the other
        // request, so the leg is that sequence and the phantoms stand at its two ends.
        auto &endpoints = route.leg_endpoints[leg];
        endpoints.source_phantom.location = from;
        endpoints.target_phantom.location = to;
        route.source_traversed_in_reverse[leg] = false;
        route.target_traversed_in_reverse[leg] = false;

        const auto cost = [&](double length)
        {
            const auto stretch = length / area->walking_speed;
            return std::pair{to_alias<EdgeWeight>(std::lround(stretch * multiplier)),
                             to_alias<EdgeDuration>(std::lround(stretch * 10.))};
        };

        // `from_edge_based_node` is fed to GetNameIndex, so it has to be an edge-based
        // node; the plaza's own way names every stretch of this leg
        const auto named = endpoints.source_phantom.forward_segment_id.id != SPECIAL_SEGMENTID
                               ? endpoints.source_phantom.forward_segment_id.id
                               : endpoints.source_phantom.reverse_segment_id.id;

        auto &path = route.unpacked_path_segments[leg];
        path.clear();
        path.reserve(bends.size());
        auto previous = from;
        for (std::size_t i = 0; i < bends.size(); ++i)
        {
            const auto [step_weight, step_duration] = cost(metres(previous, geodesic->bends[i]));
            path.push_back(
                PathData{named, bends[i], step_weight, {0}, step_duration, {0}, 0, std::nullopt});
            previous = geodesic->bends[i];
        }

        // assembleGeometry names the first node of a leg from the source phantom's own
        // segment and drops any path point repeating it.  For a leg beginning at a free
        // point that name is arbitrary, and when it lands on the first bend the bend is
        // dropped and the drawn line cuts the corner it was there to turn.  Stand the
        // phantom on a different segment nearby; it supplies only a name and a node id.
        if (!bends.empty() && startNode(facade, endpoints.source_phantom) == bends.front())
        {
            for (const auto &nearby :
                 facade.NearestPhantomNodes(from, 8, 400.0, std::nullopt, Approach::UNRESTRICTED))
            {
                const auto &candidate = nearby.phantom_node;
                if (candidate.forward_segment_id.id != SPECIAL_SEGMENTID &&
                    startNode(facade, candidate) != bends.front())
                {
                    endpoints.source_phantom.forward_segment_id = candidate.forward_segment_id;
                    endpoints.source_phantom.reverse_segment_id = candidate.reverse_segment_id;
                    endpoints.source_phantom.fwd_segment_position = candidate.fwd_segment_position;
                    break;
                }
            }
        }

        // assembleLeg adds the target phantom's own weight on top of the path, and
        // subtracts the source's when the path is empty, so the last stretch goes there
        const auto [last_weight, last_duration] = cost(metres(previous, to));
        endpoints.source_phantom.forward_weight = {0};
        endpoints.source_phantom.forward_duration = {0};
        endpoints.source_phantom.forward_distance = {0};
        endpoints.target_phantom.forward_weight = last_weight;
        endpoints.target_phantom.forward_duration = last_duration;
        endpoints.target_phantom.forward_distance = to_alias<EdgeDistance>(metres(previous, to));

        // The journey now starts and ends where it was asked to, so that is what the
        // waypoints report -- and the walk to a snapped vertex, which is no longer part
        // of the story, stops being reported as a snapping error.
        if (leg < waypoints.size() && !waypoints[leg].empty())
        {
            waypoints[leg].front().location = from;
        }
        if (leg + 1 < waypoints.size() && !waypoints[leg + 1].empty())
        {
            waypoints[leg + 1].front().location = to;
        }
        replaced = true;
    }

    // TODO: a leg that merely *starts* or *ends* inside an area is still drawn from the
    // vertex it snapped to, so the walk across the area -- a real leg of the journey, and
    // sometimes hundreds of metres of it -- is missing from the geometry and the distance.
    // Moving the endpoint is not enough: the vertex has to become a point of the path, or
    // a line straight from the request to the far end cuts whatever lies between.  Adding
    // it as a PathData works only when assembleGeometry does not then discard it, which it
    // does whenever the source phantom's own segment happens to name that same node -- and
    // for the mesh's two-node ways there is often no other segment nearby that names
    // something else.  The fix belongs in assembleGeometry, which should not be comparing
    // the first point of a leg against a node the leg does not start at.

    if (!replaced)
    {
        return;
    }

    // the weight the search reported no longer describes these legs
    route.shortest_path_weight = EdgeWeight{0};
    for (std::size_t leg = 0; leg < route.leg_endpoints.size(); ++leg)
    {
        for (const auto &step : route.unpacked_path_segments[leg])
        {
            route.shortest_path_weight = route.shortest_path_weight + step.weight_until_turn;
        }
        route.shortest_path_weight =
            route.shortest_path_weight + route.leg_endpoints[leg].target_phantom.forward_weight;
    }
}

} // namespace osrm::engine::area
