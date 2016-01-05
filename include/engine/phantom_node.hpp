#ifndef PHANTOM_NODES_H
#define PHANTOM_NODES_H

#include "extractor/travel_mode.hpp"
#include "util/typedefs.hpp"

#include "osrm/coordinate.hpp"

#include <iostream>
#include <utility>
#include <vector>

struct PhantomNode
{
    PhantomNode(NodeID forward_node_id,
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
                TravelMode backward_travel_mode);

    PhantomNode();

    template <class OtherT> PhantomNode(const OtherT &other, const FixedPointCoordinate &foot_point)
    {
        forward_node_id = other.forward_edge_based_node_id;
        reverse_node_id = other.reverse_edge_based_node_id;
        name_id = other.name_id;

        forward_weight = other.forward_weight;
        reverse_weight = other.reverse_weight;

        forward_offset = other.forward_offset;
        reverse_offset = other.reverse_offset;

        packed_geometry_id = other.packed_geometry_id;

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
    unsigned packed_geometry_id;
    struct ComponentType
    {
        uint32_t id : 31;
        bool is_tiny : 1;
    } component;
// bit-fields are broken on Windows
#ifndef _MSC_VER
    static_assert(sizeof(ComponentType) == 4, "ComponentType needs to 4 bytes big");
#endif
    FixedPointCoordinate location;
    unsigned short fwd_segment_position;
    // note 4 bits would suffice for each,
    // but the saved byte would be padding anyway
    TravelMode forward_travel_mode;
    TravelMode backward_travel_mode;

    int GetForwardWeightPlusOffset() const;

    int GetReverseWeightPlusOffset() const;

    bool is_bidirected() const;

    bool is_compressed() const;

    bool is_valid(const unsigned numberOfNodes) const;

    bool is_valid() const;

    bool operator==(const PhantomNode &other) const;
};

#ifndef _MSC_VER
static_assert(sizeof(PhantomNode) == 48, "PhantomNode has more padding then expected");
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
        << "geom: " << pn.packed_geometry_id << ", "
        << "comp: " << pn.component.is_tiny << " / " << pn.component.id << ", "
        << "pos: " << pn.fwd_segment_position << ", "
        << "loc: " << pn.location;
    return out;
}

#endif // PHANTOM_NODES_H
