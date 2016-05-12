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
// these are used for duration based mode of transportations like ferries
OSRM_STRONG_TYPEDEF(double, ValueByEdge)
OSRM_STRONG_TYPEDEF(double, ValueByMeter)
using ByEdgeOrByMeterValue = mapbox::util::variant<detail::ValueByEdge, detail::ValueByMeter>;

struct ToValueByEdge
{
    ToValueByEdge(double distance_) : distance(distance_) {}

    ValueByEdge operator()(const ValueByMeter by_meter) const
    {
        return ValueByEdge{distance / static_cast<double>(by_meter)};
    }

    ValueByEdge operator()(const ValueByEdge by_edge) const { return by_edge; }

    double distance;
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
                 TRAVEL_MODE_INACCESSIBLE,
                 false,
                 guidance::TurnLaneType::empty,
                 guidance::RoadClassification()),
          weight_data(detail::ValueByMeter{0.0}), duration_data(detail::ValueByMeter{0.0})
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
                                   TravelMode travel_mode,
                                   bool is_split,
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
                 travel_mode,
                 is_split,
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
                                     detail::ValueByMeter{0.0},
                                     detail::ValueByMeter{0.0},
                                     false, // forward
                                     false, // backward
                                     false, // roundabout
                                     false, // circular
                                     true,  // can be startpoint
                                     TRAVEL_MODE_INACCESSIBLE,
                                     false,
                                     INVALID_LANE_DESCRIPTIONID,
                                     guidance::RoadClassification(),
                                     util::Coordinate());
    }
    static InternalExtractorEdge max_osm_value()
    {
        return InternalExtractorEdge(MAX_OSM_NODEID,
                                     MAX_OSM_NODEID,
                                     SPECIAL_NODEID,
                                     detail::ValueByMeter{0.0},
                                     detail::ValueByMeter{0.0},
                                     false, // forward
                                     false, // backward
                                     false, // roundabout
                                     false, // circular
                                     true,  // can be startpoint
                                     TRAVEL_MODE_INACCESSIBLE,
                                     false,
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
