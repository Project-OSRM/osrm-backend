#ifndef NODE_BASED_EDGE_HPP
#define NODE_BASED_EDGE_HPP

#include "extractor/travel_mode.hpp"
#include "util/typedefs.hpp"

#include "extractor/guidance/classification_data.hpp"

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
                  bool forward,
                  bool backward,
                  bool roundabout,
                  bool access_restricted,
                  bool startpoint,
                  TravelMode travel_mode,
                  bool is_split,
                  guidance::RoadClassificationData road_classification);

    bool operator<(const NodeBasedEdge &other) const;

    NodeID source;
    NodeID target;
    NodeID name_id;
    EdgeWeight weight;
    bool forward : 1;
    bool backward : 1;
    bool roundabout : 1;
    bool access_restricted : 1;
    bool startpoint : 1;
    bool is_split : 1;
    TravelMode travel_mode : 4;
    guidance::RoadClassificationData road_classification;
};

struct NodeBasedEdgeWithOSM : NodeBasedEdge
{
    NodeBasedEdgeWithOSM(OSMNodeID source,
                         OSMNodeID target,
                         NodeID name_id,
                         EdgeWeight weight,
                         bool forward,
                         bool backward,
                         bool roundabout,
                         bool access_restricted,
                         bool startpoint,
                         TravelMode travel_mode,
                         bool is_split,
                         guidance::RoadClassificationData road_classification);

    OSMNodeID osm_source_id;
    OSMNodeID osm_target_id;
};

// Impl.

inline NodeBasedEdge::NodeBasedEdge()
    : source(SPECIAL_NODEID), target(SPECIAL_NODEID), name_id(0), weight(0), forward(false),
      backward(false), roundabout(false), access_restricted(false), startpoint(true),
      is_split(false), travel_mode(false)
{
}

inline NodeBasedEdge::NodeBasedEdge(NodeID source,
                                    NodeID target,
                                    NodeID name_id,
                                    EdgeWeight weight,
                                    bool forward,
                                    bool backward,
                                    bool roundabout,
                                    bool access_restricted,
                                    bool startpoint,
                                    TravelMode travel_mode,
                                    bool is_split,
                                    guidance::RoadClassificationData road_classification)
    : source(source), target(target), name_id(name_id), weight(weight), forward(forward),
      backward(backward), roundabout(roundabout), access_restricted(access_restricted),
      startpoint(startpoint), is_split(is_split), travel_mode(travel_mode),
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

inline NodeBasedEdgeWithOSM::NodeBasedEdgeWithOSM(
    OSMNodeID source,
    OSMNodeID target,
    NodeID name_id,
    EdgeWeight weight,
    bool forward,
    bool backward,
    bool roundabout,
    bool access_restricted,
    bool startpoint,
    TravelMode travel_mode,
    bool is_split,
    guidance::RoadClassificationData road_classification)
    : NodeBasedEdge(SPECIAL_NODEID,
                    SPECIAL_NODEID,
                    name_id,
                    weight,
                    forward,
                    backward,
                    roundabout,
                    access_restricted,
                    startpoint,
                    travel_mode,
                    is_split,
                    std::move(road_classification)),
      osm_source_id(std::move(source)), osm_target_id(std::move(target))
{
}

} // ns extractor
} // ns osrm

#endif /* NODE_BASED_EDGE_HPP */
