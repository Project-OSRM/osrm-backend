#include "engine/area_route.hpp"

#include "engine/area_fillet.hpp"
#include "engine/area_geodesic.hpp"

#include "util/coordinate_calculation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
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

/** Every area holding both coordinates, in an order that is a property of the data. */
std::vector<extractor::AreaPolygonSegment> commonAreas(const datafacade::BaseDataFacade &facade,
                                                       const util::Coordinate from,
                                                       const util::Coordinate to)
{
    auto here = facade.GetOpenAreasAt(from);
    if (here.empty())
    {
        return {};
    }
    const auto there = facade.GetOpenAreasAt(to);
    if (there.empty())
    {
        return {};
    }

    // the r-tree hands them back in packing order, which is not a property of the input
    std::sort(here.begin(),
              here.end(),
              [](const auto &lhs, const auto &rhs)
              { return lhs.vertices_offset < rhs.vertices_offset; });

    std::vector<extractor::AreaPolygonSegment> shared;
    for (const auto &area : here)
    {
        if (std::any_of(there.begin(),
                        there.end(),
                        [&](const auto &other)
                        { return other.vertices_offset == area.vertices_offset; }))
        {
            // a bounding box is not the area; geodesic_between checks containment properly
            shared.push_back(area);
        }
    }
    return shared;
}

/**
 * @brief The geodesic between two coordinates, across whichever shared area holds them.
 *
 * The r-tree answers with bounding boxes, and a bounding box is not the area, so a
 * coordinate can be filed under an area it is not inside.  geodesic_between settles that
 * properly and declines when it does not hold.  Taking only the first shared area
 * therefore threw the journey away whenever the first one was a near miss, even though a
 * later one answered it perfectly well.
 *
 * The area is returned with the geodesic because the caller needs its walking speed.
 */
