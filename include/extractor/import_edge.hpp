#ifndef IMPORT_EDGE_HPP
#define IMPORT_EDGE_HPP

#include "extractor/travel_mode.hpp"
#include "util/typedefs.hpp"

namespace osrm
{
namespace extractor
{

struct NodeBasedEdge
{
    bool operator<(const NodeBasedEdge &other) const
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

    NodeBasedEdge()
        : source(SPECIAL_NODEID), target(SPECIAL_NODEID), name_id(0), weight(0), forward(false),
          backward(false), roundabout(false), access_restricted(false), startpoint(true),
          is_split(false), travel_mode(false)
    {
    }

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
                  bool is_split)
        : source(source), target(target), name_id(name_id), weight(weight), forward(forward),
          backward(backward), roundabout(roundabout), access_restricted(access_restricted),
          startpoint(startpoint), is_split(is_split), travel_mode(travel_mode)
    {
    }

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
};

struct NodeBasedEdgeWithOSM : NodeBasedEdge
{
    explicit NodeBasedEdgeWithOSM(OSMNodeID source,
                                  OSMNodeID target,
                                  NodeID name_id,
                                  EdgeWeight weight,
                                  bool forward,
                                  bool backward,
                                  bool roundabout,
                                  bool access_restricted,
                                  bool startpoint,
                                  TravelMode travel_mode,
                                  bool is_split)
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
                        is_split),
          osm_source_id(source), osm_target_id(target)
    {
    }

    OSMNodeID osm_source_id;
    OSMNodeID osm_target_id;
};

struct EdgeBasedEdge
{

  public:
    bool operator<(const EdgeBasedEdge &other) const
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
    template <class EdgeT>
    explicit EdgeBasedEdge(const EdgeT &other)
        : source(other.source), target(other.target), edge_id(other.data.via),
          weight(other.data.distance), forward(other.data.forward), backward(other.data.backward)
    {
    }

    EdgeBasedEdge() : source(0), target(0), edge_id(0), weight(0), forward(false), backward(false)
    {
    }

    EdgeBasedEdge(const NodeID source,
                  const NodeID target,
                  const NodeID edge_id,
                  const EdgeWeight weight,
                  const bool forward,
                  const bool backward)
        : source(source), target(target), edge_id(edge_id), weight(weight), forward(forward),
          backward(backward)
    {
    }

    NodeID source;
    NodeID target;
    NodeID edge_id;
    EdgeWeight weight : 30;
    bool forward : 1;
    bool backward : 1;
};
}
}

#endif /* IMPORT_EDGE_HPP */
