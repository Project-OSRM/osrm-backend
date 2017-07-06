#include "extractor/extraction_containers.hpp"
#include "extractor/extraction_segment.hpp"
#include "extractor/extraction_way.hpp"
#include "extractor/restriction.hpp"
#include "extractor/serialization.hpp"

#include "util/coordinate_calculation.hpp"

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/fingerprint.hpp"
#include "util/log.hpp"
#include "util/name_table.hpp"
#include "util/timing_util.hpp"

#include "storage/io.hpp"

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/ref.hpp>

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

        if (lhs.result.name_id == rhs.result.name_id)
            return false;

        if (lhs.result.name_id == EMPTY_NAMEID)
            return false;

        if (rhs.result.name_id == EMPTY_NAMEID)
            return true;

        BOOST_ASSERT(!name_offsets.empty() && name_offsets.back() == name_data.size());
        const oe::ExtractionContainers::NameCharData::const_iterator data = name_data.begin();
        return std::lexicographical_compare(data + name_offsets[lhs.result.name_id],
                                            data + name_offsets[lhs.result.name_id + 1],
                                            data + name_offsets[rhs.result.name_id],
                                            data + name_offsets[rhs.result.name_id + 1]);
    }

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
}

namespace osrm
{
namespace extractor
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
}

/**
 * Processes the collected data and serializes it.
 * At this point nodes are still referenced by their OSM id.
 *
 * - map start-end nodes of ways to ways used in restrictions to compute compressed
 *   trippe representation
 * - filter nodes list to nodes that are referenced by ways
 * - merge edges with nodes to include location of start/end points and serialize
 *
 */
void ExtractionContainers::PrepareData(ScriptingEnvironment &scripting_environment,
                                       const std::string &osrm_path,
                                       const std::string &restrictions_file_name,
                                       const std::string &name_file_name)
{
    storage::io::FileWriter file_out(osrm_path, storage::io::FileWriter::GenerateFingerprint);

    PrepareNodes();
    WriteNodes(file_out);
    PrepareEdges(scripting_environment);
    all_nodes_list.clear(); // free all_nodes_list before allocation of normal_edges
    all_nodes_list.shrink_to_fit();
    WriteEdges(file_out);

    PrepareRestrictions();
    WriteConditionalRestrictions(restrictions_file_name);
    WriteCharData(name_file_name);
}

void ExtractionContainers::WriteCharData(const std::string &file_name)
{
    util::UnbufferedLog log;
    log << "writing street name index ... ";
    TIMER_START(write_index);
    storage::io::FileWriter file(file_name, storage::io::FileWriter::GenerateFingerprint);

    const util::NameTable::IndexedData indexed_data;
    indexed_data.write(file, name_offsets.begin(), name_offsets.end(), name_char_data.begin());

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
        tbb::parallel_sort(
            all_nodes_list.begin(), all_nodes_list.end(), [](const auto &left, const auto &right) {
                return left.node_id < right.node_id;
            });
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
            *used_nodes_iter = std::move(*ref_iter);
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
                util::Log(logDEBUG) << "Found invalid node reference "
                                    << edge_iterator->result.source;
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
        auto markSourcesInvalid = [](InternalExtractorEdge &edge) {
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
            if (!edge_iterator->result.forward && edge_iterator->result.backward)
                std::swap(source_coord, target_coord);

            const auto distance =
                util::coordinate_calculation::greatCircleDistance(source_coord, target_coord);
            const auto weight = edge_iterator->weight_data(distance);
            const auto duration = edge_iterator->duration_data(distance);

            ExtractionSegment segment(source_coord, target_coord, distance, weight, duration);
            scripting_environment.ProcessSegment(segment);

            auto &edge = edge_iterator->result;
            edge.weight = std::max<EdgeWeight>(1, std::round(segment.weight * weight_multiplier));
            edge.duration = std::max<EdgeWeight>(1, std::round(segment.duration * 10.));

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
                bool temp = edge.forward;
                edge.forward = edge.backward;
                edge.backward = temp;
            }
            ++edge_iterator;
        }

        // Remove all remaining edges. They are invalid because there are no corresponding nodes for
        // them. This happens when using osmosis with bbox or polygon to extract smaller areas.
        auto markTargetsInvalid = [](InternalExtractorEdge &edge) {
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
        std::mutex name_data_mutex;
        tbb::parallel_sort(all_edges_list.begin(),
                           all_edges_list.end(),
                           CmpEdgeByInternalSourceTargetAndName{name_char_data, name_offsets});
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

        auto min_forward = std::make_pair(std::numeric_limits<EdgeWeight>::max(),
                                          std::numeric_limits<EdgeWeight>::max());
        auto min_backward = std::make_pair(std::numeric_limits<EdgeWeight>::max(),
                                           std::numeric_limits<EdgeWeight>::max());
        std::size_t min_forward_idx = std::numeric_limits<std::size_t>::max();
        std::size_t min_backward_idx = std::numeric_limits<std::size_t>::max();

        // find minimal edge in both directions
        while (i < all_edges_list.size() && all_edges_list[i].result.source == source &&
               all_edges_list[i].result.target == target)
        {
            const auto &result = all_edges_list[i].result;
            const auto value = std::make_pair(result.weight, result.duration);
            if (result.forward && value < min_forward)
            {
                min_forward_idx = i;
                min_forward = value;
            }
            if (result.backward && value < min_backward)
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
            all_edges_list[min_forward_idx].result.is_split = false;
            all_edges_list[min_forward_idx].result.forward = true;
            all_edges_list[min_forward_idx].result.backward = true;
        }
        else
        {
            bool has_forward = min_forward_idx != std::numeric_limits<std::size_t>::max();
            bool has_backward = min_backward_idx != std::numeric_limits<std::size_t>::max();
            if (has_forward)
            {
                all_edges_list[min_forward_idx].result.forward = true;
                all_edges_list[min_forward_idx].result.backward = false;
                all_edges_list[min_forward_idx].result.is_split = has_backward;
            }
            if (has_backward)
            {
                std::swap(all_edges_list[min_backward_idx].result.source,
                          all_edges_list[min_backward_idx].result.target);
                all_edges_list[min_backward_idx].result.forward = true;
                all_edges_list[min_backward_idx].result.backward = false;
                all_edges_list[min_backward_idx].result.is_split = has_forward;
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
}

void ExtractionContainers::WriteEdges(storage::io::FileWriter &file_out) const
{
    std::vector<NodeBasedEdge> normal_edges;
    normal_edges.reserve(all_edges_list.size());
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
            normal_edges.push_back(edge.result);
        }

        if (normal_edges.size() > std::numeric_limits<uint32_t>::max())
        {
            throw util::exception("There are too many edges, OSRM only supports 2^32" + SOURCE_REF);
        }

        file_out.WriteElementCount64(normal_edges.size());
        file_out.WriteFrom(normal_edges.data(), normal_edges.size());

        TIMER_STOP(write_edges);
        log << "ok, after " << TIMER_SEC(write_edges) << "s";
        log << "Processed " << normal_edges.size() << " edges";
    }
}

