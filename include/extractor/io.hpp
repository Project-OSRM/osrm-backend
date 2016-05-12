#ifndef OSRM_EXTRACTOR_IO_HPP
#define OSRM_EXTRACTOR_IO_HPP

#include "util/fingerprint.hpp"
#include "util/typedefs.hpp"

namespace osrm
{
namespace extractor
{
namespace io
{
/**
 * All these structs are set to 1 byte alignment to get compact on-disk storage
 */
// Set to 1 byte alignment
#pragma pack(push, 1)
struct SegmentHeaderBlock
{
    std::uint32_t num_osm_nodes;
    OSMNodeID previous_osm_node_id;
};
#pragma pack(pop)
static_assert(std::is_trivial<SegmentHeaderBlock>::value, "SegmentHeaderBlock is not trivial");
static_assert(sizeof(SegmentHeaderBlock) == 12, "SegmentHeaderBlock is not packed correctly");

#pragma pack(push, 1)
struct SegmentBlock
{
    OSMNodeID this_osm_node_id;
    double segment_length;
    EdgeWeight segment_weight;
};
#pragma pack(pop)
static_assert(std::is_trivial<SegmentBlock>::value, "SegmentBlock is not trivial");
static_assert(sizeof(SegmentBlock) == 20, "SegmentBlock is not packed correctly");

#pragma pack(push, 1)
struct TurnPenaltiesHeader
{
    //! the number of penalties in each block
    std::uint64_t number_of_penalties;
};
#pragma pack(pop)
static_assert(std::is_trivial<TurnPenaltiesHeader>::value, "TurnPenaltiesHeader is not trivial");
static_assert(sizeof(TurnPenaltiesHeader) == 8, "TurnPenaltiesHeader is not packed correctly");

#pragma pack(push, 1)
struct TurnIndexBlock
{
    OSMNodeID from_id;
    OSMNodeID via_id;
    OSMNodeID to_id;
};
#pragma pack(pop)
static_assert(std::is_trivial<TurnIndexBlock>::value, "TurnIndexBlock is not trivial");
static_assert(sizeof(TurnIndexBlock) == 24, "TurnIndexBlock is not packed correctly");

#pragma pack(push, 1)
struct EdgeBasedGraphHeader
{
    util::FingerPrint fingerprint;
    std::uint64_t number_of_edges;
    EdgeID max_edge_id;
};
#pragma pack(pop)
static_assert(std::is_trivial<EdgeBasedGraphHeader>::value, "EdgeBasedGraphHeader is not trivial");
static_assert(sizeof(EdgeBasedGraphHeader) ==
                  sizeof(util::FingerPrint) + sizeof(std::uint64_t) + sizeof(EdgeID),
              "Packing of EdgeBasedGraphHeader failed");
}
}
}
#endif
