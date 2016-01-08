#ifndef EDGE_BASED_NODE_HPP
#define EDGE_BASED_NODE_HPP

#include "extractor/travel_mode.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include "osrm/coordinate.hpp"

#include <limits>

namespace osrm
{
namespace extractor
{

/// This is what util::StaticRTree serialized and stores on disk
/// It is generated in EdgeBasedGraphFactory.
struct EdgeBasedNode
{
    EdgeBasedNode()
        : forward_edge_based_node_id(SPECIAL_NODEID), reverse_edge_based_node_id(SPECIAL_NODEID),
          u(SPECIAL_NODEID), v(SPECIAL_NODEID), name_id(0),
          forward_weight(INVALID_EDGE_WEIGHT >> 1), reverse_weight(INVALID_EDGE_WEIGHT >> 1),
          forward_offset(0), reverse_offset(0), packed_geometry_id(SPECIAL_EDGEID),
          component{INVALID_COMPONENTID, false},
          fwd_segment_position(std::numeric_limits<unsigned short>::max()),
          forward_travel_mode(TRAVEL_MODE_INACCESSIBLE),
          backward_travel_mode(TRAVEL_MODE_INACCESSIBLE)
    {
    }

    explicit EdgeBasedNode(NodeID forward_edge_based_node_id,
                           NodeID reverse_edge_based_node_id,
                           NodeID u,
                           NodeID v,
                           unsigned name_id,
                           int forward_weight,
                           int reverse_weight,
                           int forward_offset,
                           int reverse_offset,
                           unsigned packed_geometry_id,
                           bool is_tiny_component,
                           unsigned component_id,
                           unsigned short fwd_segment_position,
                           TravelMode forward_travel_mode,
                           TravelMode backward_travel_mode)
        : forward_edge_based_node_id(forward_edge_based_node_id),
          reverse_edge_based_node_id(reverse_edge_based_node_id), u(u), v(v), name_id(name_id),
          forward_weight(forward_weight), reverse_weight(reverse_weight),
          forward_offset(forward_offset), reverse_offset(reverse_offset),
          packed_geometry_id(packed_geometry_id), component{component_id, is_tiny_component},
          fwd_segment_position(fwd_segment_position), forward_travel_mode(forward_travel_mode),
          backward_travel_mode(backward_travel_mode)
    {
        BOOST_ASSERT((forward_edge_based_node_id != SPECIAL_NODEID) ||
                     (reverse_edge_based_node_id != SPECIAL_NODEID));
    }

    static inline util::FixedPointCoordinate Centroid(const util::FixedPointCoordinate &a,
                                                      const util::FixedPointCoordinate &b)
    {
        util::FixedPointCoordinate centroid;
        // The coordinates of the midpoint are given by:
        centroid.lat = (a.lat + b.lat) / 2;
        centroid.lon = (a.lon + b.lon) / 2;
        return centroid;
    }

    bool IsCompressed() const { return packed_geometry_id != SPECIAL_EDGEID; }

    NodeID forward_edge_based_node_id; // needed for edge-expanded graph
    NodeID reverse_edge_based_node_id; // needed for edge-expanded graph
    NodeID u;                          // indices into the coordinates array
    NodeID v;                          // indices into the coordinates array
    unsigned name_id;                  // id of the edge name
    int forward_weight;                // weight of the edge
    int reverse_weight;                // weight in the other direction (may be different)
    int forward_offset;          // prefix sum of the weight up the edge TODO: short must suffice
    int reverse_offset;          // prefix sum of the weight from the edge TODO: short must suffice
    unsigned packed_geometry_id; // if set, then the edge represents a packed geometry
    struct
    {
        unsigned id : 31;
        bool is_tiny : 1;
    } component;
    unsigned short fwd_segment_position; // segment id in a compressed geometry
    TravelMode forward_travel_mode : 4;
    TravelMode backward_travel_mode : 4;
};
}
}

#endif // EDGE_BASED_NODE_HPP
