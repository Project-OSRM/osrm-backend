#include "engine/plugins/isochrone.hpp"
#include "engine/search_engine_data.hpp"
#include "engine/plugins/plugin_base.hpp"
#include "util/timing_util.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/log.hpp"

#include "extractor/edge_based_node.hpp"

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

IsochronePlugin::IsochronePlugin() {}

Status
IsochronePlugin::HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                               const api::IsochroneParameters &parameters,
                               osrm::engine::api::ResultT &result) const
{
    const auto range = parameters.range;
    util::Coordinate startcoord = parameters.coordinates.front();

    const double MAX_SPEED_METERS_PER_SECOND = 90.0 / 3.6;
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

    TIMER_START(GET_EDGES_TIMER);
    const auto &facade = algorithms.GetFacade();
    auto edges = facade.GetEdgesInBox(southwest, northeast);
    TIMER_STOP(GET_EDGES_TIMER);

    std::vector<PhantomNode> phantoms;
    auto startpoints = facade.NearestPhantomNodes(startcoord,
                                                  1,
                                                  std::optional<double>{},
                                                  std::optional<engine::Bearing>{},
                                                  engine::Approach::UNRESTRICTED);
    phantoms.push_back(startpoints.front().phantom_node);

    std::unordered_set<unsigned> seen_roads;

    std::for_each(edges.begin(), edges.end(), [&](const auto &data) {
        PackedGeometryID geom_id = facade.GetGeometryIndex(
            data.forward_segment_id.enabled ? data.forward_segment_id.id : data.reverse_segment_id.id)
                                            .id;
        if (seen_roads.count(geom_id) != 0)
            return;
        seen_roads.insert(geom_id);

        EdgeWeight forward_weight_offset{0}, forward_weight{0};
        EdgeWeight reverse_weight_offset{0}, reverse_weight{0};
        EdgeDuration forward_duration_offset{0}, forward_duration{0};
        EdgeDuration reverse_duration_offset{0}, reverse_duration{0};

        auto forward_weight_range = facade.GetUncompressedForwardWeights(geom_id);
        auto reverse_weight_range = facade.GetUncompressedReverseWeights(geom_id);
        auto forward_duration_range = facade.GetUncompressedForwardDurations(geom_id);
        auto reverse_duration_range = facade.GetUncompressedReverseDurations(geom_id);

        std::vector<SegmentWeight> forward_weight_vector(forward_weight_range.begin(), forward_weight_range.end());
        std::vector<SegmentWeight> reverse_weight_vector(reverse_weight_range.begin(), reverse_weight_range.end());
        std::vector<SegmentDuration> forward_duration_vector(forward_duration_range.begin(), forward_duration_range.end());
        std::vector<SegmentDuration> reverse_duration_vector(reverse_duration_range.begin(), reverse_duration_range.end());

        for (const auto &w : forward_weight_vector)
            forward_weight_offset += alias_cast<EdgeWeight>(w);
        for (const auto &d : forward_duration_vector)
            forward_duration_offset += alias_cast<EdgeDuration>(d);

        BOOST_ASSERT(data.fwd_segment_position < forward_weight_vector.size());

        forward_weight = alias_cast<EdgeWeight>(forward_weight_vector[data.fwd_segment_position]);
        forward_duration = alias_cast<EdgeDuration>(forward_duration_vector[data.fwd_segment_position]);

        BOOST_ASSERT(data.fwd_segment_position < reverse_weight_vector.size());

        for (const auto &w : reverse_weight_vector)
            reverse_weight_offset += alias_cast<EdgeWeight>(w);
        for (const auto &d : reverse_duration_vector)
            reverse_duration_offset += alias_cast<EdgeDuration>(d);

        reverse_weight = alias_cast<EdgeWeight>(
            reverse_weight_vector[reverse_weight_vector.size() - data.fwd_segment_position - 1]);
        reverse_duration = alias_cast<EdgeDuration>(
            reverse_duration_vector[reverse_duration_vector.size() - data.fwd_segment_position - 1]);

        auto vcoord = facade.GetCoordinateOfNode(data.v);
        auto ucoord = facade.GetCoordinateOfNode(data.u);

        if (data.forward_segment_id.enabled)
        {
            if (util::coordinate_calculation::greatCircleDistance(ucoord, startcoord) > MAX_TRAVEL_DISTANCE_METERS)
                return;
            auto datacopy = data;
            datacopy.reverse_segment_id.enabled = false;
            ComponentID comp{0, 0};
            phantoms.emplace_back(datacopy,
                                  comp,
                                  forward_weight,
                                  reverse_weight,
                                  forward_weight_offset,
                                  reverse_weight_offset,
                                  EdgeDistance{0},
                                  EdgeDistance{0},
                                  EdgeDistance{0},
                                  EdgeDistance{0},
                                  forward_duration,
                                  reverse_duration,
                                  forward_duration_offset,
                                  reverse_duration_offset,
                                  true,
                                  true,
                                  true,
                                  true,
                                  vcoord,
                                  vcoord,
                                  static_cast<unsigned short>(0));
        }
        if (data.reverse_segment_id.enabled)
        {
            if (util::coordinate_calculation::greatCircleDistance(vcoord, startcoord) > MAX_TRAVEL_DISTANCE_METERS)
                return;
            auto datacopy = data;
            datacopy.forward_segment_id.enabled = false;
            ComponentID comp{0, 0};
            phantoms.emplace_back(datacopy,
                                  comp,
                                  forward_weight,
                                  reverse_weight,
                                  forward_weight_offset,
                                  reverse_weight_offset,
                                  EdgeDistance{0},
                                  EdgeDistance{0},
                                  EdgeDistance{0},
                                  EdgeDistance{0},
                                  forward_duration,
                                  reverse_duration,
                                  forward_duration_offset,
                                  reverse_duration_offset,
                                  true,
                                  true,
                                  true,
                                  true,
                                  ucoord,
                                  ucoord,
                                  static_cast<unsigned short>(0));
        }
    });

    std::vector<std::size_t> sources;
    std::vector<std::size_t> destinations;

    sources.push_back(0);
    destinations.resize(phantoms.size() - 1);
    std::iota(destinations.begin(), destinations.end(), 1);

    std::vector<PhantomNodeCandidates> candidates_list;
    candidates_list.reserve(phantoms.size());
    for (const auto &ph : phantoms)
    {
        PhantomNodeCandidates c;
        c.push_back(ph);
        candidates_list.push_back(std::move(c));
    }

    auto result_pair = algorithms.ManyToManySearch(candidates_list, sources, destinations, /*calculate_distance=*/false);
    const auto &durations = result_pair.first;

    std::stringstream buf;
    buf << std::setprecision(12);
    buf << "{\"type\":\"FeatureCollection\",\"features\":\n[";
    bool first_feature = true;

    for (std::size_t i = 0; i < phantoms.size(); ++i)
    {
        if (i == 0)
            continue;
        const auto idx = i - 1;
        if (idx >= durations.size())
            continue;
        const auto duration = durations[idx];
        if (from_alias<int>(duration) < 0)
            continue;
        if (!first_feature)
            buf << ',';
        first_feature = false;
        buf << "{\"type\":\"Feature\",\"properties\":{\"weight\":" << from_alias<int>(duration)
            << "},\"geometry\":{\"type\":\"Point\",\"coordinates\":[";
        buf << toFloating(phantoms[i].location.lon) << ',' << toFloating(phantoms[i].location.lat);
        buf << "]}}";
    }
    buf << "]}";

    result = std::string(buf.str());
    return Status::Ok;
}

} // plugins
} // engine
} // osrm
