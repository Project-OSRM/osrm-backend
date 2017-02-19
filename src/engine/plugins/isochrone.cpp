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
    const auto range = 30;

    util::Coordinate startcoord = parameters.coordinates.front();

    const auto MAX_SPEED_METERS_PER_SECOND = 200 / 3.6;
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
    std::vector<PhantomNode> phantoms(edges.size() + 1);
    auto startpoints = facade->NearestPhantomNodes(startcoord, 1);
    phantoms[0] = startpoints.front().phantom_node;

    std::transform(edges.begin(), edges.end(), phantoms.begin() + 1, [&](const auto &data) {
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
        for (std::size_t i = 0; i < data.fwd_segment_position; i++)
        {
            forward_weight_offset += forward_weight_vector[i];
            forward_duration_offset += forward_duration_vector[i];
        }
        forward_weight = forward_weight_vector[data.fwd_segment_position];
        forward_duration = forward_duration_vector[data.fwd_segment_position];

        BOOST_ASSERT(data.fwd_segment_position < reverse_weight_vector.size());

        for (std::size_t i = 0; i < reverse_weight_vector.size() - data.fwd_segment_position - 1;
             i++)
        {
            reverse_weight_offset += reverse_weight_vector[i];
            reverse_duration_offset += reverse_duration_vector[i];
        }
        reverse_weight =
            reverse_weight_vector[reverse_weight_vector.size() - data.fwd_segment_position - 1];
        reverse_duration =
            reverse_duration_vector[reverse_duration_vector.size() - data.fwd_segment_position - 1];

        double ratio = 0.;
        if (data.forward_segment_id.id != SPECIAL_SEGMENTID)
        {
            forward_weight = static_cast<EdgeWeight>(forward_weight * ratio);
            forward_duration = static_cast<EdgeWeight>(forward_duration * ratio);
        }
        if (data.reverse_segment_id.id != SPECIAL_SEGMENTID)
        {
            reverse_weight -= static_cast<EdgeWeight>(reverse_weight * ratio);
            reverse_duration -= static_cast<EdgeWeight>(reverse_duration * ratio);
        }

        auto coord = facade->GetCoordinateOfNode(data.u);

        return PhantomNode{data,
                           forward_weight,
                           reverse_weight,
                           forward_weight_offset,
                           reverse_weight_offset,
                           forward_duration,
                           reverse_duration,
                           forward_duration_offset,
                           reverse_duration_offset,
                           coord,
                           coord};
    });

    std::vector<std::size_t> sources;
    std::vector<std::size_t> destinations;

    sources.push_back(0);
    destinations.resize(edges.size());
    std::iota(destinations.begin(), destinations.end(), 1);

    TIMER_STOP(PHANTOM_TIMER);
    util::Log() << "Make phantom nodes " << TIMER_MSEC(PHANTOM_TIMER);

    TIMER_START(TABLE_TIMER);
    auto table = distance_table(facade, phantoms, sources, destinations, range * 100);
    TIMER_STOP(TABLE_TIMER);
    util::Log() << "Distance table " << TIMER_MSEC(TABLE_TIMER);

    std::clog << "Phantoms size: " << phantoms.size() << std::endl;
    std::clog << "Table size: " << table.size() << std::endl;

    /*
        for (const auto n : table)
        {
            std::clog << n << " ";
        }
        std::clog << std::endl;
        */

    std::stringstream buf;
    buf << std::setprecision(12);
    buf << "{\"type\":\"FeatureCollection\",\"features\":[";
    bool firstline = true;
    for (std::size_t i = 1; i < phantoms.size(); i++)
    {
        const auto &phantom = phantoms[i];
        const auto weight = table[i - 1];

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
