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

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include "util/alias.hpp"

#include <boost/assert.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>

// OpenStreetMap node ids are higher than 2^32
namespace tag
{
struct osm_node_id
{
};
struct osm_way_id
{
};
struct duplicated_node
{
};
struct edge_weight
{
};
struct edge_duration
{
};
struct edge_distance
{
};
struct segment_weight
{
};
struct segment_duration
{
};
struct turn_penalty
{
};
} // namespace tag

using OSMNodeID = osrm::Alias<std::uint64_t, tag::osm_node_id>;
// clang-tidy fires `bugprone-throw-keyword-missing` here for unknown reason
// NOLINTNEXTLINE(bugprone-throw-keyword-missing)
static_assert(std::is_standard_layout<OSMNodeID>() && std::is_trivial<OSMNodeID>(),
              "OSMNodeID is not a valid alias");
using OSMWayID = osrm::Alias<std::uint64_t, tag::osm_way_id>;
// NOLINTNEXTLINE(bugprone-throw-keyword-missing)
static_assert(std::is_standard_layout<OSMWayID>() && std::is_trivial<OSMWayID>(),
              "OSMWayID is not a valid alias");

using DuplicatedNodeID = std::uint64_t;
using RestrictionID = std::uint64_t;

static const OSMNodeID SPECIAL_OSM_NODEID =
    OSMNodeID{std::numeric_limits<OSMNodeID::value_type>::max()};
static const OSMWayID SPECIAL_OSM_WAYID =
    OSMWayID{std::numeric_limits<OSMWayID::value_type>::max()};

static const OSMNodeID MAX_OSM_NODEID =
    OSMNodeID{std::numeric_limits<OSMNodeID::value_type>::max()};
static const OSMNodeID MIN_OSM_NODEID =
    OSMNodeID{std::numeric_limits<OSMNodeID::value_type>::min()};
static const OSMWayID MAX_OSM_WAYID = OSMWayID{std::numeric_limits<OSMWayID::value_type>::max()};
static const OSMWayID MIN_OSM_WAYID = OSMWayID{std::numeric_limits<OSMWayID::value_type>::min()};

using NodeID = std::uint32_t;
using EdgeID = std::uint32_t;
using NameID = std::uint32_t;
using AnnotationID = std::uint32_t;
using PackedGeometryID = std::uint32_t;

using EdgeWeight = osrm::Alias<std::int32_t, tag::edge_weight>;
using EdgeDuration = osrm::Alias<std::int32_t, tag::edge_duration>;
using EdgeDistance = osrm::Alias<float, tag::edge_distance>;
using SegmentWeight = osrm::Alias<std::uint32_t, tag::segment_weight>;
using SegmentDuration = osrm::Alias<std::uint32_t, tag::segment_duration>;
using TurnPenalty = osrm::Alias<std::int16_t, tag::turn_penalty>; // turn penalty in 100ms units

static const std::size_t INVALID_INDEX = std::numeric_limits<std::size_t>::max();

using LaneID = std::uint8_t;
static const LaneID INVALID_LANEID = std::numeric_limits<LaneID>::max();
using LaneDataID = std::uint16_t;
static const LaneDataID INVALID_LANE_DATAID = std::numeric_limits<LaneDataID>::max();
using LaneDescriptionID = std::uint16_t;
static const LaneDescriptionID INVALID_LANE_DESCRIPTIONID =
    std::numeric_limits<LaneDescriptionID>::max();

using BearingClassID = std::uint32_t;
using DiscreteBearing = std::uint16_t;
using EntryClassID = std::uint16_t;

static const NodeID SPECIAL_NODEID = std::numeric_limits<NodeID>::max();
static const NodeID SPECIAL_SEGMENTID = std::numeric_limits<NodeID>::max() >> 1;
static const PackedGeometryID SPECIAL_GEOMETRYID =
    std::numeric_limits<PackedGeometryID>::max() >> 1;
