#ifndef INTERNAL_EXTRACTOR_EDGE_HPP
#define INTERNAL_EXTRACTOR_EDGE_HPP

#include "extractor/guidance/road_classification.hpp"
#include "extractor/node_based_edge.hpp"
#include "extractor/travel_mode.hpp"
#include "osrm/coordinate.hpp"
#include "util/strong_typedef.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>
#include <utility>
#include <variant/variant.hpp>

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

    // data that will be written to disk
    NodeBasedEdgeWithOSM result;
    // intermediate edge weight
    WeightData weight_data;
    // intermediate edge duration
    DurationData duration_data;
    // coordinate of the source node
    util::Coordinate source_coordinate;

    InternalExtractorEdge() = default;

    // necessary static util functions for stxxl's sorting
    static InternalExtractorEdge min_osm_value()
    {
        return InternalExtractorEdge{NodeBasedEdgeWithOSM{MIN_OSM_NODEID,
                                                          MIN_OSM_NODEID,
                                                          0,
                                                          0,
                                                          0,
                                                          false,
                                                          false,
                                                          false,
                                                          false,
                                                          false,
                                                          false,
                                                          TRAVEL_MODE_INACCESSIBLE,
                                                          INVALID_LANE_DESCRIPTIONID,
                                                          guidance::RoadClassification()},
                                     detail::ValueByMeter{0.0},
                                     detail::ValueByMeter{0.0},
                                     {}};
    }
    static InternalExtractorEdge max_osm_value()
    {
        return InternalExtractorEdge{NodeBasedEdgeWithOSM{MAX_OSM_NODEID,
                                                          MAX_OSM_NODEID,
                                                          0,
                                                          0,
                                                          0,
                                                          false,
                                                          false,
                                                          false,
                                                          false,
                                                          false,
                                                          false,
                                                          TRAVEL_MODE_INACCESSIBLE,
                                                          INVALID_LANE_DESCRIPTIONID,
                                                          guidance::RoadClassification()},
                                     detail::ValueByMeter{0.0},
                                     detail::ValueByMeter{0.0},
                                     {}};
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
