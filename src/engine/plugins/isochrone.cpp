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

IsochronePlugin::IsochronePlugin() {}

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

    // Phase 1 - outgoing, upwards dijkstra search, setting d(v) for all v we visit
    heaps.InitializeOrClearFirstThreadLocalStorage(facade->GetNumberOfNodes());
    heaps.InitializeOrClearSecondThreadLocalStorage(facade->GetNumberOfNodes());

    auto &query_heap = *(heaps.forward_heap_1);
    if (phantoms[0].forward_segment_id.enabled)
    {
        query_heap.Insert(phantoms[0].forward_segment_id.id,
                          -phantoms[0].GetForwardWeightPlusOffset(),
                          phantoms[0].forward_segment_id.id);
    }
    if (phantoms[0].reverse_segment_id.enabled)
    {
        query_heap.Insert(phantoms[0].reverse_segment_id.id,
                          -phantoms[0].GetReverseWeightPlusOffset(),
                          phantoms[0].reverse_segment_id.id);
    }

    while (query_heap.Size() > 0)
    {
        const NodeID node = query_heap.DeleteMin();
        const EdgeWeight weight = query_heap.GetKey(node);

        for (auto edge : facade->GetAdjacentEdgeRange(node))
        {
            const auto &data = facade->GetEdgeData(edge);
            if (data.forward)
            {
                const NodeID to = facade->GetTarget(edge);
                const EdgeWeight edge_weight = data.weight;

                BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
                const EdgeWeight to_weight = weight + edge_weight;

                // No need to search further than our maximum radius
                if (weight > range * 10)
                    continue;

                // New Node discovered -> Add to Heap + Node Info Storage
                if (!query_heap.WasInserted(to))
                {
                    query_heap.Insert(to, to_weight, node);
                }
                // Found a shorter Path -> Update weight
                else if (to_weight < query_heap.GetKey(to))
                {
                    // new parent
                    query_heap.GetData(to) = node;
                    query_heap.DecreaseKey(to, to_weight);
                }
            }
        }
    }

    // Phase 2 - scan nodes in descending CH order, updating d(v) where we can
    TIMER_START(TABLE_TIMER);
    std::sort(phantoms.begin(), phantoms.end(), [](const auto &a, const auto &b) {
        const auto &a_id =
            a.forward_segment_id.enabled ? a.forward_segment_id.id : a.reverse_segment_id.id;
        const auto &b_id =
            b.forward_segment_id.enabled ? b.forward_segment_id.id : b.reverse_segment_id.id;

        return a_id < b_id;
    });
    // Now, scan over all the phantoms in reverse CH order, updating edge weights as we go
    for (const auto &p : phantoms)
    {
        const auto node =
            p.forward_segment_id.enabled ? p.forward_segment_id.id : p.reverse_segment_id.id;
        for (const auto &edge : facade->GetAdjacentEdgeRange(node))
        {
            const auto &data = facade->GetEdgeData(edge);
            if (data.backward)
            {
                const NodeID to = facade->GetTarget(edge);

                // New Node discovered -> Add to Heap + Node Info Storage
                if (!query_heap.WasInserted(to))
                {
                    // TODO: hmm, what does this mean? shouldn't be possible in a correct CH
                    // util::Log() << "WARNING: to not found in reverse scan";
                }
                // Found a shorter Path -> Update weight
                else
                {
                    const EdgeWeight weight = query_heap.GetKey(to);
                    const EdgeWeight edge_weight = data.weight;
                    const EdgeWeight to_weight = weight + edge_weight;

                    if (!query_heap.WasInserted(node))
                    {
                        query_heap.Insert(node, to_weight, to);
                    }
                    else if (to_weight < query_heap.GetKey(to))
                    {
                        // new parent
                        query_heap.GetData(node) = to;
                        query_heap.DecreaseKey(node, to_weight);
                    }
                }
            }
        }
    }

    TIMER_STOP(TABLE_TIMER);
    util::Log() << "Distance table " << TIMER_MSEC(TABLE_TIMER);

    std::clog << "Phantoms size: " << phantoms.size() << std::endl;

    std::stringstream buf;
    buf << std::setprecision(12);
    buf << "{\"type\":\"FeatureCollection\",\"features\":[";
    bool firstline = true;

    for (const auto &phantom : phantoms)
    {
        const auto node = phantom.forward_segment_id.enabled ? phantom.forward_segment_id.id
                                                             : phantom.reverse_segment_id.id;
        if (!query_heap.WasInserted(node))
            continue;
        const auto weight = query_heap.GetKey(node);

        // If INVALID_EDGE_WEIGHT, it means that the phantom couldn't be reached
        // from the isochrone centerpoint
        if (weight == INVALID_EDGE_WEIGHT)
            continue;

        // If the weight is > the max range, it means that this edge crosses
        // the range, but the start of it is inside the range. We need
        // to chop this line somewhere.
        // TODO: actually do that.
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