void ExtractionContainers::WriteNodes(storage::io::FileWriter &file_out) const
{
    {
        // write dummy value, will be overwritten later
        util::UnbufferedLog log;
        log << "setting number of nodes   ... " << std::flush;
        file_out.WriteElementCount64(max_internal_node_id);
        log << "ok";
    }

    {
        util::UnbufferedLog log;
        log << "Confirming/Writing used nodes     ... ";
        TIMER_START(write_nodes);
        // identify all used nodes by a merging step of two sorted lists
        auto node_iterator = all_nodes_list.begin();
        auto node_id_iterator = used_node_id_list.begin();
        const auto used_node_id_list_end = used_node_id_list.end();
        const auto all_nodes_list_end = all_nodes_list.end();

        while (node_id_iterator != used_node_id_list_end && node_iterator != all_nodes_list_end)
        {
            if (*node_id_iterator < node_iterator->node_id)
            {
                ++node_id_iterator;
                continue;
            }
            if (*node_id_iterator > node_iterator->node_id)
            {
                ++node_iterator;
                continue;
            }
            BOOST_ASSERT(*node_id_iterator == node_iterator->node_id);

            file_out.WriteOne((*node_iterator));

            ++node_id_iterator;
            ++node_iterator;
        }
        TIMER_STOP(write_nodes);
        log << "ok, after " << TIMER_SEC(write_nodes) << "s";
    }

    {
        util::UnbufferedLog log;
        log << "Writing barrier nodes     ... ";
        TIMER_START(write_nodes);
        std::vector<NodeID> internal_barrier_nodes;
        for (const auto osm_id : barrier_nodes)
        {
            const auto node_id = mapExternalToInternalNodeID(
                used_node_id_list.begin(), used_node_id_list.end(), osm_id);
            if (node_id != SPECIAL_NODEID)
            {
                internal_barrier_nodes.push_back(node_id);
            }
        }
        storage::serialization::write(file_out, internal_barrier_nodes);
        log << "ok, after " << TIMER_SEC(write_nodes) << "s";
    }

    {
        util::UnbufferedLog log;
        log << "Writing traffic light nodes     ... ";
        TIMER_START(write_nodes);
        std::vector<NodeID> internal_traffic_lights;
        for (const auto osm_id : traffic_lights)
        {
            const auto node_id = mapExternalToInternalNodeID(
                used_node_id_list.begin(), used_node_id_list.end(), osm_id);
            if (node_id != SPECIAL_NODEID)
            {
                internal_traffic_lights.push_back(node_id);
            }
        }
        storage::serialization::write(file_out, internal_traffic_lights);
        log << "ok, after " << TIMER_SEC(write_nodes) << "s";
    }

    util::Log() << "Processed " << max_internal_node_id << " nodes";
}

