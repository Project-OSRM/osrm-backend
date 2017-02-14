#ifndef NODE_BASED_EDGE_HPP
#define NODE_BASED_EDGE_HPP

#include "extractor/travel_mode.hpp"
#include "util/typedefs.hpp"

#include "extractor/guidance/road_classification.hpp"

namespace osrm
{
namespace extractor
{

struct NodeBasedEdge
{
    NodeBasedEdge();

    NodeBasedEdge(NodeID source,
                  NodeID target,
                  NodeID name_id,
                  EdgeWeight weight,
                  EdgeWeight duration,
                  bool forward,
                  bool backward,
                  bool roundabout,
                  bool circular,
                  bool startpoint,
                  bool restricted,
                  bool is_split,
                  TravelMode travel_mode,
                  const LaneDescriptionID lane_description_id,
                  guidance::RoadClassification road_classification);

    bool operator<(const NodeBasedEdge &other) const;

    NodeID source;                                    // 32 4
    NodeID target;                                    // 32 4
    NodeID name_id;                                   // 32 4
    EdgeWeight weight;                                // 32 4
    EdgeWeight duration;                              // 32 4
    std::uint8_t forward : 1;                         // 1
    std::uint8_t backward : 1;                        // 1
    std::uint8_t roundabout : 1;                      // 1
    std::uint8_t circular : 1;                        // 1
    std::uint8_t startpoint : 1;                      // 1
    std::uint8_t restricted : 1;                      // 1
    std::uint8_t is_split : 1;                        // 1
    TravelMode travel_mode : 4;                       // 4
    LaneDescriptionID lane_description_id;            // 16 2
    guidance::RoadClassification road_classification; // 16 2
};

struct NodeBasedEdgeWithOSM : NodeBasedEdge
{
    NodeBasedEdgeWithOSM(OSMNodeID source,
                         OSMNodeID target,
                         NodeID name_id,
                         EdgeWeight weight,
                         EdgeWeight duration,
                         bool forward,
                         bool backward,
                         bool roundabout,
                         bool circular,
                         bool startpoint,
                         bool restricted,
                         bool is_split,
                         TravelMode travel_mode,
                         const LaneDescriptionID lane_description_id,
                         guidance::RoadClassification road_classification);

    OSMNodeID osm_source_id;
    OSMNodeID osm_target_id;
};

// Impl.

inline NodeBasedEdge::NodeBasedEdge()
    : source(SPECIAL_NODEID), target(SPECIAL_NODEID), name_id(0), weight(0), duration(0),
      forward(false), backward(false), roundabout(false), circular(false), startpoint(true),
      restricted(false), is_split(false), travel_mode(TRAVEL_MODE_INACCESSIBLE),
      lane_description_id(INVALID_LANE_DESCRIPTIONID)
{
}

inline NodeBasedEdge::NodeBasedEdge(NodeID source,
                                    NodeID target,
                                    NodeID name_id,
                                    EdgeWeight weight,
                                    EdgeWeight duration,
                                    bool forward,
                                    bool backward,
                                    bool roundabout,
                                    bool circular,
                                    bool startpoint,
                                    bool restricted,
                                    bool is_split,
                                    TravelMode travel_mode,
                                    const LaneDescriptionID lane_description_id,
                                    guidance::RoadClassification road_classification)
    : source(source), target(target), name_id(name_id), weight(weight), duration(duration),
      forward(forward), backward(backward), roundabout(roundabout), circular(circular),
      startpoint(startpoint), restricted(restricted), is_split(is_split), travel_mode(travel_mode),
      lane_description_id(lane_description_id), road_classification(std::move(road_classification))
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
                return forward && backward && ((!other.forward) || (!other.backward));
            }
            return weight < other.weight;
        }
        return target < other.target;
    }
    return source < other.source;
}

inline NodeBasedEdgeWithOSM::NodeBasedEdgeWithOSM(OSMNodeID source,
                                                  OSMNodeID target,
                                                  NodeID name_id,
                                                  EdgeWeight weight,
                                                  EdgeWeight duration,
                                                  bool forward,
                                                  bool backward,
                                                  bool roundabout,
                                                  bool circular,
                                                  bool startpoint,
                                                  bool restricted,
                                                  bool is_split,
                                                  TravelMode travel_mode,
                                                  const LaneDescriptionID lane_description_id,
                                                  guidance::RoadClassification road_classification)
    : NodeBasedEdge(SPECIAL_NODEID,
                    SPECIAL_NODEID,
                    name_id,
                    weight,
                    duration,
                    forward,
                    backward,
                    roundabout,
                    circular,
                    startpoint,
                    restricted,
                    is_split,
                    travel_mode,
                    lane_description_id,
                    std::move(road_classification)),
      osm_source_id(std::move(source)), osm_target_id(std::move(target))
{
}

static_assert(sizeof(extractor::NodeBasedEdge) == 28,
              "Size of extractor::NodeBasedEdge type is "
              "bigger than expected. This will influence "
              "memory consumption.");

} // ns extractor
} // ns osrm

#endif /* NODE_BASED_EDGE_HPP */