std::optional<std::pair<extractor::AreaPolygonSegment, Geodesic>>
geodesicAcross(const datafacade::BaseDataFacade &facade,
               const util::Coordinate from,
               const util::Coordinate to)
{
    for (const auto &area : commonAreas(facade, from, to))
    {
        const auto rings = facade.GetOpenAreaRings(area);
        const auto stored = [&](const std::uint32_t vertex)
        { return facade.GetOpenAreaVisibility(area, vertex); };
        auto geodesic = geodesic_between(
            facade.GetCheckSum(), area.vertices_offset, rings, from, to, StoredVisibility{stored});
        if (geodesic)
        {
            return std::pair{area, std::move(*geodesic)};
        }
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

/**
 * @brief One interior point of a leg drawn across an area: where it is, and which node it
 * is booked to.
 *
 * A bend of the taut path *is* a node, so it needs no coordinate of its own.  A point on
 * an arc is not on any node, so it carries its position in PathData::coordinate
 * and is booked to the node of the nearest bend, because the annotations want a node for
 * every point and that is the honest one: it is the corner the point is rounding.
 */
struct Stop
{
    util::Coordinate at;
    NodeID node;
    bool computed;
};

/**
 * @brief The shape a leg is drawn as: the taut bends, or those bends rounded into arcs.
 *
 * With no margin, or when no rounding is legal, the bends are the shape and nothing about
 * the leg changes.  With a margin the taut path is handed to round_corners(); what comes
 * back is drawn instead, every interior point computed.
 *
 * A path with no bends still has to book its points somewhere, and the only node in the
 * story is the vertex the traveller's coordinate snapped to.  When that cannot be found
 * the straight line is kept, since a point that cannot be booked cannot be written down.
 */
std::vector<Stop> shapeOf(const datafacade::BaseDataFacade &facade,
                          const extractor::AreaPolygonSegment &area,
                          const util::Coordinate from,
                          const Geodesic &geodesic,
                          const std::vector<NodeID> &bends,
                          const util::Coordinate to,
                          const std::optional<NodeID> entered)
{
    std::vector<Stop> taut;
    taut.reserve(bends.size());
    for (std::size_t i = 0; i < bends.size(); ++i)
    {
        taut.push_back({geodesic.bends[i], bends[i], false});
    }

    const auto margin = facade.GetAreaSmoothingMargin();
    if (!(margin > 0.0) || (bends.empty() && !entered))
    {
        return taut;
    }

    std::vector<util::Coordinate> path;
    path.reserve(geodesic.bends.size() + 2);
    path.push_back(from);
    path.insert(path.end(), geodesic.bends.begin(), geodesic.bends.end());
    path.push_back(to);

    const auto smoothed = round_corners(facade.GetOpenAreaRings(area), path, margin);
    if (!smoothed)
    {
        return taut;
    }

    std::vector<Stop> drawn;
    drawn.reserve(smoothed->size() - 2);
    for (std::size_t i = 1; i + 1 < smoothed->size(); ++i)
    {
        const auto point = (*smoothed)[i];
        auto node = entered.value_or(SPECIAL_NODEID);
        auto nearest = std::numeric_limits<double>::infinity();
        for (std::size_t b = 0; b < bends.size(); ++b)
        {
            // strictly closer, so a tie goes to the earlier bend and the booking is a
            // property of the geometry rather than of summation order
            const auto distance = metres(point, geodesic.bends[b]);
            if (distance < nearest)
            {
                nearest = distance;
                node = bends[b];
            }
        }
        drawn.push_back({point, node, true});
    }
    return drawn;
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
        const auto answer = geodesicAcross(facade, from, to);
        if (!answer)
        {
            continue;
        }
        const auto &area = answer->first;
        const auto &geodesic = answer->second;

        // No comparison against the routed leg: there is nothing to compare with.  The
        // leg runs between the two *snapped* points and leaves the walks to them out of
        // its distance, so it is not measuring the same journey -- and when both ends
        // snap to one vertex it reports nothing at all.  The geodesic is the shortest
        // path between the two coordinates through the area, and a route that leaves the
        // area to come back could only beat it on ways faster than the area itself,
        // which for a profile that meshes plazas is not a case that arises.
        if (geodesic.length < WORTH_REPLACING_METRES)
        {
            continue;
        }

        // Every bend has to resolve to a node, or the geometry cannot be written down.
        std::vector<NodeID> bends;
        bends.reserve(geodesic.bends.size());
        for (const auto bend : geodesic.bends)
        {
            const auto node = nodeAt(facade, bend);
            if (!node)
            {
                break;
            }
            bends.push_back(*node);
        }
        if (bends.size() != geodesic.bends.size())
        {
            continue;
        }

        auto &endpoints = route.leg_endpoints[leg];
        // the vertex the traveller's coordinate snapped to, which a smoothed straight
        // line books its points to; asked before the phantom is moved off it below, and
        // not asked at all unless smoothing could want it, so a leg with no margin costs
        // exactly what it did
        const auto entered = bends.empty() && facade.GetAreaSmoothingMargin() > 0.0
                                 ? nodeAt(facade, endpoints.source_phantom.location)
                                 : std::nullopt;
        const auto stops = shapeOf(facade, area, from, geodesic, bends, to, entered);

        // The path is a straight run from the request through each stop to the other
        // request, so the leg is that sequence and the phantoms stand at its two ends.
        endpoints.source_phantom.location = from;
        endpoints.target_phantom.location = to;
        route.source_traversed_in_reverse[leg] = false;
        route.target_traversed_in_reverse[leg] = false;

        const auto cost = [&](double length)
        {
            const auto stretch = length / area.walking_speed;
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
        path.reserve(stops.size());
        auto previous = from;
        for (const auto &stop : stops)
        {
            const auto [step_weight, step_duration] = cost(metres(previous, stop.at));
            // a computed point says where it is; a bend is found by its node, as before
            path.push_back(PathData{named,
                                    stop.node,
                                    step_weight,
                                    {0},
                                    step_duration,
                                    {0},
                                    0,
                                    std::nullopt,
                                    stop.computed ? stop.at : util::Coordinate{}});
            previous = stop.at;
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
            const auto answer = geodesicAcross(facade, from, to);
            if (!answer)
            {
                continue;
            }
            const auto &area = answer->first;
            const auto &geodesic = answer->second;

            const auto cell = row * columns + column;
            durations[cell] =
                to_alias<EdgeDuration>(std::lround(geodesic.length / area.walking_speed * 10.));
            if (!distances.empty())
            {
                distances[cell] = to_alias<EdgeDistance>(geodesic.length);
            }
        }
    }
}

} // namespace osrm::engine::area
