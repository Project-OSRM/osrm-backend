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
                  bool access_restricted,
                  bool startpoint,
                  bool is_split,
                  TravelMode travel_mode,
                  const LaneDescriptionID lane_description_id,
                  guidance::RoadClassification road_classification);

    bool operator<(const NodeBasedEdge &other) const;

    NodeID source;
    NodeID target;
    NodeID name_id;
    EdgeWeight weight;
    EdgeWeight duration;
    std::uint8_t forward : 1;
    std::uint8_t backward : 1;
    std::uint8_t roundabout : 1;
    std::uint8_t access_restricted : 1;
    std::uint8_t startpoint : 1;
    std::uint8_t is_split : 1;
    TravelMode travel_mode : 4;
    LaneDescriptionID lane_description_id;
    guidance::RoadClassification road_classification;
};

struct NodeBasedEdgeWithOSM : NodeBasedEdge
{
    NodeBasedEdgeWithOSM() = default;
    NodeBasedEdgeWithOSM(OSMNodeID source,
                         OSMNodeID target,
                         NodeID name_id,
                         EdgeWeight weight,
                         EdgeWeight duration,
                         bool forward,
                         bool backward,
                         bool roundabout,
                         bool access_restricted,
                         bool startpoint,
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
      forward(false), backward(false), roundabout(false), access_restricted(false),
      startpoint(true), is_split(false), travel_mode(TRAVEL_MODE_INACCESSIBLE),
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
                                    bool access_restricted,
                                    bool startpoint,
                                    bool is_split,
                                    TravelMode travel_mode,
                                    const LaneDescriptionID lane_description_id,
                                    guidance::RoadClassification road_classification)
    : source(source), target(target), name_id(name_id), weight(weight), duration(duration),
      forward(forward), backward(backward), roundabout(roundabout),
      access_restricted(access_restricted), startpoint(startpoint), is_split(is_split),
      travel_mode(travel_mode), lane_description_id(lane_description_id),
      road_classification(std::move(road_classification))
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
                                                  bool access_restricted,
                                                  bool startpoint,
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
                    access_restricted,
                    startpoint,
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
