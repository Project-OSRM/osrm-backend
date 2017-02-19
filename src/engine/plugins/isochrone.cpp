#include "engine/plugins/isochrone.hpp"
#include "engine/edge_unpacker.hpp"
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

IsochronePlugin::IsochronePlugin() : distance_table(heaps) {}

Status
IsochronePlugin::HandleRequest(const std::shared_ptr<const datafacade::BaseDataFacade> facade,
                               const api::IsochroneParameters &parameters,
                               std::string &pbf_buffer) const
{

    // const auto range = parameters.range;
    const auto range = 300;

    util::Coordinate startcoord = parameters.coordinates.front();

    const auto MAX_SPEED_METERS_PER_SECOND = 90 / 3.6;
    const auto MAX_TRAVEL_DISTANCE_METERS = MAX_SPEED_METERS_PER_SECOND * range;

    const auto latitude_range = MAX_TRAVEL_DISTANCE_METERS /
                                (util::coordinate_calculation::detail::EARTH_RADIUS * M_PI * 2) *
                                360;

    const auto longitude_range = 360 * MAX_TRAVEL_DISTANCE_METERS /
                                 ((util::coordinate_calculation::detail::EARTH_RADIUS * M_PI * 2) *
                                  cos(static_cast<double>(toFloating(startcoord.lat)) *
                                      util::coordinate_calculation::detail::DEGREE_TO_RAD));

    util::Coordinate southwest{
        util::FloatLongitude{
            static_cast<double>(static_cast<double>(toFloating(startcoord.lon)) - longitude_range)},
        util::FloatLatitude{
            static_cast<double>(static_cast<double>(toFloating(startcoord.lat)) - latitude_range)}};
    util::Coordinate northeast{
        util::FloatLongitude{
            static_cast<double>(static_cast<double>(toFloating(startcoord.lon)) + longitude_range)},
        util::FloatLatitude{
            static_cast<double>(static_cast<double>(toFloating(startcoord.lat)) + latitude_range)}};

    util::Log() << "sw " << southwest;
    util::Log() << "ne " << northeast;

    TIMER_START(GET_EDGES_TIMER);
    auto edges = facade->GetEdgesInBox(southwest, northeast);
    TIMER_STOP(GET_EDGES_TIMER);
    util::Log() << "Fetch RTree " << TIMER_MSEC(GET_EDGES_TIMER);

    TIMER_START(PHANTOM_TIMER);
    // Convert edges to phantom nodes
    std::vector<PhantomNode> phantoms;
    auto startpoints = facade->NearestPhantomNodes(startcoord, 1);
    phantoms.push_back(startpoints.front().phantom_node);

    std::unordered_set<unsigned> seen_roads;

    // For every road, add two phantom nodes (one for each travel direction),
    // located at the "end" of the road.  When the distance to a node
    // comes back from the one-to-many search and the distance exceeds our range,
    // we will fetch the geometry for that road, and step backwards until we
    // are inside our range.  This needs to be done for both travel directions
    // on a road to get the full isochrone.
    std::for_each(edges.begin(), edges.end(), [&](const auto &data) {

        if (seen_roads.count(data.packed_geometry_id) != 0)
            return;

        seen_roads.insert(data.packed_geometry_id);

        EdgeWeight forward_weight_offset = 0, forward_weight = 0;
        EdgeWeight reverse_weight_offset = 0, reverse_weight = 0;
        EdgeWeight forward_duration_offset = 0, forward_duration = 0;
        EdgeWeight reverse_duration_offset = 0, reverse_duration = 0;

        const std::vector<EdgeWeight> forward_weight_vector =
            facade->GetUncompressedForwardWeights(data.packed_geometry_id);
        const std::vector<EdgeWeight> reverse_weight_vector =
            facade->GetUncompressedReverseWeights(data.packed_geometry_id);
        const std::vector<EdgeWeight> forward_duration_vector =
            facade->GetUncompressedForwardDurations(data.packed_geometry_id);
        const std::vector<EdgeWeight> reverse_duration_vector =
            facade->GetUncompressedReverseDurations(data.packed_geometry_id);

        forward_weight_offset =
            std::accumulate(forward_weight_vector.begin(), forward_weight_vector.end(), 0);
        forward_duration_offset =
            std::accumulate(forward_duration_vector.begin(), forward_duration_vector.end(), 0);

        forward_weight = forward_weight_vector[data.fwd_segment_position];
        forward_duration = forward_duration_vector[data.fwd_segment_position];

        BOOST_ASSERT(data.fwd_segment_position < reverse_weight_vector.size());

        reverse_weight_offset =
            std::accumulate(reverse_weight_vector.begin(), reverse_weight_vector.end(), 0);
        reverse_duration_offset =
            std::accumulate(reverse_duration_vector.begin(), reverse_duration_vector.end(), 0);

        reverse_weight =
            reverse_weight_vector[reverse_weight_vector.size() - data.fwd_segment_position - 1];
        reverse_duration =
            reverse_duration_vector[reverse_duration_vector.size() - data.fwd_segment_position - 1];

        auto vcoord = facade->GetCoordinateOfNode(data.v);
        auto ucoord = facade->GetCoordinateOfNode(data.u);

        if (data.forward_segment_id.enabled)
        {
            // We fetched a rectangle from the RTree - the corners will be out of range.
            // This filters those out.
            if (util::coordinate_calculation::haversineDistance(ucoord, startcoord) >
                MAX_TRAVEL_DISTANCE_METERS)
                return;
            auto datacopy = data;
            datacopy.reverse_segment_id.enabled = false;
            phantoms.emplace_back(datacopy,
                                  forward_weight,
                                  reverse_weight,
                                  forward_weight_offset,
                                  reverse_weight_offset,
                                  forward_duration,
                                  reverse_duration,
                                  forward_duration_offset,
                                  reverse_duration_offset,
                                  vcoord,
                                  vcoord);
        }
        if (data.reverse_segment_id.enabled)
        {
            if (util::coordinate_calculation::haversineDistance(vcoord, startcoord) >
                MAX_TRAVEL_DISTANCE_METERS)
                return;
            auto datacopy = data;
            datacopy.forward_segment_id.enabled = false;
            phantoms.emplace_back(datacopy,
                                  forward_weight,
                                  reverse_weight,
                                  forward_weight_offset,
                                  reverse_weight_offset,
                                  forward_duration,
                                  reverse_duration,
                                  forward_duration_offset,
                                  reverse_duration_offset,
                                  ucoord,
                                  ucoord);
        }
    });

    std::vector<std::size_t> sources;
    std::vector<std::size_t> destinations;

    sources.push_back(0);
    destinations.resize(phantoms.size() - 1);
    std::iota(destinations.begin(), destinations.end(), 1);

    TIMER_STOP(PHANTOM_TIMER);
    util::Log() << "Make phantom nodes " << TIMER_MSEC(PHANTOM_TIMER);

    TIMER_START(TABLE_TIMER);
    auto table = distance_table(facade, phantoms, sources, destinations, range * 10 + 1);
    TIMER_STOP(TABLE_TIMER);
    util::Log() << "Distance table " << TIMER_MSEC(TABLE_TIMER);

    std::clog << "Phantoms size: " << phantoms.size() << std::endl;
    std::clog << "Table size: " << table.size() << std::endl;

    std::stringstream buf;
    buf << std::setprecision(12);
    buf << "{\"type\":\"FeatureCollection\",\"features\":[";
    bool firstline = true;

    std::vector<std::size_t> sorted_idx(phantoms.size() - 1);
    std::iota(sorted_idx.begin(), sorted_idx.end(), 1);

    std::sort(sorted_idx.begin(), sorted_idx.end(), [&](const auto a, const auto b) {
        return table[a - 1] > table[b - 1];
    });

    for (const auto i : sorted_idx)
    {
        const auto &phantom = phantoms[i];
        const auto weight = table[i - 1];

        if (weight > range * 10)
            continue;

        if (!firstline)
            buf << ",";
        firstline = false;
        buf << "{\"type\":\"Feature\",\"properties\":{\"weight\":" << weight
            << "},\"geometry\":{\"type\":\"Point\",\"coordinates\":[";

        buf << toFloating(phantom.location.lon) << "," << toFloating(phantom.location.lat);

        buf << "]}}";
    }

    buf << "]}";

    pbf_buffer = buf.str();

    return Status::Ok;
}

} // plugins
} // engine
} // osrm
