/*

Copyright (c) 2017, Project OSRM contributors
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

#ifndef OSRM_ENGINE_PHANTOM_NODES_H
#define OSRM_ENGINE_PHANTOM_NODES_H

#include "extractor/travel_mode.hpp"

#include "util/bearing.hpp"
#include "util/coordinate.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace engine
{

struct PhantomNode
{
    PhantomNode()
        : forward_segment_id{SPECIAL_SEGMENTID, false}, reverse_segment_id{SPECIAL_SEGMENTID,
                                                                           false},
          forward_weight(INVALID_EDGE_WEIGHT), reverse_weight(INVALID_EDGE_WEIGHT),
          forward_weight_offset(0), reverse_weight_offset(0),
          forward_distance(INVALID_EDGE_DISTANCE), reverse_distance(INVALID_EDGE_DISTANCE),
          forward_distance_offset(0), reverse_distance_offset(0),
          forward_duration(MAXIMAL_EDGE_DURATION), reverse_duration(MAXIMAL_EDGE_DURATION),
          forward_duration_offset(0), reverse_duration_offset(0),
          fwd_segment_position(0), is_valid_forward_source{false}, is_valid_forward_target{false},
          is_valid_reverse_source{false}, is_valid_reverse_target{false}, bearing(0)

    {
    }

    EdgeWeight GetForwardWeightPlusOffset() const
    {
        BOOST_ASSERT(forward_segment_id.enabled);
        return forward_weight_offset + forward_weight;
    }

    EdgeWeight GetReverseWeightPlusOffset() const
    {
        BOOST_ASSERT(reverse_segment_id.enabled);
        return reverse_weight_offset + reverse_weight;
    }

    EdgeWeight GetForwardDuration() const
    {
        BOOST_ASSERT(forward_segment_id.enabled);
        return forward_duration + forward_duration_offset;
    }

    EdgeWeight GetReverseDuration() const
    {
        BOOST_ASSERT(reverse_segment_id.enabled);
        return reverse_duration + reverse_duration_offset;
    }

    // DO THIS FOR DISTANCE

    EdgeDistance GetForwardDistance() const
    {
        // .....                  <-- forward_distance
        //      ....              <-- offset
        // .........              <-- desired distance
        //         x              <-- this is PhantomNode.location
        // 0----1----2----3----4  <-- EdgeBasedGraph Node segments
        BOOST_ASSERT(forward_segment_id.enabled);
        return forward_distance + forward_distance_offset;
    }

    EdgeDistance GetReverseDistance() const
    {
        //            ..........  <-- reverse_distance
        //         ...            <-- offset
        //         .............  <-- desired distance
        //         x              <-- this is PhantomNode.location
        // 0----1----2----3----4  <-- EdgeBasedGraph Node segments
        BOOST_ASSERT(reverse_segment_id.enabled);
        return reverse_distance + reverse_distance_offset;
    }

    bool IsBidirected() const { return forward_segment_id.enabled && reverse_segment_id.enabled; }

    bool IsValid(const unsigned number_of_nodes) const
    {
        return location.IsValid() &&
               ((forward_segment_id.id < number_of_nodes) ||
                (reverse_segment_id.id < number_of_nodes)) &&
               ((forward_weight != INVALID_EDGE_WEIGHT) ||
                (reverse_weight != INVALID_EDGE_WEIGHT)) &&
               ((forward_duration != MAXIMAL_EDGE_DURATION) ||
                (reverse_duration != MAXIMAL_EDGE_DURATION)) &&
               ((forward_distance != INVALID_EDGE_DISTANCE) ||
                (reverse_distance != INVALID_EDGE_DISTANCE)) &&
               (component.id != INVALID_COMPONENTID);
    }

    bool IsValid(const unsigned number_of_nodes, const util::Coordinate queried_coordinate) const
    {
        return queried_coordinate == input_location && IsValid(number_of_nodes);
    }

    bool IsValid() const { return location.IsValid(); }

    bool IsValidForwardSource() const
    {
        return forward_segment_id.enabled && is_valid_forward_source;
    }
    bool IsValidForwardTarget() const
    {
        return forward_segment_id.enabled && is_valid_forward_target;
    }
    bool IsValidReverseSource() const
    {
        return reverse_segment_id.enabled && is_valid_reverse_source;
    }
    bool IsValidReverseTarget() const
    {
        return reverse_segment_id.enabled && is_valid_reverse_target;
    }
    short GetBearing(const bool traversed_in_reverse) const
    {
        if (traversed_in_reverse)
            return std::round(util::bearing::reverse(bearing));
        return std::round(bearing);
    }

    bool operator==(const PhantomNode &other) const { return location == other.location; }

    template <class OtherT>
    explicit PhantomNode(const OtherT &other,
                         ComponentID component,
                         EdgeWeight forward_weight,
                         EdgeWeight reverse_weight,
                         EdgeWeight forward_weight_offset,
                         EdgeWeight reverse_weight_offset,
                         EdgeDistance forward_distance,
                         EdgeDistance reverse_distance,
                         EdgeDistance forward_distance_offset,
                         EdgeDistance reverse_distance_offset,
                         EdgeWeight forward_duration,
                         EdgeWeight reverse_duration,
                         EdgeWeight forward_duration_offset,
                         EdgeWeight reverse_duration_offset,
                         bool is_valid_forward_source,
                         bool is_valid_forward_target,
                         bool is_valid_reverse_source,
                         bool is_valid_reverse_target,
                         const util::Coordinate location,
                         const util::Coordinate input_location,
                         const unsigned short bearing)
        : forward_segment_id{other.forward_segment_id},
          reverse_segment_id{other.reverse_segment_id}, forward_weight{forward_weight},
          reverse_weight{reverse_weight}, forward_weight_offset{forward_weight_offset},
          reverse_weight_offset{reverse_weight_offset}, forward_distance{forward_distance},
          reverse_distance{reverse_distance}, forward_distance_offset{forward_distance_offset},
          reverse_distance_offset{reverse_distance_offset}, forward_duration{forward_duration},
          reverse_duration{reverse_duration}, forward_duration_offset{forward_duration_offset},
          reverse_duration_offset{reverse_duration_offset},
          component{component.id, component.is_tiny}, location{location},
          input_location{input_location}, fwd_segment_position{other.fwd_segment_position},
          is_valid_forward_source{is_valid_forward_source},
          is_valid_forward_target{is_valid_forward_target},
          is_valid_reverse_source{is_valid_reverse_source},
          is_valid_reverse_target{is_valid_reverse_target}, bearing{bearing}
    {
    }

    SegmentID forward_segment_id;
    SegmentID reverse_segment_id;
    EdgeWeight forward_weight;
    EdgeWeight reverse_weight;
    EdgeWeight forward_weight_offset; // TODO: try to remove -> requires path unpacking changes
    EdgeWeight reverse_weight_offset; // TODO: try to remove -> requires path unpacking changes
    EdgeDistance forward_distance;
    EdgeDistance reverse_distance;
    EdgeDistance forward_distance_offset; // TODO: try to remove -> requires path unpacking changes
    EdgeDistance reverse_distance_offset; // TODO: try to remove -> requires path unpacking changes
    EdgeWeight forward_duration;
    EdgeWeight reverse_duration;
    EdgeWeight forward_duration_offset; // TODO: try to remove -> requires path unpacking changes
    EdgeWeight reverse_duration_offset; // TODO: try to remove -> requires path unpacking changes
    ComponentID component;

    util::Coordinate location; // this is the coordinate of x
    util::Coordinate input_location;
    unsigned short fwd_segment_position;
    // is phantom node valid to be used as source or target
  private:
    unsigned short is_valid_forward_source : 1;
    unsigned short is_valid_forward_target : 1;
    unsigned short is_valid_reverse_source : 1;
    unsigned short is_valid_reverse_target : 1;
    unsigned short bearing : 12;
};

static_assert(sizeof(PhantomNode) == 80, "PhantomNode has more padding then expected");

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
} // namespace engine
} // namespace osrm

#endif // PHANTOM_NODES_H
