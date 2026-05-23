#include "engine/plugins/isochrone.hpp"
#include "engine/search_engine_data.hpp"
#include "engine/plugins/plugin_base.hpp"
#include "util/timing_util.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/log.hpp"

#include "extractor/edge_based_node.hpp"
#include "engine/datafacade/contiguous_internalmem_datafacade.hpp"
#include "engine/routing_algorithms.hpp"

#include <queue>
#include <iomanip>
#include <cmath>
#include <unordered_set>
#include <numeric>

namespace osrm
{
namespace engine
{
namespace plugins
{

IsochronePlugin::IsochronePlugin(int max_range_seconds_)
    : max_range_seconds(max_range_seconds_)
{
}

Status
IsochronePlugin::HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                               const api::IsochroneParameters &parameters,
                               osrm::engine::api::ResultT &result) const
{
    const auto range = parameters.range;
    util::Coordinate startcoord = parameters.coordinates.front();

    // Use profile-specific max speed when available (m/s). Fall back to conservative 90 km/h.
    const double profile_max_speed = algorithms.GetFacade().GetMapMatchingMaxSpeed();
    const double MAX_SPEED_METERS_PER_SECOND = (profile_max_speed > 0.0) ? profile_max_speed : (90.0 / 3.6);
    const double MAX_TRAVEL_DISTANCE_METERS = MAX_SPEED_METERS_PER_SECOND * static_cast<double>(range);

    const double latitude_range = static_cast<double>(
        MAX_TRAVEL_DISTANCE_METERS / (util::coordinate_calculation::detail::EARTH_RADIUS * M_PI * 2) * 360.0);

    const double longitude_range = static_cast<double>(
        360.0 * MAX_TRAVEL_DISTANCE_METERS /
        ((util::coordinate_calculation::detail::EARTH_RADIUS * M_PI * 2) *
         cos(static_cast<double>(toFloating(startcoord.lat)) * util::coordinate_calculation::detail::DEGREE_TO_RAD)));

    const double lon_f = static_cast<double>(toFloating(startcoord.lon));
    const double lat_f = static_cast<double>(toFloating(startcoord.lat));

    util::Coordinate southwest{
        util::FloatLongitude{static_cast<double>(lon_f - longitude_range)},
        util::FloatLatitude{static_cast<double>(lat_f - latitude_range)}};
    util::Coordinate northeast{
        util::FloatLongitude{static_cast<double>(lon_f + longitude_range)},
        util::FloatLatitude{static_cast<double>(lat_f + latitude_range)}};

    // Attempt to run an undirectional Dijkstra on the routing algorithm's base graph (CH/MLD)
    // to collect reachable edge-based nodes within the time range. This is faster than expanding
    // many geometry candidates from RTree when the underlying routing graph can be traversed.

    std::vector<std::pair<NodeID, EdgeDuration>> reachable_nodes;

    // Duration threshold: clamp request to configured max and convert seconds -> deciseconds.
    const unsigned configured_max_range =
        max_range_seconds > 0 ? static_cast<unsigned>(max_range_seconds) : 1;
    const unsigned effective_range = std::min(range, configured_max_range);
    const EdgeDuration duration_threshold = to_alias<EdgeDuration>(static_cast<int>(effective_range * 10));

    const auto &df_base = algorithms.GetFacade();
    const auto *mld_facade = dynamic_cast<const datafacade::ContiguousInternalMemoryDataFacade<routing_algorithms::mld::Algorithm>*>(&df_base);
    const auto *ch_facade = dynamic_cast<const datafacade::ContiguousInternalMemoryDataFacade<routing_algorithms::ch::Algorithm>*>(&df_base);

    if (mld_facade || ch_facade)
    {
        const unsigned num_nodes = (mld_facade) ? mld_facade->GetNumberOfNodes() : ch_facade->GetNumberOfNodes();

        std::vector<EdgeDuration> dist(num_nodes, MAXIMAL_EDGE_DURATION);
        struct PQItem { EdgeDuration d; NodeID node; };
        struct Cmp { bool operator()(const PQItem &a, const PQItem &b) const { return from_alias<int>(a.d) > from_alias<int>(b.d); } };
        std::priority_queue<PQItem, std::vector<PQItem>, Cmp> pq;

        // get start phantom
        auto startpoints = df_base.NearestPhantomNodes(startcoord,
                                                        1,
                                                        std::optional<double>{},
                                                        std::optional<engine::Bearing>{},
                                                        engine::Approach::UNRESTRICTED);
        if (startpoints.empty())
            return Status::Error;
        const auto &start_ph = startpoints.front().phantom_node;

        if (start_ph.forward_segment_id.enabled)
        {
            const NodeID s = start_ph.forward_segment_id.id;
            dist[s] = EdgeDuration{0};
            pq.push(PQItem{EdgeDuration{0}, s});
        }
        if (start_ph.reverse_segment_id.enabled)
        {
            const NodeID s = start_ph.reverse_segment_id.id;
            dist[s] = EdgeDuration{0};
            pq.push(PQItem{EdgeDuration{0}, s});
        }

        while (!pq.empty())
        {
            auto cur = pq.top(); pq.pop();
            if (cur.d != dist[cur.node])
                continue;
            if (cur.d > duration_threshold)
                break;

            if (from_alias<int>(cur.d) > 0)
                reachable_nodes.emplace_back(cur.node, cur.d);

            // relax edges (undirected): for each adjacent edge, compute traversal cost and relax
            if (mld_facade)
            {
                for (auto e : mld_facade->GetAdjacentEdgeRange(cur.node))
                {
                    // For MLD, edge entries are represented by node-level metrics + optional turn penalties
                    const NodeID to = mld_facade->GetTarget(e);

                    // Skip excluded nodes
                    if (mld_facade->ExcludeNode(to))
                        continue;

                    // base node duration for traversing into `to`
                    const EdgeDuration node_dur = mld_facade->GetNodeDuration(to);

                    // turn duration penalty (if available) - stored by turn id in edge data
                    const auto edge_data = mld_facade->GetEdgeData(e);
                    const auto turn_penalty = mld_facade->GetDurationPenaltyForEdgeID(edge_data.turn_id);
                    const EdgeDuration turn_pen = to_alias<EdgeDuration>(from_alias<int>(turn_penalty));

                    const EdgeDuration edge_cost = node_dur + turn_pen;
                    const EdgeDuration nd = cur.d + edge_cost;
                    if (nd < dist[to])
                    {
                        dist[to] = nd;
                        pq.push(PQItem{nd, to});
                    }
                }
            }
            else
            {
                for (auto e : ch_facade->GetAdjacentEdgeRange(cur.node))
                {
                    const auto &edata = ch_facade->GetEdgeData(e);
                    // Only consider traversable directions
                    if (!(edata.forward || edata.backward))
                        continue;
                    const NodeID to = ch_facade->GetTarget(e);

                    // Edge duration is stored on CH edge data
                    const EdgeDuration edge_dur = to_alias<EdgeDuration>(edata.duration);
                    const EdgeDuration nd = cur.d + edge_dur;
                    if (nd < dist[to])
                    {
                        dist[to] = nd;
                        pq.push(PQItem{nd, to});
                    }
                }
            }
        }
    }
    else
    {
        // fallback to RTree bounding box expansion
        TIMER_START(GET_EDGES_TIMER);
        auto edges = df_base.GetEdgesInBox(southwest, northeast);
        TIMER_STOP(GET_EDGES_TIMER);

        std::unordered_set<unsigned> seen_roads;
        for (const auto &data : edges)
        {
            PackedGeometryID geom_id = df_base.GetGeometryIndex(
                data.forward_segment_id.enabled ? data.forward_segment_id.id : data.reverse_segment_id.id)
                                                .id;
            if (seen_roads.count(geom_id) != 0)
                continue;
            seen_roads.insert(geom_id);

            auto vcoord = df_base.GetCoordinateOfNode(data.v);
            auto ucoord = df_base.GetCoordinateOfNode(data.u);
            (void)ucoord; (void)vcoord;
        }
    }

    std::stringstream buf;
    buf << std::setprecision(12);
    buf << "{\"type\":\"FeatureCollection\",\"max_range\":" << effective_range << ",\"features\":[";
    bool first_feature = true;

    // To avoid duplicate points when geometries overlap, track seen coordinate strings
    std::unordered_set<std::string> seen_coords;

    for (const auto &p : reachable_nodes)
    {
        const NodeID node = p.first;
        const auto dur = p.second;

        // Defensive filter: only include nodes within threshold
        if (dur > duration_threshold)
            continue;

        // Map edge-based node -> full geometry (forward or reverse) and emit all vertices
        const auto geom = algorithms.GetFacade().GetGeometryIndex(node);

        if (geom.id != SPECIAL_GEOMETRYID)
        {
            // get forward or reverse geometry depending on stored flag
            if (geom.forward)
            {
                for (const auto repr : algorithms.GetFacade().GetUncompressedForwardGeometry(geom.id))
                {
                    const auto coord = algorithms.GetFacade().GetCoordinateOfNode(repr);
                    std::ostringstream key_ss;
                    key_ss << std::setprecision(12) << toFloating(coord.lon) << ',' << toFloating(coord.lat);
                    const auto key = key_ss.str();
                    if (seen_coords.find(key) != seen_coords.end())
                        continue;
                    seen_coords.insert(key);

                    if (!first_feature)
                        buf << ',';
                    first_feature = false;
                    buf << "{\"type\":\"Feature\",\"properties\":{\"weight\":" << from_alias<int>(dur) << "},\"geometry\":{\"type\":\"Point\",\"coordinates\":[";
                    buf << toFloating(coord.lon) << ',' << toFloating(coord.lat);
                    buf << "]}}";
                }
            }
            else
            {
                for (const auto repr : algorithms.GetFacade().GetUncompressedReverseGeometry(geom.id))
                {
                    const auto coord = algorithms.GetFacade().GetCoordinateOfNode(repr);
                    std::ostringstream key_ss;
                    key_ss << std::setprecision(12) << toFloating(coord.lon) << ',' << toFloating(coord.lat);
                    const auto key = key_ss.str();
                    if (seen_coords.find(key) != seen_coords.end())
                        continue;
                    seen_coords.insert(key);

                    if (!first_feature)
                        buf << ',';
                    first_feature = false;
                    buf << "{\"type\":\"Feature\",\"properties\":{\"weight\":" << from_alias<int>(dur) << "},\"geometry\":{\"type\":\"Point\",\"coordinates\":[";
                    buf << toFloating(coord.lon) << ',' << toFloating(coord.lat);
                    buf << "]}}";
                }
            }
        }
        else
        {
            // fallback: single representative coordinate
            const auto coord = algorithms.GetFacade().GetCoordinateOfNode(node);
            std::ostringstream key_ss;
            key_ss << std::setprecision(12) << toFloating(coord.lon) << ',' << toFloating(coord.lat);
            const auto key = key_ss.str();
            if (seen_coords.find(key) == seen_coords.end())
            {
                seen_coords.insert(key);
                if (!first_feature)
                    buf << ',';
                first_feature = false;
                buf << "{\"type\":\"Feature\",\"properties\":{\"weight\":" << from_alias<int>(dur) << "},\"geometry\":{\"type\":\"Point\",\"coordinates\":[";
                buf << toFloating(coord.lon) << ',' << toFloating(coord.lat);
                buf << "]}}";
            }
        }
    }
    buf << "]}";

    result = std::string(buf.str());
    return Status::Ok;
}

} // plugins
} // engine
} // osrm
