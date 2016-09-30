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

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include "util/strong_typedef.hpp"

#include <boost/assert.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>

// OpenStreetMap node ids are higher than 2^32
OSRM_STRONG_TYPEDEF(std::uint64_t, OSMNodeID)
OSRM_STRONG_TYPEDEF_HASHABLE(std::uint64_t, OSMNodeID)

OSRM_STRONG_TYPEDEF(std::uint32_t, OSMWayID)
OSRM_STRONG_TYPEDEF_HASHABLE(std::uint32_t, OSMWayID)

static const OSMNodeID SPECIAL_OSM_NODEID = OSMNodeID{std::numeric_limits<std::uint64_t>::max()};
static const OSMWayID SPECIAL_OSM_WAYID = OSMWayID{std::numeric_limits<std::uint32_t>::max()};

static const OSMNodeID MAX_OSM_NODEID = OSMNodeID{std::numeric_limits<std::uint64_t>::max()};
static const OSMNodeID MIN_OSM_NODEID = OSMNodeID{std::numeric_limits<std::uint64_t>::min()};
static const OSMWayID MAX_OSM_WAYID = OSMWayID{std::numeric_limits<std::uint32_t>::max()};
static const OSMWayID MIN_OSM_WAYID = OSMWayID{std::numeric_limits<std::uint32_t>::min()};

using OSMNodeID_weak = std::uint64_t;
using OSMEdgeID_weak = std::uint64_t;

using NodeID = std::uint32_t;
using EdgeID = std::uint32_t;
using NameID = std::uint32_t;
using EdgeWeight = std::int32_t;

using LaneID = std::uint8_t;
static const LaneID INVALID_LANEID = std::numeric_limits<LaneID>::max();
using LaneDataID = std::uint16_t;
static const LaneDataID INVALID_LANE_DATAID = std::numeric_limits<LaneDataID>::max();
using LaneDescriptionID = std::uint16_t;
static const LaneDescriptionID INVALID_LANE_DESCRIPTIONID =
    std::numeric_limits<LaneDescriptionID>::max();

using BearingClassID = std::uint32_t;
static const BearingClassID INVALID_BEARING_CLASSID = std::numeric_limits<BearingClassID>::max();

using DiscreteBearing = std::uint16_t;

using EntryClassID = std::uint16_t;
static const EntryClassID INVALID_ENTRY_CLASSID = std::numeric_limits<EntryClassID>::max();

static const NodeID SPECIAL_NODEID = std::numeric_limits<NodeID>::max();
static const NodeID SPECIAL_SEGMENTID = std::numeric_limits<NodeID>::max() >> 1;
static const EdgeID SPECIAL_EDGEID = std::numeric_limits<EdgeID>::max();
static const NameID INVALID_NAMEID = std::numeric_limits<NameID>::max();
static const NameID EMPTY_NAMEID = 0;
static const unsigned INVALID_COMPONENTID = 0;
static const EdgeWeight INVALID_EDGE_WEIGHT = std::numeric_limits<EdgeWeight>::max();

using DatasourceID = std::uint8_t;

struct SegmentID
{
    SegmentID(const NodeID id_, const bool enabled_) : id{id_}, enabled{enabled_}
    {
        BOOST_ASSERT(!enabled || id != SPECIAL_SEGMENTID);
    }

    NodeID id : 31;
    std::uint32_t enabled : 1;
};

static_assert(sizeof(SegmentID) == 4, "SegmentID needs to be 4 bytes big");

#endif /* TYPEDEFS_H */
