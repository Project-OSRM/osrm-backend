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
          component{INVALID_COMPONENTID, false},
          fwd_segment_position(std::numeric_limits<unsigned short>::max())
    {
    }

    explicit EdgeBasedNode(const SegmentID forward_segment_id_,
                           const SegmentID reverse_segment_id_,
                           NodeID u,
                           NodeID v,
                           bool is_tiny_component,
                           unsigned component_id,
                           unsigned short fwd_segment_position)
        : forward_segment_id(forward_segment_id_), reverse_segment_id(reverse_segment_id_), u(u),
          v(v), component{component_id, is_tiny_component},
          fwd_segment_position(fwd_segment_position)
    {
        BOOST_ASSERT(forward_segment_id.enabled || reverse_segment_id.enabled);
    }

    SegmentID forward_segment_id; // edge-based graph node ID in forward direction (u->v)
    SegmentID reverse_segment_id; // edge-based graph node ID in reverse direction (v->u if exists)
    NodeID u;                     // node-based graph node ID of the start node
    NodeID v;                     // node-based graph node ID of the target node

    struct
    {
        unsigned id : 31;
        bool is_tiny : 1;
    } component;
    unsigned short fwd_segment_position; // segment id in a compressed geometry
};
}
}

#endif // EDGE_BASED_NODE_HPP
