#ifndef INTERNAL_EXTRACTOR_EDGE_HPP
#define INTERNAL_EXTRACTOR_EDGE_HPP

#include "extractor/node_based_edge.hpp"
#include "extractor/travel_mode.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include "extractor/guidance/classification_data.hpp"
#include "osrm/coordinate.hpp"
#include <utility>

namespace osrm
{
namespace extractor
{

struct InternalExtractorEdge
{
    // specify the type of the weight data
    enum class WeightType : char
    {
        INVALID,
        SPEED,
        EDGE_DURATION,
        WAY_DURATION,
    };

    struct WeightData
    {
        WeightData() : duration(0.0), type(WeightType::INVALID) {}

        union {
            double duration;
            double speed;
        };
        WeightType type;
    };

    explicit InternalExtractorEdge()
        : result(MIN_OSM_NODEID,
                 MIN_OSM_NODEID,
                 0,
                 0,
                 false,
                 false,
                 false,
                 false,
                 true,
                 TRAVEL_MODE_INACCESSIBLE,
                 false,
                 guidance::TurnLaneType::empty,
                 guidance::RoadClassificationData())
    {
    }

    explicit InternalExtractorEdge(OSMNodeID source,
                                   OSMNodeID target,
                                   NodeID name_id,
                                   WeightData weight_data,
                                   bool forward,
                                   bool backward,
                                   bool roundabout,
                                   bool access_restricted,
                                   bool startpoint,
                                   TravelMode travel_mode,
                                   bool is_split,
                                   LaneDescriptionID lane_description,
                                   guidance::RoadClassificationData road_classification)
        : result(OSMNodeID(source),
                 OSMNodeID(target),
                 name_id,
                 0,
                 forward,
                 backward,
                 roundabout,
                 access_restricted,
                 startpoint,
                 travel_mode,
                 is_split,
                 lane_description,
                 std::move(road_classification)),
          weight_data(std::move(weight_data))
    {
    }

    // data that will be written to disk
    NodeBasedEdgeWithOSM result;
    // intermediate edge weight
    WeightData weight_data;
    // coordinate of the source node
    util::Coordinate source_coordinate;

    // necessary static util functions for stxxl's sorting
    static InternalExtractorEdge min_osm_value()
    {
        return InternalExtractorEdge(MIN_OSM_NODEID,
                                     MIN_OSM_NODEID,
                                     0,
                                     WeightData(),
                                     false,
                                     false,
                                     false,
                                     false,
                                     true,
                                     TRAVEL_MODE_INACCESSIBLE,
                                     false,
                                     INVALID_LANE_DESCRIPTIONID,
                                     guidance::RoadClassificationData());
    }
    static InternalExtractorEdge max_osm_value()
    {
        return InternalExtractorEdge(MAX_OSM_NODEID,
                                     MAX_OSM_NODEID,
                                     0,
                                     WeightData(),
                                     false,
                                     false,
                                     false,
                                     false,
                                     true,
                                     TRAVEL_MODE_INACCESSIBLE,
                                     false,
                                     INVALID_LANE_DESCRIPTIONID,
                                     guidance::RoadClassificationData());
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

struct CmpEdgeByInternalStartThenInternalTargetID
{
    using value_type = InternalExtractorEdge;
    bool operator()(const InternalExtractorEdge &lhs, const InternalExtractorEdge &rhs) const
    {
        return (lhs.result.source < rhs.result.source) ||
               ((lhs.result.source == rhs.result.source) &&
                (lhs.result.target < rhs.result.target));
    }

    value_type max_value() { return InternalExtractorEdge::max_internal_value(); }
    value_type min_value() { return InternalExtractorEdge::min_internal_value(); }
};

struct CmpEdgeByOSMStartID
{
    using value_type = InternalExtractorEdge;
    bool operator()(const InternalExtractorEdge &lhs, const InternalExtractorEdge &rhs) const
    {
        return lhs.result.osm_source_id < rhs.result.osm_source_id;
    }

    value_type max_value() { return InternalExtractorEdge::max_osm_value(); }
    value_type min_value() { return InternalExtractorEdge::min_osm_value(); }
};

struct CmpEdgeByOSMTargetID
{
    using value_type = InternalExtractorEdge;
    bool operator()(const InternalExtractorEdge &lhs, const InternalExtractorEdge &rhs) const
    {
        return lhs.result.osm_target_id < rhs.result.osm_target_id;
    }

    value_type max_value() { return InternalExtractorEdge::max_osm_value(); }
    value_type min_value() { return InternalExtractorEdge::min_osm_value(); }
};
}
}

#endif // INTERNAL_EXTRACTOR_EDGE_HPP
