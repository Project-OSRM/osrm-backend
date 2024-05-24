#include "extractor/extraction_containers.hpp"
#include "extractor/extraction_segment.hpp"
#include "extractor/extraction_way.hpp"
#include "extractor/files.hpp"
#include "extractor/name_table.hpp"
#include "extractor/restriction.hpp"
#include "extractor/serialization.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/integer_range.hpp"

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/for_each_indexed.hpp"
#include "util/for_each_pair.hpp"
#include "util/log.hpp"
#include "util/std_hash.hpp"
#include "util/timing_util.hpp"

#include <boost/assert.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <tbb/parallel_sort.h>

#include <chrono>
#include <limits>
#include <mutex>
#include <sstream>

namespace
{
namespace oe = osrm::extractor;

struct CmpEdgeByOSMStartID
{
    using value_type = oe::InternalExtractorEdge;
    bool operator()(const value_type &lhs, const value_type &rhs) const
    {
        return lhs.result.osm_source_id < rhs.result.osm_source_id;
    }
};

struct CmpEdgeByOSMTargetID
{
    using value_type = oe::InternalExtractorEdge;
    bool operator()(const value_type &lhs, const value_type &rhs) const
    {
        return lhs.result.osm_target_id < rhs.result.osm_target_id;
    }
};

struct CmpEdgeByInternalSourceTargetAndName
{
    using value_type = oe::InternalExtractorEdge;
    bool operator()(const value_type &lhs, const value_type &rhs) const
    {
        if (lhs.result.source != rhs.result.source)
            return lhs.result.source < rhs.result.source;

        if (lhs.result.source == SPECIAL_NODEID)
            return false;

        if (lhs.result.target != rhs.result.target)
            return lhs.result.target < rhs.result.target;

        if (lhs.result.target == SPECIAL_NODEID)
            return false;

        auto const lhs_name_id = edge_annotation_data[lhs.result.annotation_data].name_id;
        auto const rhs_name_id = edge_annotation_data[rhs.result.annotation_data].name_id;
        if (lhs_name_id == rhs_name_id)
            return false;

        if (lhs_name_id == EMPTY_NAMEID)
            return false;

        if (rhs_name_id == EMPTY_NAMEID)
            return true;

        BOOST_ASSERT(!name_offsets.empty() && name_offsets.back() == name_data.size());
        const oe::ExtractionContainers::NameCharData::const_iterator data = name_data.begin();
        return std::lexicographical_compare(data + name_offsets[lhs_name_id],
                                            data + name_offsets[lhs_name_id + 1],
                                            data + name_offsets[rhs_name_id],
                                            data + name_offsets[rhs_name_id + 1]);
    }

