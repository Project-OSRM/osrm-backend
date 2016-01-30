#ifndef PHANTOM_NODES_H
#define PHANTOM_NODES_H

#include "extractor/travel_mode.hpp"
#include "util/typedefs.hpp"

#include "osrm/coordinate.hpp"

#include <iostream>
#include <utility>
#include <vector>

namespace osrm
{
namespace engine
{

struct PhantomNode
{
    PhantomNode(NodeID forward_node_id,
                NodeID reverse_node_id,
                unsigned name_id,
                int forward_weight,
                int reverse_weight,
                int forward_offset,
                int reverse_offset,
                unsigned forward_packed_geometry_id_,
                unsigned reverse_packed_geometry_id_,
                bool is_tiny_component,
                unsigned component_id,
                util::FixedPointCoordinate location,
                unsigned short fwd_segment_position,
                extractor::TravelMode forward_travel_mode,
                extractor::TravelMode backward_travel_mode)
        : forward_node_id(forward_node_id), reverse_node_id(reverse_node_id), name_id(name_id),
          forward_weight(forward_weight), reverse_weight(reverse_weight),
          forward_offset(forward_offset), reverse_offset(reverse_offset),
          forward_packed_geometry_id(forward_packed_geometry_id_),
          reverse_packed_geometry_id(reverse_packed_geometry_id_),
          component{component_id, is_tiny_component},
          location(std::move(location)), fwd_segment_position(fwd_segment_position),
          forward_travel_mode(forward_travel_mode), backward_travel_mode(backward_travel_mode)
    {
    }

    PhantomNode()
        : forward_node_id(SPECIAL_NODEID), reverse_node_id(SPECIAL_NODEID),
          name_id(std::numeric_limits<unsigned>::max()), forward_weight(INVALID_EDGE_WEIGHT),
          reverse_weight(INVALID_EDGE_WEIGHT), forward_offset(0), reverse_offset(0),
          forward_packed_geometry_id(SPECIAL_EDGEID),
          reverse_packed_geometry_id(SPECIAL_EDGEID),
          component{INVALID_COMPONENTID, false},
          fwd_segment_position(0), forward_travel_mode(TRAVEL_MODE_INACCESSIBLE),
          backward_travel_mode(TRAVEL_MODE_INACCESSIBLE)
    {
    }

    int GetForwardWeightPlusOffset() const
    {
        if (SPECIAL_NODEID == forward_node_id)
        {
            return 0;
        }
        return forward_offset + forward_weight;
    }

    int GetReverseWeightPlusOffset() const
    {
        if (SPECIAL_NODEID == reverse_node_id)
        {
            return 0;
        }
        return reverse_offset + reverse_weight;
    }

    bool IsBidirected() const
    {
        return (forward_node_id != SPECIAL_NODEID) && (reverse_node_id != SPECIAL_NODEID);
    }

    bool IsCompressed() const { return (forward_offset != 0) || (reverse_offset != 0); }

    bool IsValid(const unsigned number_of_nodes) const
    {
        return location.IsValid() &&
               ((forward_node_id < number_of_nodes) || (reverse_node_id < number_of_nodes)) &&
               ((forward_weight != INVALID_EDGE_WEIGHT) ||
                (reverse_weight != INVALID_EDGE_WEIGHT)) &&
               (component.id != INVALID_COMPONENTID) && (name_id != INVALID_NAMEID);
    }

    bool IsValid() const { return location.IsValid() && (name_id != INVALID_NAMEID); }

    bool operator==(const PhantomNode &other) const { return location == other.location; }

    template <class OtherT>
    PhantomNode(const OtherT &other, int forward_weight_, int forward_offset_, int reverse_weight_, int reverse_offset_, const util::FixedPointCoordinate foot_point)
    {
        forward_node_id = other.forward_edge_based_node_id;
        reverse_node_id = other.reverse_edge_based_node_id;
        name_id = other.name_id;

        forward_weight = forward_weight_;
        reverse_weight = reverse_weight_;

        forward_offset = forward_offset_;
        reverse_offset = reverse_offset_;

        forward_packed_geometry_id = other.forward_packed_geometry_id;
        reverse_packed_geometry_id = other.reverse_packed_geometry_id;

        component.id = other.component.id;
        component.is_tiny = other.component.is_tiny;

        location = foot_point;
        fwd_segment_position = other.fwd_segment_position;

        forward_travel_mode = other.forward_travel_mode;
        backward_travel_mode = other.backward_travel_mode;
    }

    NodeID forward_node_id;
    NodeID reverse_node_id;
    unsigned name_id;
    int forward_weight;
    int reverse_weight;
    int forward_offset;
    int reverse_offset;
    unsigned forward_packed_geometry_id;
    unsigned reverse_packed_geometry_id;
    struct ComponentType
    {
        uint32_t id : 31;
        bool is_tiny : 1;
    } component;
// bit-fields are broken on Windows
#ifndef _MSC_VER
    static_assert(sizeof(ComponentType) == 4, "ComponentType needs to 4 bytes big");
#endif
    util::FixedPointCoordinate location;
    unsigned short fwd_segment_position;
    // note 4 bits would suffice for each,
    // but the saved byte would be padding anyway
    extractor::TravelMode forward_travel_mode;
    extractor::TravelMode backward_travel_mode;
};

#ifndef _MSC_VER
static_assert(sizeof(PhantomNode) == 52, "PhantomNode has more padding then expected");
#endif

using PhantomNodePair = std::pair<PhantomNode, PhantomNode>;

struct PhantomNodeWithDistance
{
    PhantomNode phantom_node;
    double distance;
};

struct PhantomNodes
{
    PhantomNode source_phantom;
    PhantomNode target_phantom;
};

inline std::ostream &operator<<(std::ostream &out, const PhantomNodes &pn)
{
    out << "source_coord: " << pn.source_phantom.location << "\n";
    out << "target_coord: " << pn.target_phantom.location << std::endl;
    return out;
}

inline std::ostream &operator<<(std::ostream &out, const PhantomNode &pn)
{
    out << "node1: " << pn.forward_node_id << ", "
        << "node2: " << pn.reverse_node_id << ", "
        << "name: " << pn.name_id << ", "
        << "fwd-w: " << pn.forward_weight << ", "
        << "rev-w: " << pn.reverse_weight << ", "
        << "fwd-o: " << pn.forward_offset << ", "
        << "rev-o: " << pn.reverse_offset << ", "
        << "fwd_geom: " << pn.forward_packed_geometry_id << ", "
        << "rev_geom: " << pn.reverse_packed_geometry_id << ", "
        << "comp: " << pn.component.is_tiny << " / " << pn.component.id << ", "
        << "pos: " << pn.fwd_segment_position << ", "
        << "loc: " << pn.location;
    return out;
}
}
}

#endif // PHANTOM_NODES_H
