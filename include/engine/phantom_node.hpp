/*

Copyright (c) 2016, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef PHANTOM_NODES_H
#define PHANTOM_NODES_H

#include "extractor/travel_mode.hpp"
#include "extractor/edge_based_node.hpp"
#include "util/typedefs.hpp"

#include "util/coordinate.hpp"

#include <boost/assert.hpp>

#include <iostream>
#include <utility>
#include <vector>

namespace osrm
{
namespace engine
{

struct PhantomNode
{
    PhantomNode(const extractor::EdgeBasedNode &edge_data,
                int forward_weight,
                int reverse_weight,
                int forward_offset,
                int reverse_offset,
                util::Coordinate location,
                util::Coordinate input_location)
        : edge_data(edge_data), forward_weight(forward_weight), reverse_weight(reverse_weight),
          forward_offset(forward_offset), reverse_offset(reverse_offset),
          location(std::move(location)), input_location(std::move(input_location))
    {
    }

    PhantomNode()
        : edge_data(extractor::EdgeBasedNode()), forward_weight(INVALID_EDGE_WEIGHT),
          reverse_weight(INVALID_EDGE_WEIGHT), forward_offset(0),
          reverse_offset(0)
    {
    }

    int GetForwardWeightPlusOffset() const
    {
        BOOST_ASSERT(edge_data.forward_segment_id.enabled);
        return forward_offset + forward_weight;
    }

    int GetReverseWeightPlusOffset() const
    {
        BOOST_ASSERT(edge_data.reverse_segment_id.enabled);
        return reverse_offset + reverse_weight;
    }

    bool IsBidirected() const { return edge_data.forward_segment_id.enabled && edge_data.reverse_segment_id.enabled; }

    bool IsValid(const unsigned number_of_nodes) const
    {
        return location.IsValid() && ((edge_data.forward_segment_id.id < number_of_nodes) ||
                                      (edge_data.reverse_segment_id.id < number_of_nodes)) &&
               ((forward_weight != INVALID_EDGE_WEIGHT) ||
                (reverse_weight != INVALID_EDGE_WEIGHT)) &&
               (edge_data.component.id != INVALID_COMPONENTID) && (edge_data.name_id != INVALID_NAMEID);
    }

    bool IsValid(const unsigned number_of_nodes, const util::Coordinate queried_coordinate) const
    {
        return queried_coordinate == input_location && IsValid(number_of_nodes);
    }

    bool IsValid() const { return location.IsValid() && (edge_data.name_id != INVALID_NAMEID); }

    bool operator==(const PhantomNode &other) const { return location == other.location; }

    extractor::EdgeBasedNode edge_data;
    int forward_weight;
    int reverse_weight;
    int forward_offset;
    int reverse_offset;
    struct ComponentType
    {
        std::uint32_t id : 31;
        std::uint32_t is_tiny : 1;
    } component;
    static_assert(sizeof(ComponentType) == 4, "ComponentType needs to be 4 bytes big");

    util::Coordinate location;
    util::Coordinate input_location;
};

// TODO how big should PhantomNodes be now?
//static_assert(sizeof(PhantomNode) == 60, "PhantomNode has more padding then expected");

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
    out << "node1: " << pn.edge_data.forward_segment_id.id << ", "
        << "node2: " << pn.edge_data.reverse_segment_id.id << ", "
        << "name: " << pn.edge_data.name_id << ", "
        << "fwd-w: " << pn.forward_weight << ", "
        << "rev-w: " << pn.reverse_weight << ", "
        << "fwd-o: " << pn.forward_offset << ", "
        << "rev-o: " << pn.reverse_offset << ", "
        << "fwd_geom: " << pn.edge_data.forward_packed_geometry_id << ", "
        << "rev_geom: " << pn.edge_data.reverse_packed_geometry_id << ", "
        << "comp: " << pn.component.is_tiny << " / " << pn.component.id << ", "
        << "pos: " << pn.edge_data.fwd_segment_position << ", "
        << "loc: " << pn.location;
    return out;
}
}
}

#endif // PHANTOM_NODES_H
