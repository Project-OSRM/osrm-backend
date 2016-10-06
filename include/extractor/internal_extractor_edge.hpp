#ifndef INTERNAL_EXTRACTOR_EDGE_HPP
#define INTERNAL_EXTRACTOR_EDGE_HPP

#include "extractor/node_based_edge.hpp"
#include "extractor/travel_mode.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include "extractor/guidance/road_classification.hpp"
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
                 false, // road_priority_forward
                 false, // road_priority_backward
                 TRAVEL_MODE_INACCESSIBLE,
                 false,
                 guidance::TurnLaneType::empty,
                 guidance::RoadClassification())
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
                                   bool road_priority_forward,
                                   bool road_priority_backward,
                                   TravelMode travel_mode,
                                   bool is_split,
                                   LaneDescriptionID lane_description,
                                   guidance::RoadClassification road_classification)
        : result(source,
                 target,
                 name_id,
                 0,
                 forward,
                 backward,
                 roundabout,
                 access_restricted,
                 startpoint,
                 road_priority_forward,
                 road_priority_backward,
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
                                     false, // road_priority_forward
                                     false, // road_priority_backward
                                     TRAVEL_MODE_INACCESSIBLE,
                                     false,
                                     INVALID_LANE_DESCRIPTIONID,
                                     guidance::RoadClassification());
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
                                     false, // road_priority_forward
                                     false, // road_priority_backward
                                     TRAVEL_MODE_INACCESSIBLE,
                                     false,
                                     INVALID_LANE_DESCRIPTIONID,
                                     guidance::RoadClassification());
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