void ExtractionContainers::WriteConditionalRestrictions(const std::string &path)
{
    std::uint64_t written_restriction_count = conditional_turn_restrictions.size();
    storage::io::FileWriter restrictions_out_file(path,
                                                  storage::io::FileWriter::GenerateFingerprint);
    serialization::write(restrictions_out_file, conditional_turn_restrictions);
    util::Log() << "number of conditional restrictions written to disk: "
                << written_restriction_count;
}

void ExtractionContainers::PrepareRestrictions()
{

    {
        util::UnbufferedLog log;
        log << "Sorting used ways         ... ";
        TIMER_START(sort_ways);
        tbb::parallel_sort(way_start_end_id_list.begin(),
                           way_start_end_id_list.end(),
                           FirstAndLastSegmentOfWayCompare());
        TIMER_STOP(sort_ways);
        log << "ok, after " << TIMER_SEC(sort_ways) << "s";
    }

    // contain the start/end nodes of each way that is part of an restriction
    std::unordered_map<OSMWayID, FirstAndLastSegmentOfWay> referenced_ways;

    // enter invalid IDs into the above maps to indicate that we want to find out about start/end
    // nodes of these ways
    const auto mark_ids = [&](auto const &turn_restriction) {
        FirstAndLastSegmentOfWay dummy_segment{
            MAX_OSM_WAYID, MAX_OSM_NODEID, MAX_OSM_NODEID, MAX_OSM_NODEID, MAX_OSM_NODEID};
        if (turn_restriction.Type() == RestrictionType::WAY_RESTRICTION)
        {
            const auto &way = turn_restriction.AsWayRestriction();
            referenced_ways[OSMWayID{way.from}] = dummy_segment;
            referenced_ways[OSMWayID{way.to}] = dummy_segment;
            referenced_ways[OSMWayID{way.via}] = dummy_segment;
        }
        else
        {
            BOOST_ASSERT(turn_restriction.Type() == RestrictionType::NODE_RESTRICTION);
            const auto &node = turn_restriction.AsNodeRestriction();
            referenced_ways[OSMWayID{node.from}] = dummy_segment;
            referenced_ways[OSMWayID{node.to}] = dummy_segment;
        }
    };

    // update the values for all edges already sporting SPECIAL_NODEID
    const auto set_ids = [&](auto const &start_end) {
        auto itr = referenced_ways.find(start_end.way_id);
        if (itr != referenced_ways.end())
            itr->second = start_end;
    };

    // prepare for extracting source/destination nodes for all restrictions
    {
        util::UnbufferedLog log;
        log << "Collecting start/end information on " << restrictions_list.size()
            << " restrictions...";
        TIMER_START(prepare_restrictions);
        std::for_each(restrictions_list.begin(), restrictions_list.end(), mark_ids);
        std::for_each(way_start_end_id_list.cbegin(), way_start_end_id_list.cend(), set_ids);
        TIMER_STOP(prepare_restrictions);
        log << "ok, after " << TIMER_SEC(prepare_restrictions) << "s";
    }

    auto const to_internal = [&](auto const osm_node) {
        auto internal = mapExternalToInternalNodeID(
            used_node_id_list.begin(), used_node_id_list.end(), OSMNodeID{osm_node});
        if (internal == SPECIAL_NODEID)
        {
            util::Log(logDEBUG) << "Restriction references invalid node: " << osm_node;
        }
        return internal;
    };

    // Given:
    // a -- b - ????????? - c -- d
    // Given
    // a -- b - ????????? - c -- d as via segment
    // and either
    // d -- e - ????????? - f -- g or
    // h -- i - ????????? - j -- a
    // (d,e) or (j,a) as entry-segment
    auto const find_node_restriction =
        [&](auto const &segment, auto const &via_segment, auto const via_node) {
            // connected at the front of the segment
            if (via_node == MAX_OSM_NODEID || segment.first_segment_source_id == via_node)
            {
                if (segment.first_segment_source_id == via_segment.first_segment_source_id)
                {
                    return NodeRestriction{to_internal(segment.first_segment_target_id),
                                           to_internal(segment.first_segment_source_id),
                                           to_internal(via_segment.first_segment_target_id)};
                }
                else if (segment.first_segment_source_id == via_segment.last_segment_target_id)
                {
                    return NodeRestriction{to_internal(segment.first_segment_target_id),
                                           to_internal(segment.first_segment_source_id),
                                           to_internal(via_segment.last_segment_source_id)};
                }
            }

            // connected at the end of the segment
            if (via_node == MAX_OSM_NODEID || segment.last_segment_target_id == via_node)
            {
                if (segment.last_segment_target_id == via_segment.first_segment_source_id)
                {
                    return NodeRestriction{to_internal(segment.last_segment_source_id),
                                           to_internal(segment.last_segment_target_id),
                                           to_internal(via_segment.first_segment_target_id)};
                }
                else if (segment.last_segment_target_id == via_segment.last_segment_target_id)
                {
                    return NodeRestriction{to_internal(segment.last_segment_source_id),
                                           to_internal(segment.last_segment_target_id),
                                           to_internal(via_segment.last_segment_source_id)};
                }
            }

            // unconnected
            util::Log(logDEBUG) << "Restriction references unconnected way: " << segment.way_id;
            return NodeRestriction{SPECIAL_NODEID, SPECIAL_NODEID, SPECIAL_NODEID};
        };

    // translate the turn from one segment onto another into a node restriction (the ways can only
    // be connected at a single location)
    auto const get_node_restriction_from_OSM_ids = [&](
        auto const from_id, auto const to_id, const OSMNodeID via_node = MAX_OSM_NODEID) {
        auto const from_segment_itr = referenced_ways.find(OSMWayID{from_id});
        if (from_segment_itr->second.way_id != OSMWayID{from_id})
        {
            util::Log(logDEBUG) << "Restriction references invalid way: " << from_id;
            return NodeRestriction{SPECIAL_NODEID, SPECIAL_NODEID, SPECIAL_NODEID};
        }

        auto const to_segment_itr = referenced_ways.find(OSMWayID{to_id});
        if (to_segment_itr->second.way_id != OSMWayID{to_id})
        {
            util::Log(logDEBUG) << "Restriction references invalid way: " << to_id;
            return NodeRestriction{SPECIAL_NODEID, SPECIAL_NODEID, SPECIAL_NODEID};
        }
        return find_node_restriction(from_segment_itr->second, to_segment_itr->second, via_node);
    };

    // transform an OSMRestriction (based on WayIDs) into an OSRM restriction (base on NodeIDs)
    // returns true on successful transformation, false in case of invalid references
    const auto transform = [&](auto const &external_type, auto &internal_type) {
        if (external_type.Type() == RestrictionType::WAY_RESTRICTION)
        {
            auto const &external = external_type.AsWayRestriction();
            // check if we were able to resolve all the involved ways
            auto const from_restriction =
                get_node_restriction_from_OSM_ids(external.from, external.via);
            auto const to_restriction =
                get_node_restriction_from_OSM_ids(external.via, external.to);

            // failed to translate either of the involved nodes?
            if (!from_restriction.Valid() || !to_restriction.Valid())
                return false;

            // point located at both via and segment is alway on `second`, to FSSF is the order we
            // need
            WayRestriction way_restriction{from_restriction, to_restriction};
            internal_type.node_or_way = std::move(way_restriction);
            return true;
        }
        else
        {
            BOOST_ASSERT(external_type.Type() == RestrictionType::NODE_RESTRICTION);
            auto const &external = external_type.AsNodeRestriction();
            auto const via_node = to_internal(external.via);

            // check if we were able to resolve all the involved ways
            auto restriction = get_node_restriction_from_OSM_ids(
                external.from, external.to, OSMNodeID{external.via});

            if (!restriction.Valid())
            {
                std::cout << " >>> Invalid" << std::endl;
                return false;
            }

            if (restriction.via != via_node)
            {
                util::Log(logDEBUG) << "Restriction references invalid way: " << external.via;
                return false;
            }

            internal_type.node_or_way = std::move(restriction);
            return true;
        }
    };

    // wrapper function to handle distinction between conditional and unconditional turn
    // restrictions
    const auto transform_into_internal_types = [&](auto &external_restriction) {
        // unconditional restriction
        if (external_restriction.condition.empty())
        {
            TurnRestriction restriction;
            restriction.flags = external_restriction.flags;
            if (transform(external_restriction, restriction))
                unconditional_turn_restrictions.push_back(restriction);
        }
        // conditional turn restriction
        else
        {
            ConditionalTurnRestriction restriction;
            restriction.flags = external_restriction.flags;
            restriction.condition = std::move(external_restriction.condition);
            if (transform(external_restriction, restriction))
                conditional_turn_restrictions.push_back(restriction);
        }
    };

    // Transforming the restrictions into the dedicated internal types
    {
        util::UnbufferedLog log;
        log << "Collecting start/end information on " << restrictions_list.size()
            << " restrictions...";
        TIMER_START(transform);
        std::for_each(
            restrictions_list.begin(), restrictions_list.end(), transform_into_internal_types);
        TIMER_STOP(transform);
        log << "ok, after " << TIMER_SEC(transform) << "s";
    }
}

} // namespace extractor
} // namespace osrm
