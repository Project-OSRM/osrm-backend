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
        : forward_segment_id{SPECIAL_SEGMENTID, false},
          reverse_segment_id{SPECIAL_SEGMENTID, false}, u(SPECIAL_NODEID), v(SPECIAL_NODEID),
          name_id(0), packed_geometry_id(SPECIAL_GEOMETRYID), component{INVALID_COMPONENTID, false},
          fwd_segment_position(std::numeric_limits<unsigned short>::max()),
          forward_travel_mode(TRAVEL_MODE_INACCESSIBLE),
          backward_travel_mode(TRAVEL_MODE_INACCESSIBLE)
    {
    }

    explicit EdgeBasedNode(const SegmentID forward_segment_id_,
                           const SegmentID reverse_segment_id_,
                           NodeID u,
                           NodeID v,
                           unsigned name_id,
                           unsigned packed_geometry_id_,
                           bool is_tiny_component,
                           unsigned component_id,
                           unsigned short fwd_segment_position,
                           TravelMode forward_travel_mode,
                           TravelMode backward_travel_mode)
        : forward_segment_id(forward_segment_id_), reverse_segment_id(reverse_segment_id_), u(u),
          v(v), name_id(name_id), packed_geometry_id(packed_geometry_id_),
          component{component_id, is_tiny_component}, fwd_segment_position(fwd_segment_position),
          forward_travel_mode(forward_travel_mode), backward_travel_mode(backward_travel_mode)
    {
        BOOST_ASSERT(forward_segment_id.enabled || reverse_segment_id.enabled);
    }

    SegmentID forward_segment_id; // needed for edge-expanded graph
    SegmentID reverse_segment_id; // needed for edge-expanded graph
    NodeID u;                     // indices into the coordinates array
    NodeID v;                     // indices into the coordinates array
    unsigned name_id;             // id of the edge name

    unsigned packed_geometry_id;
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