static const EdgeID SPECIAL_EDGEID = std::numeric_limits<EdgeID>::max();
static const RestrictionID SPECIAL_RESTRICTIONID = std::numeric_limits<RestrictionID>::max();
static const NameID INVALID_NAMEID = std::numeric_limits<NameID>::max();
static const NameID EMPTY_NAMEID = 0;
static const unsigned INVALID_COMPONENTID = 0;
static const std::size_t SEGMENT_WEIGHT_BITS = 22;
static const std::size_t SEGMENT_DURATION_BITS = 22;
static const SegmentWeight INVALID_SEGMENT_WEIGHT = SegmentWeight{(1u << SEGMENT_WEIGHT_BITS) - 1};
static const SegmentDuration INVALID_SEGMENT_DURATION =
    SegmentDuration{(1u << SEGMENT_DURATION_BITS) - 1};
static const SegmentWeight MAX_SEGMENT_WEIGHT = INVALID_SEGMENT_WEIGHT - SegmentWeight{1};
static const SegmentDuration MAX_SEGMENT_DURATION = INVALID_SEGMENT_DURATION - SegmentDuration{1};
static const EdgeWeight INVALID_EDGE_WEIGHT =
    EdgeWeight{std::numeric_limits<EdgeWeight::value_type>::max()};
static const EdgeDuration INVALID_EDGE_DURATION =
    EdgeDuration{std::numeric_limits<EdgeDuration::value_type>::max()};
static const EdgeDistance INVALID_EDGE_DISTANCE =
    EdgeDistance{std::numeric_limits<EdgeDistance::value_type>::max()};
static const TurnPenalty INVALID_TURN_PENALTY =
    TurnPenalty{std::numeric_limits<TurnPenalty::value_type>::max()};
static const EdgeDistance INVALID_FALLBACK_SPEED =
    EdgeDistance{std::numeric_limits<EdgeDistance::value_type>::max()};
// TODO: These are the same as the invalid values. Do we need both?
static const EdgeWeight MAXIMAL_EDGE_WEIGHT =
    EdgeWeight{std::numeric_limits<EdgeWeight::value_type>::max()};
static const EdgeDuration MAXIMAL_EDGE_DURATION =
    EdgeDuration{std::numeric_limits<EdgeDuration::value_type>::max()};
static const EdgeDistance MAXIMAL_EDGE_DISTANCE =
    EdgeDistance{std::numeric_limits<EdgeDistance::value_type>::max()};
static const TurnPenalty MAXIMAL_TURN_PENALTY =
    TurnPenalty{std::numeric_limits<TurnPenalty::value_type>::max()};

using DatasourceID = std::uint8_t;

using BisectionID = std::uint32_t;
using LevelID = std::uint8_t;
using CellID = std::uint32_t;
using PartitionID = std::uint64_t;

static constexpr auto INVALID_LEVEL_ID = std::numeric_limits<LevelID>::max();
static constexpr auto INVALID_CELL_ID = std::numeric_limits<CellID>::max();

struct SegmentID
{
    SegmentID(const NodeID id_, const bool enabled_) : id{id_}, enabled{enabled_}
    {
        BOOST_ASSERT(!enabled || id != SPECIAL_SEGMENTID);
    }

    NodeID id : 31;
    std::uint32_t enabled : 1;
};

/* We need to bit pack here because the index for the via_node
 * is given to us without knowing whether the geometry should
 * be read forward or in reverse. The extra field `forward`
 * indicates that to the routing engine
 */
struct GeometryID
{
    GeometryID(const PackedGeometryID id_, const bool forward_) : id{id_}, forward{forward_} {}

    GeometryID() : id(std::numeric_limits<unsigned>::max() >> 1), forward(false) {}

    PackedGeometryID id : 31;
    std::uint32_t forward : 1;
};

static_assert(sizeof(SegmentID) == 4, "SegmentID needs to be 4 bytes big");

// Strongly connected component ID of an edge-based node
struct ComponentID
{
    std::uint32_t id : 31;
    std::uint32_t is_tiny : 1;
};

#endif /* TYPEDEFS_H */
