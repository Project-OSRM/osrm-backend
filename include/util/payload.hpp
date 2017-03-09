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

#ifndef PAYLOAD_H
#define PAYLOAD_H

#include "util/typedefs.hpp"

struct DurationDistancePayload 
{
    EdgeWeight duration;
    EdgeDistance distance;
    
    DurationDistancePayload(const EdgeWeight & duration, const EdgeDistance & distance) : duration(duration), distance(distance) {}
    DurationDistancePayload() : duration(0), distance(0) {}
    
    bool operator==(const DurationDistancePayload &right) const
    {
        return duration == right.duration && distance == right.distance;
    }
    
    bool operator<(const DurationDistancePayload &right) const
    {
        return duration < right.duration || (duration == right.duration && distance < right.distance);
    }
    
    DurationDistancePayload & operator+=(const DurationDistancePayload &right)
    {
        duration += right.duration;
        distance += right.distance;
        return *this;
    }
    
   DurationDistancePayload operator-() const
   {
        return DurationDistancePayload(-duration, -distance);
   }
};

inline DurationDistancePayload operator+(const DurationDistancePayload& lhs, const DurationDistancePayload& rhs)
{
  return DurationDistancePayload(lhs.duration + rhs.duration, lhs.distance + rhs.distance);
}

static_assert(sizeof(DurationDistancePayload) == 8, "DurationDistancePayload is larger than expected");

struct DurationPayload 
{
    EdgeWeight duration;
        
    DurationPayload(const DurationDistancePayload & other) : DurationPayload(other.duration) {}
    DurationPayload(const EdgeWeight & duration, const EdgeDistance & distance) : DurationPayload(duration) { (void)distance; }
    DurationPayload(const EdgeWeight & duration) : duration(duration) {}
    DurationPayload() : duration(0) {}
    
    bool operator==(const DurationPayload &right) const
    {
        return duration == right.duration;
    }
    
    bool operator<(const DurationPayload &right) const
    {
        return duration < right.duration;
    }
    
    DurationPayload & operator+=(const DurationPayload &right)
    {
        duration += right.duration;
        return *this;
    }
    
    DurationPayload operator-() const
    {
        return DurationPayload(-duration);
    }
};

inline DurationPayload operator+(const DurationPayload& lhs, const DurationPayload& rhs)
{
  return DurationPayload(lhs.duration + rhs.duration);
}

static_assert(sizeof(DurationPayload) == 4, "DurationPayload is larger than expected");


//////////////////////////////////////////////////////

#ifdef PAYLOAD_TYPE_DURATIONS_AND_DISTANCES
using EdgePayload = DurationDistancePayload;
using UncompressedEdgePayload = DurationPayload;
using RoutingPayload = DurationDistancePayload;
#elif PAYLOAD_TYPE_DURATIONS
using EdgePayload = DurationPayload;
using UncompressedEdgePayload = DurationPayload;
using RoutingPayload = DurationPayload;
#else
#error "Please specify desired payload type in Makefile."
#endif

static inline EdgePayload MAKE_PAYLOAD(EdgeWeight duration, EdgeDistance distance)
{
    return DurationDistancePayload(duration, distance);
}

static inline RoutingPayload MAKE_ROUTING_PAYLOAD(EdgeWeight duration, EdgeDistance distance)
{
    return RoutingPayload(duration, distance);
}


static const EdgePayload INVALID_PAYLOAD = MAKE_PAYLOAD(MAXIMAL_EDGE_DURATION, MAXIMAL_EDGE_DISTANCE);
static const RoutingPayload INVALID_RPAYLOAD = MAKE_ROUTING_PAYLOAD(MAXIMAL_EDGE_DURATION, MAXIMAL_EDGE_DISTANCE);

#endif /* PAYLOAD_H */