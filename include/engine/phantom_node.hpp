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
    PhantomNode(SegmentID forward_segment_id,
                SegmentID reverse_segment_id,
                unsigned name_id,
                int forward_weight,
                int reverse_weight,
                int forward_duration,
                int reverse_duration,
                int forward_offset,
                int reverse_offset,
                unsigned packed_geometry_id_,
                bool is_tiny_component,
                unsigned component_id,
                util::Coordinate location,
                util::Coordinate input_location,
                unsigned short fwd_segment_position,
                extractor::TravelMode forward_travel_mode,
                extractor::TravelMode backward_travel_mode)
        : forward_segment_id(forward_segment_id), reverse_segment_id(reverse_segment_id),
          name_id(name_id), forward_weight(forward_weight), reverse_weight(reverse_weight),
          forward_duration(forward_duration), reverse_duration(reverse_duration),
          forward_offset(forward_offset), reverse_offset(reverse_offset),
          packed_geometry_id(packed_geometry_id_), component{component_id, is_tiny_component},
          location(std::move(location)), input_location(std::move(input_location)),
          fwd_segment_position(fwd_segment_position), forward_travel_mode(forward_travel_mode),
          backward_travel_mode(backward_travel_mode)
    {
    }

    PhantomNode()
        : forward_segment_id{SPECIAL_SEGMENTID, false},
          reverse_segment_id{SPECIAL_SEGMENTID, false},
          name_id(std::numeric_limits<unsigned>::max()), forward_weight(INVALID_EDGE_WEIGHT),
          reverse_weight(INVALID_EDGE_WEIGHT), forward_duration(INVALID_EDGE_WEIGHT),
          reverse_duration(INVALID_EDGE_WEIGHT), forward_offset(0), reverse_offset(0),
          packed_geometry_id(SPECIAL_GEOMETRYID), component{INVALID_COMPONENTID, false},
          fwd_segment_position(0), forward_travel_mode(TRAVEL_MODE_INACCESSIBLE),
          backward_travel_mode(TRAVEL_MODE_INACCESSIBLE)
    {
    }

    int GetForwardWeightPlusOffset() const
    {
        BOOST_ASSERT(forward_segment_id.enabled);
        return forward_offset + forward_weight;
    }

    int GetReverseWeightPlusOffset() const
    {
        BOOST_ASSERT(reverse_segment_id.enabled);
        return reverse_offset + reverse_weight;
    }

    bool IsBidirected() const { return forward_segment_id.enabled && reverse_segment_id.enabled; }

    bool IsValid(const unsigned number_of_nodes) const
    {
        return location.IsValid() && ((forward_segment_id.id < number_of_nodes) ||
                                      (reverse_segment_id.id < number_of_nodes)) &&
               ((forward_weight != INVALID_EDGE_WEIGHT) ||
                (reverse_weight != INVALID_EDGE_WEIGHT)) &&
               ((forward_duration != INVALID_EDGE_WEIGHT) ||
                (reverse_duration != INVALID_EDGE_WEIGHT)) &&
               (component.id != INVALID_COMPONENTID) && (name_id != INVALID_NAMEID);
    }

    bool IsValid(const unsigned number_of_nodes, const util::Coordinate queried_coordinate) const
    {
        return queried_coordinate == input_location && IsValid(number_of_nodes);
    }

    bool IsValid() const { return location.IsValid() && (name_id != INVALID_NAMEID); }

    bool operator==(const PhantomNode &other) const { return location == other.location; }

    template <class OtherT>
    explicit PhantomNode(const OtherT &other,
                         int forward_weight_,
                         int reverse_weight_,
                         int forward_duration_,
                         int reverse_duration_,
                         int forward_offset_,
                         int reverse_offset_,
                         const util::Coordinate location_,
                         const util::Coordinate input_location_)
        : forward_segment_id{other.forward_segment_id},
          reverse_segment_id{other.reverse_segment_id}, name_id{other.name_id},
          forward_weight{forward_weight_}, reverse_weight{reverse_weight_},
          forward_duration{forward_duration_}, reverse_duration{reverse_duration_},
          forward_offset{forward_offset_}, reverse_offset{reverse_offset_},
          packed_geometry_id{other.packed_geometry_id},
          component{other.component.id, other.component.is_tiny}, location{location_},
          input_location{input_location_}, fwd_segment_position{other.fwd_segment_position},
          forward_travel_mode{other.forward_travel_mode},
          backward_travel_mode{other.backward_travel_mode}
    {
    }

    SegmentID forward_segment_id;
    SegmentID reverse_segment_id;
    unsigned name_id;
    int forward_weight;
    int reverse_weight;
    int forward_duration;
    int reverse_duration;
    int forward_offset;
    int reverse_offset;
    unsigned packed_geometry_id;
    struct ComponentType
    {
        std::uint32_t id : 31;
        std::uint32_t is_tiny : 1;
    } component;
    static_assert(sizeof(ComponentType) == 4, "ComponentType needs to be 4 bytes big");

    util::Coordinate location;
    util::Coordinate input_location;
    unsigned short fwd_segment_position;
    // note 4 bits would suffice for each,
    // but the saved byte would be padding anyway
    extractor::TravelMode forward_travel_mode;
    extractor::TravelMode backward_travel_mode;
};

static_assert(sizeof(PhantomNode) == 64, "PhantomNode has more padding then expected");

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
    out << "node1: " << pn.forward_segment_id.id << ", "
        << "node2: " << pn.reverse_segment_id.id << ", "
        << "name: " << pn.name_id << ", "
        << "fwd-w: " << pn.forward_weight << ", "
        << "rev-w: " << pn.reverse_weight << ", "
        << "fwd-d: " << pn.forward_duration << ", "
        << "rev-d: " << pn.reverse_duration << ", "
        << "fwd-o: " << pn.forward_offset << ", "
        << "rev-o: " << pn.reverse_offset << ", "
        << "geom: " << pn.packed_geometry_id << ", "
        << "comp: " << pn.component.is_tiny << " / " << pn.component.id << ", "
        << "pos: " << pn.fwd_segment_position << ", "
        << "loc: " << pn.location;
    return out;
}
}
}

#endif // PHANTOM_NODES_H
