#ifndef INTERNAL_EXTRACTOR_EDGE_HPP
#define INTERNAL_EXTRACTOR_EDGE_HPP

#include "extractor/guidance/road_classification.hpp"
#include "extractor/guidance/turn_lane_types.hpp"
#include "extractor/node_based_edge.hpp"
#include "extractor/travel_mode.hpp"
#include "osrm/coordinate.hpp"
#include "util/strong_typedef.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>
#include <mapbox/variant.hpp>
#include <utility>

namespace osrm
{
namespace extractor
{

namespace detail
{
// Use a single float to pack two positive values:
//  * by_edge: + sign, value >= 0, value used as the edge weight
//  * by_meter: - sign, value > 0, value used as a denominator in distance / value
struct ByEdgeOrByMeterValue
{
    struct ValueByEdge
    {
    } static const by_edge;
    struct ValueByMeter
    {
    } static const by_meter;

    ByEdgeOrByMeterValue() : value(0.f) {}

    ByEdgeOrByMeterValue(ValueByEdge, double input)
    {
        BOOST_ASSERT(input >= 0.f);
        value = static_cast<value_type>(input);
    }

    ByEdgeOrByMeterValue(ValueByMeter, double input)
    {
        BOOST_ASSERT(input > 0.f);
        value = -static_cast<value_type>(input);
    }

    double operator()(double distance) { return value >= 0 ? value : -distance / value; }

  private:
    using value_type = float;
    value_type value;
};
}

struct InternalExtractorEdge
{
    using WeightData = detail::ByEdgeOrByMeterValue;
    using DurationData = detail::ByEdgeOrByMeterValue;

    explicit InternalExtractorEdge()
        : result(MIN_OSM_NODEID,
                 MIN_OSM_NODEID,
                 SPECIAL_NODEID,
                 0,
                 0,
                 false, // forward
                 false, // backward
                 false, // roundabout
                 false, // circular
                 true,  // can be startpoint
                 false, // local access only
                 false, // split edge
                 TRAVEL_MODE_INACCESSIBLE,
                 guidance::TurnLaneType::empty,
                 guidance::RoadClassification()),
          weight_data(), duration_data()
    {
    }

    explicit InternalExtractorEdge(OSMNodeID source,
                                   OSMNodeID target,
                                   NodeID name_id,
                                   WeightData weight_data,
                                   DurationData duration_data,
                                   bool forward,
                                   bool backward,
                                   bool roundabout,
                                   bool circular,
                                   bool startpoint,
                                   bool restricted,
                                   bool is_split,
                                   TravelMode travel_mode,
                                   LaneDescriptionID lane_description,
                                   guidance::RoadClassification road_classification,
                                   util::Coordinate source_coordinate)
        : result(source,
                 target,
                 name_id,
                 0,
                 0,
                 forward,
                 backward,
                 roundabout,
                 circular,
                 startpoint,
                 restricted,
                 is_split,
                 travel_mode,
                 lane_description,
                 std::move(road_classification)),
          weight_data(std::move(weight_data)), duration_data(std::move(duration_data)),
          source_coordinate(std::move(source_coordinate))
    {
    }

    // data that will be written to disk
    NodeBasedEdgeWithOSM result;
    // intermediate edge weight
    WeightData weight_data;
    // intermediate edge duration
    DurationData duration_data;
    // coordinate of the source node
    util::Coordinate source_coordinate;

    // necessary static util functions for stxxl's sorting
    static InternalExtractorEdge min_osm_value()
    {
        return InternalExtractorEdge(MIN_OSM_NODEID,
                                     MIN_OSM_NODEID,
                                     SPECIAL_NODEID,
                                     WeightData(),
                                     DurationData(),
                                     false, // forward
                                     false, // backward
                                     false, // roundabout
                                     false, // circular
                                     true,  // can be startpoint
                                     false, // local access only
                                     false, // split edge
                                     TRAVEL_MODE_INACCESSIBLE,
                                     INVALID_LANE_DESCRIPTIONID,
                                     guidance::RoadClassification(),
                                     util::Coordinate());
    }
    static InternalExtractorEdge max_osm_value()
    {
        return InternalExtractorEdge(MAX_OSM_NODEID,
                                     MAX_OSM_NODEID,
                                     SPECIAL_NODEID,
                                     WeightData(),
                                     DurationData(),
                                     false, // forward
                                     false, // backward
                                     false, // roundabout
                                     false, // circular
                                     true,  // can be startpoint
                                     false, // local access only
                                     false, // split edge
                                     TRAVEL_MODE_INACCESSIBLE,
                                     INVALID_LANE_DESCRIPTIONID,
                                     guidance::RoadClassification(),
                                     util::Coordinate());
    }

    static InternalExtractorEdge min_internal_value()
    {
        auto v = min_osm_value();
        v.result.source = 0;
        v.result.target = 0;
        return v;
    }
    static InternalExtractorEdge max_internal_value()
    {
        auto v = max_osm_value();
        v.result.source = std::numeric_limits<NodeID>::max();
        v.result.target = std::numeric_limits<NodeID>::max();
        return v;
    }
};
}
}

#endif // INTERNAL_EXTRACTOR_EDGE_HPP
