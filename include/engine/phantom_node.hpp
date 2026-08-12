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

#ifndef OSRM_ENGINE_PHANTOM_NODE_H
#define OSRM_ENGINE_PHANTOM_NODE_H

#include <vector>

#include "extractor/travel_mode.hpp"

#include "util/bearing.hpp"
#include "util/coordinate.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

namespace osrm::engine
{

struct PhantomNode
{
    PhantomNode()
        : forward_segment_id{SPECIAL_SEGMENTID, false},
          reverse_segment_id{SPECIAL_SEGMENTID, false}, forward_weight(INVALID_EDGE_WEIGHT),
          reverse_weight(INVALID_EDGE_WEIGHT), forward_weight_offset{0}, reverse_weight_offset{0},
          forward_distance(INVALID_EDGE_DISTANCE), reverse_distance(INVALID_EDGE_DISTANCE),
          forward_distance_offset{0}, reverse_distance_offset{0},
          forward_duration(MAXIMAL_EDGE_DURATION), reverse_duration(MAXIMAL_EDGE_DURATION),
          forward_duration_offset{0}, reverse_duration_offset{0}, approach_weight{0},
          approach_duration{0}, approach_distance{0}, component({INVALID_COMPONENTID, 0}),
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

    EdgeDuration GetForwardDuration() const
    {
        BOOST_ASSERT(forward_segment_id.enabled);
        return forward_duration + forward_duration_offset;
    }

    EdgeDuration GetReverseDuration() const
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

    //
    // What a search puts in its heap for this phantom.  A source is seeded with the
    // negation of its own cost, which the path then makes up as it travels the rest of
    // the segment; a target with the cost itself.  The approach walk is added in both
    // roles, being travelled either way.
    //
    // Deliberately not folded into the Get*PlusOffset accessors: leg assembly reads
    // those to work out what a route costs, and the walk is already drawn into the leg
    // itself (engine/area_route.hpp).  These are for seeding a heap and nothing else.
    //

    EdgeWeight GetForwardWeightAsSource() const
    { return approach_weight - GetForwardWeightPlusOffset(); }
    EdgeWeight GetForwardWeightAsTarget() const
    { return approach_weight + GetForwardWeightPlusOffset(); }
    EdgeWeight GetReverseWeightAsSource() const
    { return approach_weight - GetReverseWeightPlusOffset(); }
    EdgeWeight GetReverseWeightAsTarget() const
    { return approach_weight + GetReverseWeightPlusOffset(); }

    EdgeDuration GetForwardDurationAsSource() const
    { return approach_duration - GetForwardDuration(); }
    EdgeDuration GetForwardDurationAsTarget() const
    { return approach_duration + GetForwardDuration(); }
    EdgeDuration GetReverseDurationAsSource() const
    { return approach_duration - GetReverseDuration(); }
    EdgeDuration GetReverseDurationAsTarget() const
    { return approach_duration + GetReverseDuration(); }

    EdgeDistance GetForwardDistanceAsSource() const
    { return approach_distance - GetForwardDistance(); }
    EdgeDistance GetForwardDistanceAsTarget() const
    { return approach_distance + GetForwardDistance(); }
    EdgeDistance GetReverseDistanceAsSource() const
    { return approach_distance - GetReverseDistance(); }
    EdgeDistance GetReverseDistanceAsTarget() const
    { return approach_distance + GetReverseDistance(); }

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
    { return queried_coordinate == input_location && IsValid(number_of_nodes); }

    bool IsValid() const { return location.IsValid(); }

    bool IsValidForwardSource() const
    { return forward_segment_id.enabled && is_valid_forward_source; }
    bool IsValidForwardTarget() const
    { return forward_segment_id.enabled && is_valid_forward_target; }
    bool IsValidReverseSource() const
    { return reverse_segment_id.enabled && is_valid_reverse_source; }
    bool IsValidReverseTarget() const
    { return reverse_segment_id.enabled && is_valid_reverse_target; }
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
                         EdgeDuration forward_duration,
                         EdgeDuration reverse_duration,
                         EdgeDuration forward_duration_offset,
                         EdgeDuration reverse_duration_offset,
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
          reverse_duration_offset{reverse_duration_offset}, approach_weight{0},
          approach_duration{0}, approach_distance{0}, component{component.id, component.is_tiny},
          location{location}, input_location{input_location},
          fwd_segment_position{other.fwd_segment_position},
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
    EdgeDuration forward_duration;
    EdgeDuration reverse_duration;
    EdgeDuration forward_duration_offset; // TODO: try to remove -> requires path unpacking changes
    EdgeDuration reverse_duration_offset; // TODO: try to remove -> requires path unpacking changes

    /**
     * The walk from where the traveller asked to be to where this phantom sits, for a
     * coordinate that snapped into an open area (engine/area_snapping.hpp).  Zero for
     * every ordinary phantom.
     *
     * It cannot live in the offsets above.  A search seeds a source with the negation of
     * its offset and a target with the offset itself, so one stored number would have to
     * carry both signs, and the same coordinate is a source in one journey and a target
     * in another: every coordinate of a table is both at once, and so is a via point.
     * Kept separately, it is added by the seeding accessors below in either role.
     */
    EdgeWeight approach_weight;
    EdgeDuration approach_duration;
    EdgeDistance approach_distance;

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

static_assert(sizeof(PhantomNode) == 92, "PhantomNode has more padding than expected");

using PhantomNodeCandidates = std::vector<PhantomNode>;
using PhantomCandidateAlternatives = std::pair<PhantomNodeCandidates, PhantomNodeCandidates>;

struct PhantomNodeWithDistance
{
    PhantomNode phantom_node;
    double distance;
};

struct PhantomEndpointCandidates
{
    const PhantomNodeCandidates &source_phantoms;
    const PhantomNodeCandidates &target_phantoms;
};

struct PhantomCandidatesToTarget
{
    const PhantomNodeCandidates &source_phantoms;
    const PhantomNode &target_phantom;
};

inline util::Coordinate candidatesSnappedLocation(const PhantomNodeCandidates &candidates)
{
    BOOST_ASSERT(!candidates.empty());
    return candidates.front().location;
}

inline util::Coordinate candidatesInputLocation(const PhantomNodeCandidates &candidates)
{
    BOOST_ASSERT(!candidates.empty());
    return candidates.front().input_location;
}

inline bool candidatesHaveComponent(const PhantomNodeCandidates &candidates, uint32_t component_id)
{
    return std::any_of(candidates.begin(),
                       candidates.end(),
                       [component_id](const PhantomNode &node)
                       { return node.component.id == component_id; });
}

struct PhantomEndpoints
{
    PhantomNode source_phantom;
    PhantomNode target_phantom;
};

} // namespace osrm::engine

#endif // OSRM_ENGINE_PHANTOM_NODE_H