    const oe::ExtractionContainers::AnnotationDataVector &edge_annotation_data;
    const oe::ExtractionContainers::NameCharData &name_data;
    const oe::ExtractionContainers::NameOffsets &name_offsets;
};

template <typename Iter>
inline NodeID mapExternalToInternalNodeID(Iter first, Iter last, const OSMNodeID value)
{
    const auto it = std::lower_bound(first, last, value);
    return (it == last || value < *it) ? SPECIAL_NODEID
                                       : static_cast<NodeID>(std::distance(first, it));
}

/**
 *   Here's what these properties represent on the node-based-graph
 *       way "ABCD"                         way "AB"
 *  -----------------------------------------------------------------
 *     ⬇   A  first_segment_source_id
 *     ⬇   |
 *     ⬇︎   B  first_segment_target_id      A  first_segment_source_id
 *     ⬇︎   |                            ⬇ |  last_segment_source_id
 *     ⬇︎   |                            ⬇ |
 *     ⬇︎   |                               B  first_segment_target_id
 *     ⬇︎   C  last_segment_source_id          last_segment_target_id
 *     ⬇︎   |
 *     ⬇︎   D  last_segment_target_id
 *
 * Finds the point where two ways connect at the end, and returns the 3
 * node-based nodes that describe the turn (the node just before, the
 * node at the turn, and the next node after the turn)
 **/
std::tuple<OSMNodeID, OSMNodeID, OSMNodeID> find_turn_nodes(const oe::NodesOfWay &from,
                                                            const oe::NodesOfWay &via,
                                                            const OSMNodeID &intersection_node)
{
    // connection node needed to choose orientation if from and via are the same way. E.g. u-turns
    if (intersection_node == SPECIAL_OSM_NODEID ||
        intersection_node == from.first_segment_source_id())
    {
        if (from.first_segment_source_id() == via.first_segment_source_id())
        {
            return std::make_tuple(from.first_segment_target_id(),
                                   via.first_segment_source_id(),
                                   via.first_segment_target_id());
        }
        if (from.first_segment_source_id() == via.last_segment_target_id())
        {
            return std::make_tuple(from.first_segment_target_id(),
                                   via.last_segment_target_id(),
                                   via.last_segment_source_id());
        }
    }
    if (intersection_node == SPECIAL_OSM_NODEID ||
        intersection_node == from.last_segment_target_id())
    {
        if (from.last_segment_target_id() == via.first_segment_source_id())
        {
            return std::make_tuple(from.last_segment_source_id(),
                                   via.first_segment_source_id(),
                                   via.first_segment_target_id());
        }
        if (from.last_segment_target_id() == via.last_segment_target_id())
        {
            return std::make_tuple(from.last_segment_source_id(),
                                   via.last_segment_target_id(),
                                   via.last_segment_source_id());
        }
    }
    return std::make_tuple(SPECIAL_OSM_NODEID, SPECIAL_OSM_NODEID, SPECIAL_OSM_NODEID);
}

// Via-node paths describe a relation between the two segments closest
// to the shared via-node on the from and to ways.
// from: [a, b, c, d, e]
// to: [f, g, h, i, j]
//
// The via node establishes the orientation of the from/to intersection when choosing the
// segments.
//   via    |  node path
//   a=f    |     b,a,g
//   a=j    |     b,a,i
//   e=f    |     d,e,g
//   e=j    |     d,e,i
oe::ViaNodePath find_via_node_path(const std::string &turn_relation_type,
                                   const oe::NodesOfWay &from_segment,
                                   const oe::NodesOfWay &to_segment,
                                   const OSMNodeID via_node,
                                   const std::function<NodeID(OSMNodeID)> &to_internal_node)
{

    OSMNodeID from, via, to;
    std::tie(from, via, to) = find_turn_nodes(from_segment, to_segment, via_node);
    if (via == SPECIAL_OSM_NODEID)
    {
        // unconnected
        osrm::util::Log(logDEBUG) << turn_relation_type
                                  << " references unconnected way: " << from_segment.way_id;
        return oe::ViaNodePath{SPECIAL_NODEID, SPECIAL_NODEID, SPECIAL_NODEID};
    }
    return oe::ViaNodePath{to_internal_node(from), to_internal_node(via), to_internal_node(to)};
}

// Via way paths are comprised of:
// 1. The segment in the from way that intersects with the via ways
// 2. All segments that make up the via path
// 3. The segment in the to way that intersects with the via path.
//
// from: [a, b, c, d, e]
// via: [[f, g, h, i, j], [k, l], [m, n, o]]
// to: [p, q, r, s]
//
// First establish the orientation of the from/via intersection by finding which end
// nodes both ways share. From this we can select the from segment.
//
// intersect | from segment | next_connection
//    a=f    |      b,a     |        f
//    a=j    |      b,a     |        j
//    e=f    |      e,d     |        f
//    e=j    |      e,d     |        j
//
// Use the next connection to inform the orientation of the first via
// way and the intersection between first and second via ways.
//
// next_connection | intersect | via result  | next_next_connection
//       f         |    j=k    | [f,g,h,i,j] |      k
//       f         |    j=l    | [f,g,h,i,j] |      l
//       j         |    f=k    | [j,i,h,g,f] |      k
//       j         |    f=l    | [j,i,h,g,f] |      l
//
// This is continued for the remaining via ways, appending to the via result
//
// The final via/to intersection also uses the next_connection information in a similar fashion.
//
// next_connection | intersect | to_segment
//       m         |    o=p    |    p,q
//       m         |    o=s    |    s,r
//       o         |    m=p    |    p,q
//       o         |    m=s    |    s,r
//
// The final result is a list of nodes that represent a valid from->via->to path through the
// ways.
//
// E.g. if intersection nodes are a=j, f=l, k=o, m=s
// the result will be {e [d,c,b,a,i,h,g,f,k,n,m] r}
oe::ViaWayPath find_via_way_path(const std::string &turn_relation_type,
                                 const oe::NodesOfWay &from_way,
                                 const std::vector<oe::NodesOfWay> &via_ways,
                                 const oe::NodesOfWay &to_way,
                                 const std::function<NodeID(OSMNodeID)> &to_internal_node)
{
    BOOST_ASSERT(!via_ways.empty());

    oe::ViaWayPath way_path;

    // Find the orientation of the connected ways starting with the from-via intersection.
    OSMNodeID from, via;
    std::tie(from, via, std::ignore) =
        find_turn_nodes(from_way, via_ways.front(), SPECIAL_OSM_NODEID);
    if (via == SPECIAL_OSM_NODEID)
    {
        osrm::util::Log(logDEBUG) << turn_relation_type
                                  << " has unconnected from and via ways: " << from_way.way_id
                                  << ", " << via_ways.front().way_id;
        return oe::ViaWayPath{SPECIAL_NODEID, {}, SPECIAL_NODEID};
    }
    way_path.from = to_internal_node(from);
    way_path.via.push_back(to_internal_node(via));

    // Use the connection node from the previous intersection to inform our conversion of
    // via ways into internal nodes.
    OSMNodeID next_connection = via;
    for (const auto &via_way : via_ways)
    {
        if (next_connection == via_way.first_segment_source_id())
        {
            std::transform(std::next(via_way.node_ids.begin()),
                           via_way.node_ids.end(),
                           std::back_inserter(way_path.via),
                           to_internal_node);
            next_connection = via_way.last_segment_target_id();
        }
        else if (next_connection == via_way.last_segment_target_id())
        {
            std::transform(std::next(via_way.node_ids.rbegin()),
                           via_way.node_ids.rend(),
                           std::back_inserter(way_path.via),
                           to_internal_node);
            next_connection = via_way.first_segment_source_id();
        }
        else
        {
            osrm::util::Log(logDEBUG)
                << turn_relation_type << " has unconnected via way: " << via_way.way_id
                << " to node " << next_connection;
            return oe::ViaWayPath{SPECIAL_NODEID, {}, SPECIAL_NODEID};
        }
    }

    // Add the final to node after the via-to intersection.
    if (next_connection == to_way.first_segment_source_id())
    {
        way_path.to = to_internal_node(to_way.first_segment_target_id());
    }
    else if (next_connection == to_way.last_segment_target_id())
    {
        way_path.to = to_internal_node(to_way.last_segment_source_id());
    }
    else
    {
        osrm::util::Log(logDEBUG) << turn_relation_type
                                  << " has unconnected via and to ways: " << via_ways.back().way_id
                                  << ", " << to_way.way_id;
        return oe::ViaWayPath{SPECIAL_NODEID, {}, SPECIAL_NODEID};
    }
    return way_path;
}

// Check if we were able to resolve all the involved OSM elements before translating to an
// internal via-way turn path
oe::ViaWayPath
get_via_way_path_from_OSM_ids(const std::string &turn_relation_type,
                              const std::unordered_map<OSMWayID, oe::NodesOfWay> &referenced_ways,
                              const OSMWayID from_id,
                              const OSMWayID to_id,
                              const std::vector<OSMWayID> &via_ids,
                              const std::function<NodeID(OSMNodeID)> &to_internal_node)
{
    auto const from_way_itr = referenced_ways.find(from_id);
    if (from_way_itr->second.way_id != from_id)
    {
        osrm::util::Log(logDEBUG) << turn_relation_type
                                  << " references invalid from way: " << from_id;
        return oe::ViaWayPath{SPECIAL_NODEID, {}, SPECIAL_NODEID};
    }

    std::vector<oe::NodesOfWay> via_ways;
    for (const auto &via_id : via_ids)
    {
        auto const via_segment_itr = referenced_ways.find(via_id);
        if (via_segment_itr->second.way_id != via_id)
        {
            osrm::util::Log(logDEBUG)
                << turn_relation_type << " references invalid via way: " << via_id;
            return oe::ViaWayPath{SPECIAL_NODEID, {}, SPECIAL_NODEID};
        }
        via_ways.push_back(via_segment_itr->second);
    }

    auto const to_way_itr = referenced_ways.find(to_id);
    if (to_way_itr->second.way_id != to_id)
    {
        osrm::util::Log(logDEBUG) << turn_relation_type << " references invalid to way: " << to_id;
        return oe::ViaWayPath{SPECIAL_NODEID, {}, SPECIAL_NODEID};
    }

    return find_via_way_path(
        turn_relation_type, from_way_itr->second, via_ways, to_way_itr->second, to_internal_node);
}

// Check if we were able to resolve all the involved OSM elements before translating to an
// internal via-node turn path
oe::ViaNodePath
get_via_node_path_from_OSM_ids(const std::string &turn_relation_type,
                               const std::unordered_map<OSMWayID, oe::NodesOfWay> &referenced_ways,
                               const OSMWayID from_id,
                               const OSMWayID to_id,
                               const OSMNodeID via_node,
                               const std::function<NodeID(OSMNodeID)> &to_internal_node)
{

    auto const from_segment_itr = referenced_ways.find(from_id);

    if (from_segment_itr->second.way_id != from_id)
    {
        osrm::util::Log(logDEBUG) << turn_relation_type << " references invalid way: " << from_id;
        return oe::ViaNodePath{SPECIAL_NODEID, SPECIAL_NODEID, SPECIAL_NODEID};
    }

    auto const to_segment_itr = referenced_ways.find(to_id);
    if (to_segment_itr->second.way_id != to_id)
    {
        osrm::util::Log(logDEBUG) << turn_relation_type << " references invalid way: " << to_id;
        return oe::ViaNodePath{SPECIAL_NODEID, SPECIAL_NODEID, SPECIAL_NODEID};
    }

    return find_via_node_path(turn_relation_type,
                              from_segment_itr->second,
                              to_segment_itr->second,
                              via_node,
                              to_internal_node);
}

} // namespace

