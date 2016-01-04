#include "engine/phantom_node.hpp"

#include "util/typedefs.hpp"
#include "extractor/travel_mode.hpp"

#include "osrm/coordinate.hpp"

#include <limits>

PhantomNode::PhantomNode(NodeID forward_node_id,
                         NodeID reverse_node_id,
                         unsigned name_id,
                         int forward_weight,
                         int reverse_weight,
                         int forward_offset,
                         int reverse_offset,
                         unsigned packed_geometry_id,
                         bool is_tiny_component,
                         unsigned component_id,
                         FixedPointCoordinate &location,
                         unsigned short fwd_segment_position,
                         TravelMode forward_travel_mode,
                         TravelMode backward_travel_mode)
    : forward_node_id(forward_node_id), reverse_node_id(reverse_node_id), name_id(name_id),
      forward_weight(forward_weight), reverse_weight(reverse_weight),
      forward_offset(forward_offset), reverse_offset(reverse_offset),
      packed_geometry_id(packed_geometry_id), component{component_id, is_tiny_component}, location(location),
      fwd_segment_position(fwd_segment_position), forward_travel_mode(forward_travel_mode),
      backward_travel_mode(backward_travel_mode)
{
}

PhantomNode::PhantomNode()
    : forward_node_id(SPECIAL_NODEID), reverse_node_id(SPECIAL_NODEID),
      name_id(std::numeric_limits<unsigned>::max()), forward_weight(INVALID_EDGE_WEIGHT),
      reverse_weight(INVALID_EDGE_WEIGHT), forward_offset(0), reverse_offset(0),
      packed_geometry_id(SPECIAL_EDGEID), component{INVALID_COMPONENTID, false},
      fwd_segment_position(0), forward_travel_mode(TRAVEL_MODE_INACCESSIBLE),
      backward_travel_mode(TRAVEL_MODE_INACCESSIBLE)
{
}

int PhantomNode::GetForwardWeightPlusOffset() const
{
    if (SPECIAL_NODEID == forward_node_id)
    {
        return 0;
    }
    return forward_offset + forward_weight;
}

int PhantomNode::GetReverseWeightPlusOffset() const
{
    if (SPECIAL_NODEID == reverse_node_id)
    {
        return 0;
    }
    return reverse_offset + reverse_weight;
}

bool PhantomNode::is_bidirected() const
{
    return (forward_node_id != SPECIAL_NODEID) && (reverse_node_id != SPECIAL_NODEID);
}

bool PhantomNode::is_compressed() const { return (forward_offset != 0) || (reverse_offset != 0); }

bool PhantomNode::is_valid(const unsigned number_of_nodes) const
{
    return location.is_valid() &&
           ((forward_node_id < number_of_nodes) || (reverse_node_id < number_of_nodes)) &&
           ((forward_weight != INVALID_EDGE_WEIGHT) || (reverse_weight != INVALID_EDGE_WEIGHT)) &&
           (component.id != INVALID_COMPONENTID) && (name_id != INVALID_NAMEID);
}

bool PhantomNode::is_valid() const { return location.is_valid() && (name_id != INVALID_NAMEID); }

bool PhantomNode::operator==(const PhantomNode &other) const { return location == other.location; }
