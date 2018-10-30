#ifndef NODE_BASED_EDGE_HPP
#define NODE_BASED_EDGE_HPP

#include <cstdint>
#include <tuple>

#include "extractor/class_data.hpp"
#include "extractor/travel_mode.hpp"
#include "util/typedefs.hpp"

#include "extractor/road_classification.hpp"

namespace osrm
{
namespace extractor
{

// Flags describing the class of the road. This data is used during creation of graphs/guidance
// generation but is not available in annotation/navigation
struct NodeBasedEdgeClassification
{
    std::uint8_t forward : 1;                     // 1
    std::uint8_t backward : 1;                    // 1
    std::uint8_t is_split : 1;                    // 1
    std::uint8_t roundabout : 1;                  // 1
    std::uint8_t circular : 1;                    // 1
    std::uint8_t startpoint : 1;                  // 1
    std::uint8_t restricted : 1;                  // 1
    RoadClassification road_classification;       // 16 2
    std::uint8_t highway_turn_classification : 4; // 4
    std::uint8_t access_turn_classification : 4;  // 4

    NodeBasedEdgeClassification();

    NodeBasedEdgeClassification(const bool forward,
                                const bool backward,
                                const bool is_split,
                                const bool roundabout,
                                const bool circular,
                                const bool startpoint,
                                const bool restricted,
                                RoadClassification road_classification,
                                const std::uint8_t highway_turn_classification,
                                const std::uint8_t access_turn_classification)
        : forward(forward), backward(backward), is_split(is_split), roundabout(roundabout),
          circular(circular), startpoint(startpoint), restricted(restricted),
          road_classification(road_classification),
          highway_turn_classification(highway_turn_classification),
          access_turn_classification(access_turn_classification)
    {
    }

    bool operator==(const NodeBasedEdgeClassification &other) const
    {
        return (road_classification == other.road_classification) && (forward == other.forward) &&
               (backward == other.backward) && (is_split) == (other.is_split) &&
               (roundabout == other.roundabout) && (circular == other.circular) &&
               (startpoint == other.startpoint) && (restricted == other.restricted);
    }
};

// Annotative data, used in parts in guidance generation, in parts during navigation (classes) but
// mostly for annotation of edges. The entry can be shared between multiple edges and usually
// describes features present on OSM ways. This is the place to put specific data that you want to
// see as part of the API output but that does not influence navigation
struct NodeBasedEdgeAnnotation
{
    NameID name_id;                        // 32 4
    LaneDescriptionID lane_description_id; // 16 2
    ClassData classes;                     // 8  1
    TravelMode travel_mode : 4;            // 4
    bool is_left_hand_driving : 1;         // 1

    bool CanCombineWith(const NodeBasedEdgeAnnotation &other) const
    {
        return (
            std::tie(name_id, classes, travel_mode, is_left_hand_driving) ==
            std::tie(other.name_id, other.classes, other.travel_mode, other.is_left_hand_driving));
    }

    bool operator<(const NodeBasedEdgeAnnotation &other) const
    {
        return (std::tie(name_id, lane_description_id, classes, travel_mode, is_left_hand_driving) <
                std::tie(other.name_id,
                         other.lane_description_id,
                         other.classes,
                         other.travel_mode,
                         other.is_left_hand_driving));
    }
};

struct NodeBasedEdge
{
    NodeBasedEdge();

    NodeBasedEdge(NodeID source,
                  NodeID target,
                  EdgeWeight weight,
                  EdgeDuration duration,
                  EdgeDistance distance,
                  GeometryID geometry_id,
                  AnnotationID annotation_data,
                  NodeBasedEdgeClassification flags);

    bool operator<(const NodeBasedEdge &other) const;

    NodeID source;                     // 32 4
    NodeID target;                     // 32 4
    EdgeWeight weight;                 // 32 4
    EdgeDuration duration;             // 32 4
    EdgeDistance distance;             // 32 4
    GeometryID geometry_id;            // 32 4
    AnnotationID annotation_data;      // 32 4
    NodeBasedEdgeClassification flags; // 32 4
};

struct NodeBasedEdgeWithOSM : NodeBasedEdge
{
    NodeBasedEdgeWithOSM();

    NodeBasedEdgeWithOSM(OSMNodeID source,
                         OSMNodeID target,
                         EdgeWeight weight,
                         EdgeDuration duration,
                         EdgeDistance distance,
                         GeometryID geometry_id,
                         AnnotationID annotation_data,
                         NodeBasedEdgeClassification flags);

    OSMNodeID osm_source_id;
    OSMNodeID osm_target_id;
};

// Impl.

inline NodeBasedEdgeClassification::NodeBasedEdgeClassification()
    : forward(false), backward(false), is_split(false), roundabout(false), circular(false),
      startpoint(false), restricted(false)
{
}

inline NodeBasedEdge::NodeBasedEdge()
    : source(SPECIAL_NODEID), target(SPECIAL_NODEID), weight(0), duration(0), distance(0),
      annotation_data(-1)
{
}

inline NodeBasedEdge::NodeBasedEdge(NodeID source,
                                    NodeID target,
                                    EdgeWeight weight,
                                    EdgeDuration duration,
                                    EdgeDistance distance,
                                    GeometryID geometry_id,
                                    AnnotationID annotation_data,
                                    NodeBasedEdgeClassification flags)
    : source(source), target(target), weight(weight), duration(duration), distance(distance),
      geometry_id(geometry_id), annotation_data(annotation_data), flags(flags)
{
}

inline bool NodeBasedEdge::operator<(const NodeBasedEdge &other) const
{
    if (source == other.source)
    {
        if (target == other.target)
        {
            if (weight == other.weight)
            {
                return flags.forward && flags.backward &&
                       ((!other.flags.forward) || (!other.flags.backward));
            }
            return weight < other.weight;
        }
        return target < other.target;
    }
    return source < other.source;
}

inline NodeBasedEdgeWithOSM::NodeBasedEdgeWithOSM(OSMNodeID source,
                                                  OSMNodeID target,
                                                  EdgeWeight weight,
                                                  EdgeDuration duration,
                                                  EdgeDistance distance,
                                                  GeometryID geometry_id,
                                                  AnnotationID annotation_data,
                                                  NodeBasedEdgeClassification flags)
    : NodeBasedEdge(SPECIAL_NODEID,
                    SPECIAL_NODEID,
                    weight,
                    duration,
                    distance,
                    geometry_id,
                    annotation_data,
                    flags),
      osm_source_id(std::move(source)), osm_target_id(std::move(target))
{
}

inline NodeBasedEdgeWithOSM::NodeBasedEdgeWithOSM()
    : osm_source_id(MIN_OSM_NODEID), osm_target_id(MIN_OSM_NODEID)
{
}

static_assert(sizeof(extractor::NodeBasedEdge) == 32,
              "Size of extractor::NodeBasedEdge type is "
              "bigger than expected. This will influence "
              "memory consumption.");

} // ns extractor
} // ns osrm

#endif /* NODE_BASED_EDGE_HPP */