namespace osrm::extractor
{

ExtractionContainers::ExtractionContainers()
{
    // Insert four empty strings offsets for name, ref, destination, pronunciation, and exits
    name_offsets.push_back(0);
    name_offsets.push_back(0);
    name_offsets.push_back(0);
    name_offsets.push_back(0);
    name_offsets.push_back(0);
    // Insert the total length sentinel (corresponds to the next name string offset)
    name_offsets.push_back(0);
    // Sentinel for offset into used_nodes
    way_node_id_offsets.push_back(0);
}

/**
 * Processes the collected data and serializes it.
 * At this point nodes are still referenced by their OSM id.
 *
 * - Identify nodes of ways used in restrictions and maneuver overrides
 * - Filter nodes list to nodes that are referenced by ways
 * - Prepare edges and compute routing properties
 * - Prepare and validate restrictions and maneuver overrides
 *
 */
void ExtractionContainers::PrepareData(ScriptingEnvironment &scripting_environment,
                                       const std::string &name_file_name)
{
    const auto restriction_ways = IdentifyRestrictionWays();
    const auto maneuver_override_ways = IdentifyManeuverOverrideWays();
    const auto traffic_signals = IdentifyTrafficSignals();

    PrepareNodes();
    PrepareEdges(scripting_environment);

    PrepareTrafficSignals(traffic_signals);
    PrepareManeuverOverrides(maneuver_override_ways);
    PrepareRestrictions(restriction_ways);
    WriteCharData(name_file_name);
}

void ExtractionContainers::WriteCharData(const std::string &file_name)
{
    util::UnbufferedLog log;
    log << "writing street name index ... ";
    TIMER_START(write_index);

    files::writeNames(file_name,
                      NameTable{NameTable::IndexedData(
                          name_offsets.begin(), name_offsets.end(), name_char_data.begin())});

    TIMER_STOP(write_index);
    log << "ok, after " << TIMER_SEC(write_index) << "s";
}

void ExtractionContainers::PrepareNodes()
{
    {
        util::UnbufferedLog log;
        log << "Sorting used nodes        ... " << std::flush;
        TIMER_START(sorting_used_nodes);
        tbb::parallel_sort(used_node_id_list.begin(), used_node_id_list.end());
        TIMER_STOP(sorting_used_nodes);
        log << "ok, after " << TIMER_SEC(sorting_used_nodes) << "s";
    }

    {
        util::UnbufferedLog log;
        log << "Erasing duplicate nodes   ... " << std::flush;
        TIMER_START(erasing_dups);
        auto new_end = std::unique(used_node_id_list.begin(), used_node_id_list.end());
        used_node_id_list.resize(new_end - used_node_id_list.begin());
        TIMER_STOP(erasing_dups);
        log << "ok, after " << TIMER_SEC(erasing_dups) << "s";
    }

    {
        util::UnbufferedLog log;
        log << "Sorting all nodes         ... " << std::flush;
        TIMER_START(sorting_nodes);
        tbb::parallel_sort(all_nodes_list.begin(),
                           all_nodes_list.end(),
                           [](const auto &left, const auto &right)
                           { return left.node_id < right.node_id; });
        TIMER_STOP(sorting_nodes);
        log << "ok, after " << TIMER_SEC(sorting_nodes) << "s";
    }

    {
        util::UnbufferedLog log;
        log << "Building node id map      ... " << std::flush;
        TIMER_START(id_map);
        auto node_iter = all_nodes_list.begin();
        auto ref_iter = used_node_id_list.begin();
        auto used_nodes_iter = used_node_id_list.begin();
        const auto all_nodes_list_end = all_nodes_list.end();
        const auto used_node_id_list_end = used_node_id_list.end();

        // compute the intersection of nodes that were referenced and nodes we actually have
        while (node_iter != all_nodes_list_end && ref_iter != used_node_id_list_end)
        {
            if (node_iter->node_id < *ref_iter)
            {
                node_iter++;
                continue;
            }
            if (node_iter->node_id > *ref_iter)
            {
                ref_iter++;
                continue;
            }
            BOOST_ASSERT(node_iter->node_id == *ref_iter);
            *used_nodes_iter = *ref_iter;
            used_nodes_iter++;
            node_iter++;
            ref_iter++;
        }

        // Remove unused nodes and check maximal internal node id
        used_node_id_list.resize(std::distance(used_node_id_list.begin(), used_nodes_iter));
        if (used_node_id_list.size() > std::numeric_limits<NodeID>::max())
        {
            throw util::exception("There are too many nodes remaining after filtering, OSRM only "
                                  "supports 2^32 unique nodes, but there were " +
                                  std::to_string(used_node_id_list.size()) + SOURCE_REF);
        }
        max_internal_node_id = boost::numeric_cast<std::uint64_t>(used_node_id_list.size());
        TIMER_STOP(id_map);
        log << "ok, after " << TIMER_SEC(id_map) << "s";
    }
    {
        util::UnbufferedLog log;
        log << "Confirming/Writing used nodes     ... ";
        TIMER_START(write_nodes);
        // identify all used nodes by a merging step of two sorted lists
        auto node_iterator = all_nodes_list.begin();
        auto node_id_iterator = used_node_id_list.begin();
        const auto all_nodes_list_end = all_nodes_list.end();

        for (const auto index : util::irange<NodeID>(0, used_node_id_list.size()))
        {
            boost::ignore_unused(index);
            BOOST_ASSERT(node_id_iterator != used_node_id_list.end());
            BOOST_ASSERT(node_iterator != all_nodes_list_end);
            BOOST_ASSERT(*node_id_iterator >= node_iterator->node_id);
            while (*node_id_iterator > node_iterator->node_id &&
                   node_iterator != all_nodes_list_end)
            {
                ++node_iterator;
            }
            if (node_iterator == all_nodes_list_end || *node_id_iterator < node_iterator->node_id)
            {
                throw util::exception(
                    "Invalid OSM data: Referenced non-existing node with ID " +
                    std::to_string(static_cast<std::uint64_t>(*node_id_iterator)));
            }
            BOOST_ASSERT(*node_id_iterator == node_iterator->node_id);

            ++node_id_iterator;

            used_nodes.emplace_back(*node_iterator++);
        }

        TIMER_STOP(write_nodes);
        log << "ok, after " << TIMER_SEC(write_nodes) << "s";
    }

    {
        util::UnbufferedLog log;
        log << "Writing barrier nodes     ... ";
        TIMER_START(write_nodes);
        for (const auto osm_id : barrier_nodes)
        {
            const auto node_id = mapExternalToInternalNodeID(
                used_node_id_list.begin(), used_node_id_list.end(), osm_id);
            if (node_id != SPECIAL_NODEID)
            {
                used_barrier_nodes.emplace(node_id);
            }
        }
        log << "ok, after " << TIMER_SEC(write_nodes) << "s";
    }

    util::Log() << "Processed " << max_internal_node_id << " nodes";
}

void ExtractionContainers::PrepareEdges(ScriptingEnvironment &scripting_environment)
{
    // Sort edges by start.
    {
        util::UnbufferedLog log;
        log << "Sorting edges by start    ... " << std::flush;
        TIMER_START(sort_edges_by_start);
        tbb::parallel_sort(all_edges_list.begin(), all_edges_list.end(), CmpEdgeByOSMStartID());
        TIMER_STOP(sort_edges_by_start);
        log << "ok, after " << TIMER_SEC(sort_edges_by_start) << "s";
    }

    {
        util::UnbufferedLog log;
        log << "Setting start coords      ... " << std::flush;
        TIMER_START(set_start_coords);
        // Traverse list of edges and nodes in parallel and set start coord
        auto node_iterator = all_nodes_list.begin();
        auto edge_iterator = all_edges_list.begin();

        const auto all_edges_list_end = all_edges_list.end();
        const auto all_nodes_list_end = all_nodes_list.end();

        while (edge_iterator != all_edges_list_end && node_iterator != all_nodes_list_end)
        {
            if (edge_iterator->result.osm_source_id < node_iterator->node_id)
            {
                util::Log(logDEBUG)
                    << "Found invalid node reference " << edge_iterator->result.source;
                edge_iterator->result.source = SPECIAL_NODEID;
                ++edge_iterator;
                continue;
            }
            if (edge_iterator->result.osm_source_id > node_iterator->node_id)
            {
                node_iterator++;
                continue;
            }

            // remove loops
            if (edge_iterator->result.osm_source_id == edge_iterator->result.osm_target_id)
            {
                edge_iterator->result.source = SPECIAL_NODEID;
                edge_iterator->result.target = SPECIAL_NODEID;
                ++edge_iterator;
                continue;
            }

            BOOST_ASSERT(edge_iterator->result.osm_source_id == node_iterator->node_id);

            // assign new node id
            const auto node_id = mapExternalToInternalNodeID(
                used_node_id_list.begin(), used_node_id_list.end(), node_iterator->node_id);
            BOOST_ASSERT(node_id != SPECIAL_NODEID);
            edge_iterator->result.source = node_id;

            edge_iterator->source_coordinate.lat = node_iterator->lat;
            edge_iterator->source_coordinate.lon = node_iterator->lon;
            ++edge_iterator;
        }

        // Remove all remaining edges. They are invalid because there are no corresponding nodes for
        // them. This happens when using osmosis with bbox or polygon to extract smaller areas.
        auto markSourcesInvalid = [](InternalExtractorEdge &edge)
        {
            util::Log(logDEBUG) << "Found invalid node reference " << edge.result.source;
            edge.result.source = SPECIAL_NODEID;
            edge.result.osm_source_id = SPECIAL_OSM_NODEID;
        };
        std::for_each(edge_iterator, all_edges_list_end, markSourcesInvalid);
        TIMER_STOP(set_start_coords);
        log << "ok, after " << TIMER_SEC(set_start_coords) << "s";
    }

    {
        // Sort Edges by target
        util::UnbufferedLog log;
        log << "Sorting edges by target   ... " << std::flush;
        TIMER_START(sort_edges_by_target);
        tbb::parallel_sort(all_edges_list.begin(), all_edges_list.end(), CmpEdgeByOSMTargetID());
        TIMER_STOP(sort_edges_by_target);
        log << "ok, after " << TIMER_SEC(sort_edges_by_target) << "s";
    }

    {
        // Compute edge weights
        util::UnbufferedLog log;
        log << "Computing edge weights    ... " << std::flush;
        TIMER_START(compute_weights);
        auto node_iterator = all_nodes_list.begin();
        auto edge_iterator = all_edges_list.begin();
        const auto all_edges_list_end_ = all_edges_list.end();
        const auto all_nodes_list_end_ = all_nodes_list.end();

        const auto weight_multiplier =
            scripting_environment.GetProfileProperties().GetWeightMultiplier();

        while (edge_iterator != all_edges_list_end_ && node_iterator != all_nodes_list_end_)
        {
            // skip all invalid edges
            if (edge_iterator->result.source == SPECIAL_NODEID)
            {
                ++edge_iterator;
                continue;
            }

            if (edge_iterator->result.osm_target_id < node_iterator->node_id)
            {
                util::Log(logDEBUG) << "Found invalid node reference "
                                    << static_cast<uint64_t>(edge_iterator->result.osm_target_id);
                edge_iterator->result.target = SPECIAL_NODEID;
                ++edge_iterator;
                continue;
            }
            if (edge_iterator->result.osm_target_id > node_iterator->node_id)
            {
                ++node_iterator;
                continue;
            }

            BOOST_ASSERT(edge_iterator->result.osm_target_id == node_iterator->node_id);
            BOOST_ASSERT(edge_iterator->source_coordinate.lat !=
                         util::FixedLatitude{std::numeric_limits<std::int32_t>::min()});
            BOOST_ASSERT(edge_iterator->source_coordinate.lon !=
                         util::FixedLongitude{std::numeric_limits<std::int32_t>::min()});

            util::Coordinate source_coord(edge_iterator->source_coordinate);
            util::Coordinate target_coord{node_iterator->lon, node_iterator->lat};

            // flip source and target coordinates if segment is in backward direction only
            if (!edge_iterator->result.flags.forward && edge_iterator->result.flags.backward)
                std::swap(source_coord, target_coord);

            const auto distance =
                util::coordinate_calculation::greatCircleDistance(source_coord, target_coord);
            const auto weight = edge_iterator->weight_data(distance);
            const auto duration = edge_iterator->duration_data(distance);

            const auto accurate_distance =
                util::coordinate_calculation::greatCircleDistance(source_coord, target_coord);

            ExtractionSegment segment(source_coord,
                                      target_coord,
                                      distance,
                                      weight,
                                      duration,
                                      edge_iterator->result.flags);
            scripting_environment.ProcessSegment(segment);

            auto &edge = edge_iterator->result;
            edge.weight = std::max<EdgeWeight>(
                {1}, to_alias<EdgeWeight>(std::round(segment.weight * weight_multiplier)));
            edge.duration = std::max<EdgeDuration>(
                {1}, to_alias<EdgeDuration>(std::round(segment.duration * 10.)));
            edge.distance = to_alias<EdgeDistance>(accurate_distance);

            // assign new node id
            const auto node_id = mapExternalToInternalNodeID(
                used_node_id_list.begin(), used_node_id_list.end(), node_iterator->node_id);
            BOOST_ASSERT(node_id != SPECIAL_NODEID);
            edge.target = node_id;

            // orient edges consistently: source id < target id
            // important for multi-edge removal
            if (edge.source > edge.target)
            {
                std::swap(edge.source, edge.target);

                // std::swap does not work with bit-fields
                bool temp = edge.flags.forward;
                edge.flags.forward = edge.flags.backward;
                edge.flags.backward = temp;
            }
            ++edge_iterator;
        }

        // Remove all remaining edges. They are invalid because there are no corresponding nodes for
        // them. This happens when using osmosis with bbox or polygon to extract smaller areas.
        auto markTargetsInvalid = [](InternalExtractorEdge &edge)
        {
            util::Log(logDEBUG) << "Found invalid node reference " << edge.result.target;
            edge.result.target = SPECIAL_NODEID;
        };
        std::for_each(edge_iterator, all_edges_list_end_, markTargetsInvalid);
        TIMER_STOP(compute_weights);
        log << "ok, after " << TIMER_SEC(compute_weights) << "s";
    }

    // Sort edges by start.
    {
        util::UnbufferedLog log;
        log << "Sorting edges by renumbered start ... ";
        TIMER_START(sort_edges_by_renumbered_start);
        tbb::parallel_sort(all_edges_list.begin(),
                           all_edges_list.end(),
                           CmpEdgeByInternalSourceTargetAndName{
                               all_edges_annotation_data_list, name_char_data, name_offsets});
        TIMER_STOP(sort_edges_by_renumbered_start);
        log << "ok, after " << TIMER_SEC(sort_edges_by_renumbered_start) << "s";
    }

    BOOST_ASSERT(all_edges_list.size() > 0);
    for (std::size_t i = 0; i < all_edges_list.size();)
    {
        // only invalid edges left
        if (all_edges_list[i].result.source == SPECIAL_NODEID)
        {
            break;
        }
        // skip invalid edges
        if (all_edges_list[i].result.target == SPECIAL_NODEID)
        {
            ++i;
            continue;
        }

        std::size_t start_idx = i;
        NodeID source = all_edges_list[i].result.source;
        NodeID target = all_edges_list[i].result.target;

        auto min_forward = std::make_pair(MAXIMAL_EDGE_WEIGHT, MAXIMAL_EDGE_DURATION);
        auto min_backward = std::make_pair(MAXIMAL_EDGE_WEIGHT, MAXIMAL_EDGE_DURATION);
        std::size_t min_forward_idx = std::numeric_limits<std::size_t>::max();
        std::size_t min_backward_idx = std::numeric_limits<std::size_t>::max();

        // find minimal edge in both directions
        while (i < all_edges_list.size() && all_edges_list[i].result.source == source &&
               all_edges_list[i].result.target == target)
        {
            const auto &result = all_edges_list[i].result;
            const auto value = std::make_pair(result.weight, result.duration);
            if (result.flags.forward && value < min_forward)
            {
                min_forward_idx = i;
                min_forward = value;
            }
            if (result.flags.backward && value < min_backward)
            {
                min_backward_idx = i;
                min_backward = value;
            }

            // this also increments the outer loop counter!
            i++;
        }

        BOOST_ASSERT(min_forward_idx == std::numeric_limits<std::size_t>::max() ||
                     min_forward_idx < i);
        BOOST_ASSERT(min_backward_idx == std::numeric_limits<std::size_t>::max() ||
                     min_backward_idx < i);
        BOOST_ASSERT(min_backward_idx != std::numeric_limits<std::size_t>::max() ||
                     min_forward_idx != std::numeric_limits<std::size_t>::max());

        if (min_backward_idx == min_forward_idx)
        {
            all_edges_list[min_forward_idx].result.flags.is_split = false;
            all_edges_list[min_forward_idx].result.flags.forward = true;
            all_edges_list[min_forward_idx].result.flags.backward = true;
        }
        else
        {
            bool has_forward = min_forward_idx != std::numeric_limits<std::size_t>::max();
            bool has_backward = min_backward_idx != std::numeric_limits<std::size_t>::max();
            if (has_forward)
            {
                all_edges_list[min_forward_idx].result.flags.forward = true;
                all_edges_list[min_forward_idx].result.flags.backward = false;
                all_edges_list[min_forward_idx].result.flags.is_split = has_backward;
            }
            if (has_backward)
            {
                std::swap(all_edges_list[min_backward_idx].result.source,
                          all_edges_list[min_backward_idx].result.target);
                all_edges_list[min_backward_idx].result.flags.forward = true;
                all_edges_list[min_backward_idx].result.flags.backward = false;
                all_edges_list[min_backward_idx].result.flags.is_split = has_forward;
            }
        }

        // invalidate all unused edges
        for (std::size_t j = start_idx; j < i; j++)
        {
            if (j == min_forward_idx || j == min_backward_idx)
            {
                continue;
            }
            all_edges_list[j].result.source = SPECIAL_NODEID;
            all_edges_list[j].result.target = SPECIAL_NODEID;
        }
    }

    all_nodes_list.clear(); // free all_nodes_list before allocation of used_edges
    all_nodes_list.shrink_to_fit();

    used_edges.reserve(all_edges_list.size());
    {
        util::UnbufferedLog log;
        log << "Writing used edges       ... " << std::flush;
        TIMER_START(write_edges);
        // Traverse list of edges and nodes in parallel and set target coord

        for (const auto &edge : all_edges_list)
        {
            if (edge.result.source == SPECIAL_NODEID || edge.result.target == SPECIAL_NODEID)
            {
                continue;
            }

            // IMPORTANT: here, we're using slicing to only write the data from the base
            // class of NodeBasedEdgeWithOSM
            used_edges.push_back(edge.result);
        }

        if (used_edges.size() > std::numeric_limits<uint32_t>::max())
        {
            throw util::exception("There are too many edges, OSRM only supports 2^32" + SOURCE_REF);
        }

        TIMER_STOP(write_edges);
        log << "ok, after " << TIMER_SEC(write_edges) << "s";
        log << " -- Processed " << used_edges.size() << " edges";
    }
}

ExtractionContainers::ReferencedWays ExtractionContainers::IdentifyManeuverOverrideWays()
{
    ReferencedWays maneuver_override_ways;

    // prepare for extracting source/destination nodes for all maneuvers
    util::UnbufferedLog log;
    log << "Collecting way information on " << external_maneuver_overrides_list.size()
        << " maneuver overrides...";
    TIMER_START(identify_maneuver_override_ways);

    const auto mark_ids = [&](auto const &external_maneuver_override)
    {
        NodesOfWay dummy_segment{MAX_OSM_WAYID, {MAX_OSM_NODEID, MAX_OSM_NODEID}};
        const auto &turn_path = external_maneuver_override.turn_path;
        maneuver_override_ways[turn_path.From()] = dummy_segment;
        maneuver_override_ways[turn_path.To()] = dummy_segment;
        if (external_maneuver_override.turn_path.Type() == TurnPathType::VIA_WAY_TURN_PATH)
        {
            const auto &way = turn_path.AsViaWayPath();
            for (const auto &via : way.via)
            {
                maneuver_override_ways[via] = dummy_segment;
            }
        }
    };

    // First, make an empty hashtable keyed by the ways referenced
    // by the maneuver overrides
    std::for_each(
        external_maneuver_overrides_list.begin(), external_maneuver_overrides_list.end(), mark_ids);

    const auto set_ids = [&](size_t way_list_idx, auto const &way_id)
    {
        auto itr = maneuver_override_ways.find(way_id);
        if (itr != maneuver_override_ways.end())
        {
            auto node_start_itr = used_node_id_list.begin() + way_node_id_offsets[way_list_idx];
            auto node_end_itr = used_node_id_list.begin() + way_node_id_offsets[way_list_idx + 1];
            itr->second = NodesOfWay(way_id, std::vector<OSMNodeID>(node_start_itr, node_end_itr));
        }
    };

    // Then, populate the values in that hashtable for only the ways
    // referenced
    util::for_each_indexed(ways_list, set_ids);

    TIMER_STOP(identify_maneuver_override_ways);
    log << "ok, after " << TIMER_SEC(identify_maneuver_override_ways) << "s";

    return maneuver_override_ways;
}

void ExtractionContainers::PrepareTrafficSignals(
    const ExtractionContainers::ReferencedTrafficSignals &referenced_traffic_signals)
{
    const auto &bidirectional_signal_nodes = referenced_traffic_signals.first;
    const auto &unidirectional_signal_segments = referenced_traffic_signals.second;

    util::UnbufferedLog log;
    log << "Preparing traffic light signals for " << bidirectional_signal_nodes.size()
        << " bidirectional, " << unidirectional_signal_segments.size()
        << " unidirectional nodes ...";
    TIMER_START(prepare_traffic_signals);

    std::unordered_set<NodeID> bidirectional;
    std::unordered_set<std::pair<NodeID, NodeID>> unidirectional;

    for (const auto &osm_node : bidirectional_signal_nodes)
    {
        const auto node_id = mapExternalToInternalNodeID(
            used_node_id_list.begin(), used_node_id_list.end(), osm_node);
        if (node_id != SPECIAL_NODEID)
        {
            bidirectional.insert(node_id);
        }
    }
    for (const auto &to_from : unidirectional_signal_segments)
    {
        const auto to_node_id = mapExternalToInternalNodeID(
            used_node_id_list.begin(), used_node_id_list.end(), to_from.first);
        const auto from_node_id = mapExternalToInternalNodeID(
            used_node_id_list.begin(), used_node_id_list.end(), to_from.second);
        if (from_node_id != SPECIAL_NODEID && to_node_id != SPECIAL_NODEID)
        {
            unidirectional.insert({from_node_id, to_node_id});
        }
    }

    internal_traffic_signals.bidirectional_nodes = std::move(bidirectional);
    internal_traffic_signals.unidirectional_segments = std::move(unidirectional);

    TIMER_STOP(prepare_traffic_signals);
    log << "ok, after " << TIMER_SEC(prepare_traffic_signals) << "s";
}

void ExtractionContainers::PrepareManeuverOverrides(const ReferencedWays &maneuver_override_ways)
{
    auto const osm_node_to_internal_nbn = [&](auto const osm_node)
    {
        auto internal = mapExternalToInternalNodeID(
            used_node_id_list.begin(), used_node_id_list.end(), osm_node);
        if (internal == SPECIAL_NODEID)
        {
            util::Log(logDEBUG) << "Maneuver override references invalid node: " << osm_node;
        }
        return internal;
    };

    const auto strings_to_turn_type_and_direction =
        [](const std::string &turn_string, const std::string &direction_string)
    {
        auto result = std::make_pair(guidance::TurnType::MaxTurnType,
                                     guidance::DirectionModifier::MaxDirectionModifier);

        if (turn_string == "uturn")
        {
            result.first = guidance::TurnType::Turn;
            result.second = guidance::DirectionModifier::UTurn;
        }
        else if (turn_string == "continue")
        {
            result.first = guidance::TurnType::Continue;
        }
        else if (turn_string == "turn")
        {
            result.first = guidance::TurnType::Turn;
        }
        else if (turn_string == "fork")
        {
            result.first = guidance::TurnType::Fork;
        }
        else if (turn_string == "suppress")
        {
            result.first = guidance::TurnType::Suppressed;
        }

        // Directions
        if (direction_string == "left")
        {
            result.second = guidance::DirectionModifier::Left;
        }
        else if (direction_string == "slight_left")
        {
            result.second = guidance::DirectionModifier::SlightLeft;
        }
        else if (direction_string == "sharp_left")
        {
            result.second = guidance::DirectionModifier::SharpLeft;
        }
        else if (direction_string == "sharp_right")
        {
            result.second = guidance::DirectionModifier::SharpRight;
        }
        else if (direction_string == "slight_right")
        {
            result.second = guidance::DirectionModifier::SlightRight;
        }
        else if (direction_string == "right")
        {
            result.second = guidance::DirectionModifier::Right;
        }
        else if (direction_string == "straight")
        {
            result.second = guidance::DirectionModifier::Straight;
        }

        return result;
    };

    // Transform an InternalManeuverOverride (based on WayIDs) into an OSRM override (base on
    // NodeIDs).
    // Returns true on successful transformation, false in case of invalid references.
    // Later, the UnresolvedManeuverOverride will be converted into a final ManeuverOverride
    // once the edge-based-node IDs are generated by the edge-based-graph-factory
    const auto transform = [&](const auto &external_type, auto &internal_type)
    {
        if (external_type.turn_path.Type() == TurnPathType::VIA_WAY_TURN_PATH)
        {
            auto const &external = external_type.turn_path.AsViaWayPath();
            internal_type.turn_path.node_or_way =
                get_via_way_path_from_OSM_ids(internal_type.Name(),
                                              maneuver_override_ways,
                                              external.from,
                                              external.to,
                                              external.via,
                                              osm_node_to_internal_nbn);
        }
        else
        {
            BOOST_ASSERT(external_type.turn_path.Type() == TurnPathType::VIA_NODE_TURN_PATH);
            auto const &external = external_type.turn_path.AsViaNodePath();
            internal_type.turn_path.node_or_way =
                get_via_node_path_from_OSM_ids(internal_type.Name(),
                                               maneuver_override_ways,
                                               external.from,
                                               external.to,
                                               external.via,
                                               osm_node_to_internal_nbn);
        }

        internal_type.instruction_node = osm_node_to_internal_nbn(external_type.via_node),
        std::tie(internal_type.override_type, internal_type.direction) =
            strings_to_turn_type_and_direction(external_type.maneuver, external_type.direction);

        return internal_type.Valid();
    };

    const auto transform_into_internal_types =
        [&](const InputManeuverOverride &external_maneuver_override)
    {
        UnresolvedManeuverOverride internal_maneuver_override;
        if (transform(external_maneuver_override, internal_maneuver_override))
            internal_maneuver_overrides.push_back(std::move(internal_maneuver_override));
    };

    // Transforming the overrides into the dedicated internal types
    {
        util::UnbufferedLog log;
        log << "Collecting node information on " << external_maneuver_overrides_list.size()
            << " maneuver overrides...";
        TIMER_START(transform);
        std::for_each(external_maneuver_overrides_list.begin(),
                      external_maneuver_overrides_list.end(),
                      transform_into_internal_types);
        TIMER_STOP(transform);
        log << "ok, after " << TIMER_SEC(transform) << "s";
    }
}

ExtractionContainers::ReferencedWays ExtractionContainers::IdentifyRestrictionWays()
{
    // Contains the nodes of each way that is part of a restriction
    ReferencedWays restriction_ways;

    // Prepare for extracting nodes for all restrictions
    util::UnbufferedLog log;
    log << "Collecting way information on " << restrictions_list.size() << " restrictions...";
    TIMER_START(identify_restriction_ways);

    // Enter invalid IDs into the map to indicate that we want to find out about
    // nodes of these ways.
    const auto mark_ids = [&](auto const &turn_restriction)
    {
        NodesOfWay dummy_segment{MAX_OSM_WAYID, {MAX_OSM_NODEID, MAX_OSM_NODEID}};
        const auto &turn_path = turn_restriction.turn_path;
        restriction_ways[turn_path.From()] = dummy_segment;
        restriction_ways[turn_path.To()] = dummy_segment;
        if (turn_path.Type() == TurnPathType::VIA_WAY_TURN_PATH)
        {
            const auto &way = turn_path.AsViaWayPath();
            for (const auto &via : way.via)
            {
                restriction_ways[via] = dummy_segment;
            }
        }
    };

    std::for_each(restrictions_list.begin(), restrictions_list.end(), mark_ids);

    // Update the values for all ways already sporting SPECIAL_NODEID
    const auto set_ids = [&](const size_t way_list_idx, auto const &way_id)
    {
        auto itr = restriction_ways.find(way_id);
        if (itr != restriction_ways.end())
        {
            const auto node_start_offset =
                used_node_id_list.begin() + way_node_id_offsets[way_list_idx];
            const auto node_end_offset =
                used_node_id_list.begin() + way_node_id_offsets[way_list_idx + 1];
            itr->second =
                NodesOfWay(way_id, std::vector<OSMNodeID>(node_start_offset, node_end_offset));
        }
    };

    util::for_each_indexed(ways_list, set_ids);
    TIMER_STOP(identify_restriction_ways);
    log << "ok, after " << TIMER_SEC(identify_restriction_ways) << "s";

    return restriction_ways;
}

ExtractionContainers::ReferencedTrafficSignals ExtractionContainers::IdentifyTrafficSignals()
{
    util::UnbufferedLog log;
    log << "Collecting traffic signal information on " << external_traffic_signals.size()
        << " signals...";
    TIMER_START(identify_traffic_signals);

    // Temporary store for nodes containing a unidirectional signal.
    std::unordered_map<OSMNodeID, TrafficLightClass::Direction> unidirectional_signals;

    // For each node that has a unidirectional traffic signal, we store the node(s)
    // that lead up to the signal.
    std::unordered_multimap<OSMNodeID, OSMNodeID> signal_segments;

    std::unordered_set<OSMNodeID> bidirectional_signals;

    const auto mark_signals = [&](auto const &traffic_signal)
    {
        if (traffic_signal.second == TrafficLightClass::DIRECTION_FORWARD ||
            traffic_signal.second == TrafficLightClass::DIRECTION_REVERSE)
        {
            unidirectional_signals.insert({traffic_signal.first, traffic_signal.second});
        }
        else
        {
            BOOST_ASSERT(traffic_signal.second == TrafficLightClass::DIRECTION_ALL);
            bidirectional_signals.insert(traffic_signal.first);
        }
    };
    std::for_each(external_traffic_signals.begin(), external_traffic_signals.end(), mark_signals);

    // Extract all the segments that lead up to unidirectional traffic signals.
    const auto set_segments = [&](const size_t way_list_idx, auto const & /*unused*/)
    {
        const auto node_start_offset =
            used_node_id_list.begin() + way_node_id_offsets[way_list_idx];
        const auto node_end_offset =
            used_node_id_list.begin() + way_node_id_offsets[way_list_idx + 1];

        for (auto node_it = node_start_offset; node_it < node_end_offset; node_it++)
        {
            const auto sig = unidirectional_signals.find(*node_it);
            if (sig != unidirectional_signals.end())
            {
                if (sig->second == TrafficLightClass::DIRECTION_FORWARD)
                {
                    if (node_it != node_start_offset)
                    {
                        // Previous node leads to signal
                        signal_segments.insert({*node_it, *(node_it - 1)});
                    }
                }
                else
                {
                    BOOST_ASSERT(sig->second == TrafficLightClass::DIRECTION_REVERSE);
                    if (node_it + 1 != node_end_offset)
                    {
                        // Next node leads to signal
                        signal_segments.insert({*node_it, *(node_it + 1)});
                    }
                }
            }
        }
    };
    util::for_each_indexed(ways_list.cbegin(), ways_list.cend(), set_segments);

    util::for_each_pair(
        signal_segments,
        [](const auto pair_a, const auto pair_b)
        {
            if (pair_a.first == pair_b.first)
            {
                // If a node is appearing multiple times in this map, then it's ambiguous.
                // The node is an intersection and the traffic direction is being use for multiple
                // ways. We can't be certain of the original intent. See:
                // https://wiki.openstreetmap.org/wiki/Key:traffic_signals:direction

                // OSRM will include the signal for all intersecting ways in the specified
                // direction, but let's flag this as a concern.
                util::Log(logWARNING)
                    << "OSM node " << pair_a.first
                    << " has a unidirectional traffic signal ambiguously applied to multiple ways";
            }
        });

    TIMER_STOP(identify_traffic_signals);
    log << "ok, after " << TIMER_SEC(identify_traffic_signals) << "s";

    return {std::move(bidirectional_signals), std::move(signal_segments)};
}

void ExtractionContainers::PrepareRestrictions(const ReferencedWays &restriction_ways)
{

    auto const to_internal = [&](auto const osm_node)
    {
        auto internal = mapExternalToInternalNodeID(
            used_node_id_list.begin(), used_node_id_list.end(), osm_node);
        if (internal == SPECIAL_NODEID)
        {
            util::Log(logDEBUG) << "Restriction references invalid node: " << osm_node;
        }
        return internal;
    };

    // Transform an OSMRestriction (based on WayIDs) into an OSRM restriction (base on NodeIDs).
    // Returns true on successful transformation, false in case of invalid references.
    const auto transform = [&](const auto &external_type, auto &internal_type)
    {
        if (external_type.turn_path.Type() == TurnPathType::VIA_WAY_TURN_PATH)
        {
            auto const &external = external_type.turn_path.AsViaWayPath();
            internal_type.turn_path.node_or_way =
                get_via_way_path_from_OSM_ids(internal_type.Name(),
                                              restriction_ways,
                                              external.from,
                                              external.to,
                                              external.via,
                                              to_internal);
        }
        else
        {
            BOOST_ASSERT(external_type.turn_path.Type() == TurnPathType::VIA_NODE_TURN_PATH);
            auto const &external = external_type.turn_path.AsViaNodePath();
            internal_type.turn_path.node_or_way =
                get_via_node_path_from_OSM_ids(internal_type.Name(),
                                               restriction_ways,
                                               external.from,
                                               external.to,
                                               external.via,
                                               to_internal);
        }
        internal_type.is_only = external_type.is_only;
        internal_type.condition = std::move(external_type.condition);
        return internal_type.Valid();
    };

    const auto transform_into_internal_types = [&](InputTurnRestriction &external_restriction)
    {
        TurnRestriction restriction;
        if (transform(external_restriction, restriction))
        {
            turn_restrictions.push_back(std::move(restriction));
        }
    };

    // Transforming the restrictions into the dedicated internal types
    {
        util::UnbufferedLog log;
        log << "Collecting node information on " << restrictions_list.size() << " restrictions...";
        TIMER_START(transform);
        std::for_each(
            restrictions_list.begin(), restrictions_list.end(), transform_into_internal_types);
        TIMER_STOP(transform);
        log << "ok, after " << TIMER_SEC(transform) << "s";
    }
}

} // namespace osrm::extractor
