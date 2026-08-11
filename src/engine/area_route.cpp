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

//! Used only when an area cannot be found to ask; every meshed area carries its own.
constexpr double DEFAULT_WALKING_SPEED = 1.4;

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

/** Say that a waypoint is where it was asked for, when the caller is tracking waypoints. */
void report(const std::vector<PhantomNodeCandidates *> &waypoints,
            std::size_t which,
            const util::Coordinate where)
{
    if (which < waypoints.size() && waypoints[which] != nullptr && !waypoints[which]->empty())
    {
        waypoints[which]->front().location = where;
    }
}

} // namespace

void useGeodesicWhereShorter(const datafacade::BaseDataFacade &facade,
                             InternalRouteResult &route,
                             const std::vector<PhantomNodeCandidates *> &waypoints)
{
    if (!route.is_valid())
    {
        return;
    }

    const auto multiplier = facade.GetWeightMultiplier();
    bool replaced = false;
    for (std::size_t leg = 0; leg < route.leg_endpoints.size(); ++leg)
    {
        // where the traveller asked to be, which is what the phantoms carry
        const auto from = route.leg_endpoints[leg].source_phantom.input_location;
        const auto to = route.leg_endpoints[leg].target_phantom.input_location;
        if (!from.IsValid() || !to.IsValid())
        {
            continue;
        }
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
        report(waypoints, leg, from);
        report(waypoints, leg + 1, to);
        replaced = true;
    }

    // A leg the geodesic did not take over may still begin or end inside an area,
    // snapped to a vertex some way off (engine/area_snapping.hpp).  That walk is a leg of
    // the journey and not a snapping error, so it belongs in the geometry -- otherwise
    // the route is drawn from a point the traveller never asked about, and its distance
    // leaves out a stretch they can see on the map.
    //
    // The vertex has to become a point of the path and not merely a moved endpoint: the
    // walk and the ways beyond it run in different directions, and a line straight from
    // the request to the far end would cut across whatever lies between.
    for (std::size_t leg = 0; leg < route.leg_endpoints.size(); ++leg)
    {
        auto &endpoints = route.leg_endpoints[leg];
        auto &path = route.unpacked_path_segments[leg];

        const auto walked = [&](const PhantomNode &phantom)
        {
            return phantom.input_location.IsValid() && phantom.input_location != phantom.location &&
                   !facade.GetOpenAreasAt(phantom.input_location).empty();
        };
        const auto cost = [&](const util::Coordinate a, const util::Coordinate b)
        {
            const auto areas = facade.GetOpenAreasAt(a);
            const auto speed = areas.empty() ? DEFAULT_WALKING_SPEED : areas.front().walking_speed;
            const auto stretch = metres(a, b) / speed;
            return std::pair{to_alias<EdgeWeight>(std::lround(stretch * multiplier)),
                             to_alias<EdgeDuration>(std::lround(stretch * 10.))};
        };
        const auto named = [](const PhantomNode &phantom)
        {
            return phantom.forward_segment_id.id != SPECIAL_SEGMENTID
                       ? phantom.forward_segment_id.id
                       : phantom.reverse_segment_id.id;
        };

        if (walked(endpoints.source_phantom))
        {
            if (const auto vertex = nodeAt(facade, endpoints.source_phantom.location))
            {
                const auto [weight, duration] = cost(endpoints.source_phantom.input_location,
                                                     endpoints.source_phantom.location);
                path.insert(path.begin(),
                            PathData{named(endpoints.source_phantom),
                                     *vertex,
                                     weight,
                                     {0},
                                     duration,
                                     {0},
                                     0,
                                     std::nullopt});
                endpoints.source_phantom.location = endpoints.source_phantom.input_location;
                report(waypoints, leg, endpoints.source_phantom.input_location);
                replaced = true;
            }
        }

        if (walked(endpoints.target_phantom))
        {
            if (const auto vertex = nodeAt(facade, endpoints.target_phantom.location))
            {
                const auto reversed = route.target_traversed_in_reverse[leg];
                auto &target_weight = reversed ? endpoints.target_phantom.reverse_weight
                                               : endpoints.target_phantom.forward_weight;
                auto &target_duration = reversed ? endpoints.target_phantom.reverse_duration
                                                 : endpoints.target_phantom.forward_duration;
                // what the leg already charged for reaching the vertex moves onto the
                // path, and the phantom is left holding only the walk beyond it
                path.push_back(PathData{named(endpoints.target_phantom),
                                        *vertex,
                                        target_weight,
                                        {0},
                                        target_duration,
                                        {0},
                                        0,
                                        std::nullopt});
                const auto [weight, duration] = cost(endpoints.target_phantom.input_location,
                                                     endpoints.target_phantom.location);
                target_weight = weight;
                target_duration = duration;
                endpoints.target_phantom.location = endpoints.target_phantom.input_location;
                report(waypoints, leg + 1, endpoints.target_phantom.input_location);
                replaced = true;
            }
        }
    }

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

void useGeodesicInTable(const datafacade::BaseDataFacade &facade,
                        const std::vector<util::Coordinate> &coordinates,
                        const std::vector<std::size_t> &sources,
                        const std::vector<std::size_t> &destinations,
                        std::vector<EdgeDuration> &durations,
                        std::vector<EdgeDistance> &distances)
{
    const auto rows = sources.empty() ? coordinates.size() : sources.size();
    const auto columns = destinations.empty() ? coordinates.size() : destinations.size();
    if (durations.size() != rows * columns)
    {
        return;
    }

    for (std::size_t row = 0; row < rows; ++row)
    {
        const auto from = coordinates[sources.empty() ? row : sources[row]];
        // one lookup for the whole row: a table over one plaza asks about the same area
        // again and again
        if (facade.GetOpenAreasAt(from).empty())
        {
            continue;
        }

        for (std::size_t column = 0; column < columns; ++column)
        {
            const auto to = coordinates[destinations.empty() ? column : destinations[column]];
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

            const auto cell = row * columns + column;
            durations[cell] =
                to_alias<EdgeDuration>(std::lround(geodesic->length / area->walking_speed * 10.));
            if (!distances.empty())
            {
                distances[cell] = to_alias<EdgeDistance>(geodesic->length);
            }
        }
    }
}

} // namespace osrm::engine::area
