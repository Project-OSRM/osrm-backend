#ifndef OSRM_EXTRACT_EDGE_BASED_NODE_SEGMENT_HPP
#define OSRM_EXTRACT_EDGE_BASED_NODE_SEGMENT_HPP

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
struct EdgeBasedNodeSegment
{
    EdgeBasedNodeSegment()
        : forward_segment_id{SPECIAL_SEGMENTID, false},
          reverse_segment_id{SPECIAL_SEGMENTID, false}, u(SPECIAL_NODEID), v(SPECIAL_NODEID),
          fwd_segment_position(std::numeric_limits<unsigned short>::max() >>
                               1), // >> 1 because we've only got 15 bits
          is_startpoint(false)
    {
    }

    explicit EdgeBasedNodeSegment(const SegmentID forward_segment_id_,
                                  const SegmentID reverse_segment_id_,
                                  NodeID u,
                                  NodeID v,
                                  unsigned short fwd_segment_position,
                                  bool is_startpoint_)
        : forward_segment_id(forward_segment_id_), reverse_segment_id(reverse_segment_id_), u(u),
          v(v), fwd_segment_position(fwd_segment_position), is_startpoint(is_startpoint_)
    {
        BOOST_ASSERT(forward_segment_id.enabled || reverse_segment_id.enabled);
    }

    SegmentID forward_segment_id; // edge-based graph node ID in forward direction (u->v)
    SegmentID reverse_segment_id; // edge-based graph node ID in reverse direction (v->u if exists)
    NodeID u;                     // node-based graph node ID of the start node
    NodeID v;                     // node-based graph node ID of the target node
    unsigned short fwd_segment_position : 15; // segment id in a compressed geometry
    bool is_startpoint : 1;
};
}
}

#endif // OSRM_EXTRACT_EDGE_BASED_NODE_SEGMENT_HPP
