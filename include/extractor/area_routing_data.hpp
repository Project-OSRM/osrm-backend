#ifndef OSRM_EXTRACTOR_AREA_ROUTING_DATA_HPP
#define OSRM_EXTRACTOR_AREA_ROUTING_DATA_HPP

#include "util/typedefs.hpp"

#include <cstdint>

namespace osrm::extractor
{

/**
 * @brief One open area, as stored in the area R-tree.
 *
 * util::StaticRTree derives each leaf's bounding box from the coordinates its @c u and
 * @c v members index, so an area is represented to the tree by the two opposite corners
 * of its bounding box.
 *
 * Those indices point into a coordinate list belonging to the area R-tree alone, holding
 * nothing but these corners -- *not* into the coordinate list shared with the rest of
 * the engine.  That list is indexed by NodeID and runs parallel to osm_node_ids;
 * appending corners to it would create NodeIDs corresponding to no OSM node, and every
 * consumer of that invariant would have to be audited.  Two coordinates per area is a
 * cheap price for not having that conversation.
 *
 * The polygon itself lives in flat arrays alongside the tree, which each area addresses
 * by offset and count.
 *
 * The entry points are deliberately not stored.  What the engine needs to know about a
 * vertex is whether it has any routable edge, and it can see that directly: a vertex the
 * mesher gave an edge to has a phantom node sitting exactly on it.  Storing entry points
 * would answer a narrower question and would mean resolving OSM node ids to NodeIDs at
 * extraction time, which is a scan over every node in the input.
 *
 * Every ring is carried, outer first and then the obstacles, because snapping adds a
 * virtual edge from the query coordinate to each visibility-graph vertex it can see --
 * and the obstacle corners are exactly the vertices that make going round the back of an
 * obstacle possible.  Storing only the outer ring would leave the engine unable to see
 * them, and unable to work out what blocks a line of sight in the first place.
 */
struct AreaPolygonSegment
{
    //! Opposite corners of the bounding box, indexing the area R-tree's own coordinates.
    //! Area @c i owns corners @c 2i and @c 2i+1, so the members are redundant with the
    //! area's own index -- but StaticRTree reorders its objects while packing, and after
    //! that the index is gone.  They have to travel with the object.
    NodeID u = SPECIAL_NODEID;
    NodeID v = SPECIAL_NODEID;

    //! Every ring flattened, outer first: a range into the shared vertex array.
    std::uint32_t vertices_offset = 0;
    std::uint32_t num_vertices = 0;

    //! Where each ring ends within that range: a range into the ring-length array.
    //! The first ring is the outer one, the rest are obstacles.
    std::uint32_t rings_offset = 0;
    std::uint32_t num_rings = 0;

    //! Speed in m/s for crossing this area, taken from the profile at extraction time.
    double walking_speed = 0.0;
};

// The R-tree packs leaves into pages, so the size of this decides how many areas share a
// page.  It is not a correctness constraint, but a change here changes the on-disk
// layout and wants to be deliberate.
static_assert(sizeof(AreaPolygonSegment) == 32, "AreaPolygonSegment has unexpected padding");

} // namespace osrm::extractor

#endif // OSRM_EXTRACTOR_AREA_ROUTING_DATA_HPP
