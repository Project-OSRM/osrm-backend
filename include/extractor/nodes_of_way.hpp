#ifndef NODES_OF_WAY_HPP
#define NODES_OF_WAY_HPP

#include "util/typedefs.hpp"

#include <limits>
#include <string>
#include <vector>

namespace osrm::extractor
{

// NodesOfWay contains the ordered nodes of a way and provides convenience functions for getting
// nodes in the first and last segments.
struct NodesOfWay
{
    OSMWayID way_id;
    std::vector<OSMNodeID> node_ids;

    NodesOfWay() : way_id(SPECIAL_OSM_WAYID), node_ids{SPECIAL_OSM_NODEID, SPECIAL_OSM_NODEID} {}

    NodesOfWay(OSMWayID w, std::vector<OSMNodeID> ns) : way_id(w), node_ids(std::move(ns)) {}

    static NodesOfWay min_value() { return {MIN_OSM_WAYID, {MIN_OSM_NODEID, MIN_OSM_NODEID}}; }
    static NodesOfWay max_value() { return {MAX_OSM_WAYID, {MAX_OSM_NODEID, MAX_OSM_NODEID}}; }

    const OSMNodeID &first_segment_source_id() const
    {
        BOOST_ASSERT(!node_ids.empty());
        return node_ids[0];
    }

    const OSMNodeID &first_segment_target_id() const
    {
        BOOST_ASSERT(node_ids.size() > 1);
        return node_ids[1];
    }

    const OSMNodeID &last_segment_source_id() const
    {
        BOOST_ASSERT(node_ids.size() > 1);
        return node_ids[node_ids.size() - 2];
    }

    const OSMNodeID &last_segment_target_id() const
    {
        BOOST_ASSERT(!node_ids.empty());
        return node_ids[node_ids.size() - 1];
    }
};
} // namespace osrm::extractor

#endif /* NODES_OF_WAY_HPP */
