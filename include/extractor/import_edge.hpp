#ifndef IMPORT_EDGE_HPP
#define IMPORT_EDGE_HPP

#include "extractor/travel_mode.hpp"
#include "util/typedefs.hpp"

struct NodeBasedEdge
{
    bool operator<(const NodeBasedEdge &e) const;

    NodeBasedEdge();
    explicit NodeBasedEdge(NodeID source,
                           NodeID target,
                           NodeID name_id,
                           EdgeWeight weight,
                           bool forward,
                           bool backward,
                           bool roundabout,
                           bool access_restricted,
                           bool startpoint,
                           TravelMode travel_mode,
                           bool is_split);

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
        : NodeBasedEdge(SPECIAL_NODEID, SPECIAL_NODEID, name_id, weight, forward, backward, roundabout, access_restricted, startpoint, travel_mode, is_split),
        osm_source_id(source), osm_target_id(target) {}

    OSMNodeID osm_source_id;
    OSMNodeID osm_target_id;
};

struct EdgeBasedEdge
{

  public:
    bool operator<(const EdgeBasedEdge &e) const;

    template <class EdgeT> explicit EdgeBasedEdge(const EdgeT &myEdge);

    EdgeBasedEdge();

    explicit EdgeBasedEdge(const NodeID source,
                           const NodeID target,
                           const NodeID edge_id,
                           const EdgeWeight weight,
                           const bool forward,
                           const bool backward);
    NodeID source;
    NodeID target;
    NodeID edge_id;
    EdgeWeight weight : 30;
    bool forward : 1;
    bool backward : 1;
};

#endif /* IMPORT_EDGE_HPP */
