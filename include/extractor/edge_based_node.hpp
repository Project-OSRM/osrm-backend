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

struct RoadSegment
{
    RoadSegment() : edge_based_node_id{SPECIAL_NODEID}, fwd_segment_position{0} {}
    RoadSegment(NodeID edge_based_node_id_, unsigned short fwd_segment_pos_, NodeID u_, NodeID v_)
        : edge_based_node_id{edge_based_node_id_}, fwd_segment_position{fwd_segment_pos_}
    {
    }
    NodeID edge_based_node_id; // edge-based-node-id
    unsigned short fwd_segment_position;
};

/// This is what util::StaticRTree serialized and stores on disk
/// It is generated in EdgeBasedGraphFactory.
struct EdgeBasedNode
{
    EdgeBasedNode()
        : forward_segment_id{SPECIAL_SEGMENTID, false},
          reverse_segment_id{SPECIAL_SEGMENTID, false}, name_id(0),
          packed_geometry_id(SPECIAL_GEOMETRYID), component{INVALID_COMPONENTID, false},
          forward_travel_mode(TRAVEL_MODE_INACCESSIBLE),
          backward_travel_mode(TRAVEL_MODE_INACCESSIBLE)
    {
    }

    explicit EdgeBasedNode(const SegmentID forward_segment_id_,
                           const SegmentID reverse_segment_id_,
                           unsigned name_id,
                           unsigned packed_geometry_id_,
                           bool is_tiny_component,
                           unsigned component_id,
                           TravelMode forward_travel_mode,
                           TravelMode backward_travel_mode)
        : forward_segment_id(forward_segment_id_), reverse_segment_id(reverse_segment_id_),
          name_id(name_id), packed_geometry_id(packed_geometry_id_),
          component{component_id, is_tiny_component}, forward_travel_mode(forward_travel_mode),
          backward_travel_mode(backward_travel_mode)
    {
        BOOST_ASSERT(forward_segment_id.enabled || reverse_segment_id.enabled);
    }

    SegmentID forward_segment_id; // needed for edge-expanded graph
    SegmentID reverse_segment_id; // needed for edge-expanded graph
    unsigned name_id;             // id of the edge name

    unsigned packed_geometry_id;
    struct
    {
        unsigned id : 31;
        bool is_tiny : 1;
    } component;
    TravelMode forward_travel_mode : 4;
    TravelMode backward_travel_mode : 4;
};
}
}

#endif // EDGE_BASED_NODE_HPP
