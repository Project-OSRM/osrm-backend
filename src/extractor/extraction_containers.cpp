#include "extractor/extraction_containers.hpp"
#include "extractor/extraction_segment.hpp"
#include "extractor/extraction_way.hpp"
#include "extractor/files.hpp"
#include "extractor/name_table.hpp"
#include "extractor/restriction.hpp"
#include "extractor/serialization.hpp"

#include "util/coordinate_calculation.hpp"

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/fingerprint.hpp"
#include "util/log.hpp"
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
                                       const std::string &name_file_name)
{
    storage::tar::FileWriter writer(osrm_path, storage::tar::FileWriter::GenerateFingerprint);

    PrepareNodes();
    WriteNodes(writer);
    PrepareEdges(scripting_environment);
    all_nodes_list.clear(); // free all_nodes_list before allocation of normal_edges
    all_nodes_list.shrink_to_fit();
    WriteEdges(writer);
    WriteMetadata(writer);

    /* Sort these so that searching is a bit faster later on */
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

    PrepareManeuverOverrides();
    PrepareRestrictions();
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
            if (!edge_iterator->result.flags.forward && edge_iterator->result.flags.backward)
                std::swap(source_coord, target_coord);

            const auto distance =
                util::coordinate_calculation::greatCircleDistance(source_coord, target_coord);
            const auto weight = edge_iterator->weight_data(distance);
            const auto duration = edge_iterator->duration_data(distance);

            const auto accurate_distance =
                util::coordinate_calculation::fccApproximateDistance(source_coord, target_coord);

            ExtractionSegment segment(source_coord, target_coord, distance, weight, duration);
            scripting_environment.ProcessSegment(segment);

            auto &edge = edge_iterator->result;
            edge.weight = std::max<EdgeWeight>(1, std::round(segment.weight * weight_multiplier));
            edge.duration = std::max<EdgeWeight>(1, std::round(segment.duration * 10.));
            edge.distance = accurate_distance;

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
}

void ExtractionContainers::WriteEdges(storage::tar::FileWriter &writer) const
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

        storage::serialization::write(writer, "/extractor/edges", normal_edges);

        TIMER_STOP(write_edges);
        log << "ok, after " << TIMER_SEC(write_edges) << "s";
        log << " -- Processed " << normal_edges.size() << " edges";
    }
}

void ExtractionContainers::WriteMetadata(storage::tar::FileWriter &writer) const
{
    util::UnbufferedLog log;
    log << "Writing way meta-data     ... " << std::flush;
    TIMER_START(write_meta_data);

    storage::serialization::write(writer, "/extractor/annotations", all_edges_annotation_data_list);

    TIMER_STOP(write_meta_data);
    log << "ok, after " << TIMER_SEC(write_meta_data) << "s";
    log << " -- Metadata contains << " << all_edges_annotation_data_list.size() << " entries.";
}

void ExtractionContainers::WriteNodes(storage::tar::FileWriter &writer) const
{
    {
        util::UnbufferedLog log;
        log << "Confirming/Writing used nodes     ... ";
        TIMER_START(write_nodes);
        // identify all used nodes by a merging step of two sorted lists
        auto node_iterator = all_nodes_list.begin();
        auto node_id_iterator = used_node_id_list.begin();
        const auto all_nodes_list_end = all_nodes_list.end();

        const std::function<QueryNode()> encode_function = [&]() -> QueryNode {
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
            return *node_iterator++;
        };

        writer.WriteElementCount64("/extractor/nodes", used_node_id_list.size());
        writer.WriteStreaming<QueryNode>(
            "/extractor/nodes",
            boost::make_function_input_iterator(encode_function, boost::infinite()),
            used_node_id_list.size());

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
        storage::serialization::write(writer, "/extractor/barriers", internal_barrier_nodes);
        log << "ok, after " << TIMER_SEC(write_nodes) << "s";
    }

    {
        util::UnbufferedLog log;
        log << "Writing traffic light nodes     ... ";
        TIMER_START(write_nodes);
        std::vector<NodeID> internal_traffic_signals;
        for (const auto osm_id : traffic_signals)
        {
            const auto node_id = mapExternalToInternalNodeID(
                used_node_id_list.begin(), used_node_id_list.end(), osm_id);
            if (node_id != SPECIAL_NODEID)
            {
                internal_traffic_signals.push_back(node_id);
            }
        }
        storage::serialization::write(
            writer, "/extractor/traffic_lights", internal_traffic_signals);
        log << "ok, after " << TIMER_SEC(write_nodes) << "s";
    }

    util::Log() << "Processed " << max_internal_node_id << " nodes";
}

void ExtractionContainers::PrepareManeuverOverrides()
{
    std::unordered_map<OSMWayID, FirstAndLastSegmentOfWay> referenced_ways;

    // prepare for extracting source/destination nodes for all maneuvers
    {
        util::UnbufferedLog log;
        log << "Collecting start/end information on " << external_maneuver_overrides_list.size()
            << " maneuver overrides...";
        TIMER_START(prepare_maneuver_overrides);

        const auto mark_ids = [&](auto const &external_maneuver_override) {
            FirstAndLastSegmentOfWay dummy_segment{
                MAX_OSM_WAYID, MAX_OSM_NODEID, MAX_OSM_NODEID, MAX_OSM_NODEID, MAX_OSM_NODEID};
            std::for_each(external_maneuver_override.via_ways.begin(),
                          external_maneuver_override.via_ways.end(),
                          [&referenced_ways, dummy_segment](const auto &element) {
                              referenced_ways[element] = dummy_segment;
                          });
        };

        // First, make an empty hashtable keyed by the ways referenced
        // by the maneuver overrides
        std::for_each(external_maneuver_overrides_list.begin(),
                      external_maneuver_overrides_list.end(),
                      mark_ids);

        const auto set_ids = [&](auto const &start_end) {
            auto itr = referenced_ways.find(start_end.way_id);
            if (itr != referenced_ways.end())
                itr->second = start_end;
        };

        // Then, populate the values in that hashtable for only the ways
        // referenced
        std::for_each(way_start_end_id_list.cbegin(), way_start_end_id_list.cend(), set_ids);

        TIMER_STOP(prepare_maneuver_overrides);
        log << "ok, after " << TIMER_SEC(prepare_maneuver_overrides) << "s";
    }

    auto const osm_node_to_internal_nbn = [&](auto const osm_node) {
        auto internal = mapExternalToInternalNodeID(
            used_node_id_list.begin(), used_node_id_list.end(), osm_node);
        if (internal == SPECIAL_NODEID)
        {
            util::Log(logDEBUG) << "Maneuver override references invalid node: " << osm_node;
        }
        return internal;
    };

    // Given
    // a -- b - ????????? - c -- d as via segment
    // and either
    // d -- e - ????????? - f -- g or
    // h -- i - ????????? - j -- a
    // return
    // (d,e) or (j,a) as entry-segment

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
    auto const find_turn_from_way_tofrom_nodes = [&](auto const &from_segment,
                                                     auto const &to_segment) {

        if (from_segment.first_segment_source_id == to_segment.first_segment_source_id)
        {
            return NodeBasedTurn{osm_node_to_internal_nbn(from_segment.first_segment_target_id),
                                 osm_node_to_internal_nbn(from_segment.first_segment_source_id),
                                 osm_node_to_internal_nbn(to_segment.first_segment_target_id)};
        }
        else if (from_segment.first_segment_source_id == to_segment.last_segment_target_id)
        {
            return NodeBasedTurn{osm_node_to_internal_nbn(from_segment.first_segment_target_id),
                                 osm_node_to_internal_nbn(from_segment.first_segment_source_id),
                                 osm_node_to_internal_nbn(to_segment.last_segment_source_id)};
        }
        else if (from_segment.last_segment_target_id == to_segment.first_segment_source_id)
        {
            return NodeBasedTurn{osm_node_to_internal_nbn(from_segment.last_segment_source_id),
                                 osm_node_to_internal_nbn(from_segment.last_segment_target_id),
                                 osm_node_to_internal_nbn(to_segment.first_segment_target_id)};
        }
        else if (from_segment.last_segment_target_id == to_segment.last_segment_target_id)
        {
            return NodeBasedTurn{osm_node_to_internal_nbn(from_segment.last_segment_source_id),
                                 osm_node_to_internal_nbn(from_segment.last_segment_target_id),
                                 osm_node_to_internal_nbn(to_segment.last_segment_source_id)};
        }
        util::Log(logDEBUG) << "Maneuver override ways " << from_segment.way_id << " and "
                            << to_segment.way_id << " are not connected";
        return NodeBasedTurn{SPECIAL_NODEID, SPECIAL_NODEID, SPECIAL_NODEID};
    };

    auto const get_turn_from_way_pair = [&](const OSMWayID &from_id, const OSMWayID &to_id) {
        auto const from_segment_itr = referenced_ways.find(from_id);
        if (from_segment_itr->second.way_id != from_id)
        {
            util::Log(logDEBUG) << "Override references invalid way: " << from_id;
            return NodeBasedTurn{SPECIAL_NODEID, SPECIAL_NODEID, SPECIAL_NODEID};
        }

        auto const to_segment_itr = referenced_ways.find(to_id);
        if (to_segment_itr->second.way_id != to_id)
        {
            util::Log(logDEBUG) << "Override references invalid way: " << to_id;
            return NodeBasedTurn{SPECIAL_NODEID, SPECIAL_NODEID, SPECIAL_NODEID};
        }

        auto result =
            find_turn_from_way_tofrom_nodes(from_segment_itr->second, to_segment_itr->second);

        return result;
    };

    const auto strings_to_turn_type_and_direction = [](const std::string &turn_string,
                                                       const std::string &direction_string) {
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
    const auto transform = [&](const auto &external, auto &internal) {

        // Create a stub override
        auto maneuver_override =
            UnresolvedManeuverOverride{{},
                                       osm_node_to_internal_nbn(external.via_node),
                                       guidance::TurnType::Invalid,
                                       guidance::DirectionModifier::MaxDirectionModifier};

        // Convert Way IDs into node-based-node IDs
        // We iterate from back to front here because the first node in the node_sequence
        // must eventually be a source node, but all the others must be targets.
        // the get_internal_pairs_from_ways returns (source,target), so if we
        // iterate backwards, we will end up with source,target,target,target,target
        // in a sequence, which is what we want
        for (auto i = 0ul; i < external.via_ways.size() - 1; ++i)
        {
            // returns the two far ends of the referenced ways
            auto turn = get_turn_from_way_pair(external.via_ways[i], external.via_ways[i + 1]);

            maneuver_override.turn_sequence.push_back(turn);
        }

        // check if we were able to resolve all the involved ways
        // auto maneuver_override =
        //    get_maneuver_override_from_OSM_ids(external.from, external.to,
        //    external.via_node);

        std::tie(maneuver_override.override_type, maneuver_override.direction) =
            strings_to_turn_type_and_direction(external.maneuver, external.direction);

        if (!maneuver_override.Valid())
        {
            util::Log(logDEBUG) << "Override is invalid";
            return false;
        }

        internal = std::move(maneuver_override);
        return true;
    };

    const auto transform_into_internal_types =
        [&](const InputManeuverOverride &external_maneuver_override) {
            UnresolvedManeuverOverride internal_maneuver_override;
            if (transform(external_maneuver_override, internal_maneuver_override))
                internal_maneuver_overrides.push_back(std::move(internal_maneuver_override));
        };

    // Transforming the overrides into the dedicated internal types
    {
        util::UnbufferedLog log;
        log << "Collecting start/end information on " << external_maneuver_overrides_list.size()
            << " maneuver overrides...";
        TIMER_START(transform);
        std::for_each(external_maneuver_overrides_list.begin(),
                      external_maneuver_overrides_list.end(),
                      transform_into_internal_types);
        TIMER_STOP(transform);
        log << "ok, after " << TIMER_SEC(transform) << "s";
    }
}

void ExtractionContainers::PrepareRestrictions()
{

    // contain the start/end nodes of each way that is part of an restriction
    std::unordered_map<OSMWayID, FirstAndLastSegmentOfWay> referenced_ways;

    // prepare for extracting source/destination nodes for all restrictions
    {
        util::UnbufferedLog log;
        log << "Collecting start/end information on " << restrictions_list.size()
            << " restrictions...";
        TIMER_START(prepare_restrictions);

        const auto mark_ids = [&](auto const &turn_restriction) {
            FirstAndLastSegmentOfWay dummy_segment{
                MAX_OSM_WAYID, MAX_OSM_NODEID, MAX_OSM_NODEID, MAX_OSM_NODEID, MAX_OSM_NODEID};
            if (turn_restriction.Type() == RestrictionType::WAY_RESTRICTION)
            {
                const auto &way = turn_restriction.AsWayRestriction();
                referenced_ways[way.from] = dummy_segment;
                referenced_ways[way.to] = dummy_segment;
                referenced_ways[way.via] = dummy_segment;
            }
            else
            {
                BOOST_ASSERT(turn_restriction.Type() == RestrictionType::NODE_RESTRICTION);
                const auto &node = turn_restriction.AsNodeRestriction();
                referenced_ways[node.from] = dummy_segment;
                referenced_ways[node.to] = dummy_segment;
            }
        };

        std::for_each(restrictions_list.begin(), restrictions_list.end(), mark_ids);

        // enter invalid IDs into the above maps to indicate that we want to find out about
        // start/end
        // nodes of these ways
        // update the values for all edges already sporting SPECIAL_NODEID
        const auto set_ids = [&](auto const &start_end) {
            auto itr = referenced_ways.find(start_end.way_id);
            if (itr != referenced_ways.end())
                itr->second = start_end;
        };

        std::for_each(way_start_end_id_list.cbegin(), way_start_end_id_list.cend(), set_ids);
        TIMER_STOP(prepare_restrictions);
        log << "ok, after " << TIMER_SEC(prepare_restrictions) << "s";
    }

    auto const to_internal = [&](auto const osm_node) {
        auto internal = mapExternalToInternalNodeID(
            used_node_id_list.begin(), used_node_id_list.end(), osm_node);
        if (internal == SPECIAL_NODEID)
        {
            util::Log(logDEBUG) << "Restriction references invalid node: " << osm_node;
        }
        return internal;
    };

    // Given
    // a -- b - ????????? - c -- d as via segment
    // and either
    // d -- e - ????????? - f -- g or
    // h -- i - ????????? - j -- a
    // (d,e) or (j,a) as entry-segment
    auto const find_node_restriction =
        [&](auto const &segment, auto const &via_segment, auto const via_node) {
            // In case of way-restrictions, via-node will be set to MAX_OSM_NODEID to signal
            // that
            // the node is not present.
            // connected at the front of the segment
            // Turn restrictions are described as a restriction between the two segments closest
            // to
            // the shared via-node on the from and to ways. Graph compression will later
            // renumber
            // the from and to internal node IDs as nodes are plucked out of the node-based
            // graph.
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

    // translate the turn from one segment onto another into a node restriction (the ways can
    // only
    // be connected at a single location)
    auto const get_node_restriction_from_OSM_ids = [&](
        auto const from_id, auto const to_id, const OSMNodeID via_node) {
        auto const from_segment_itr = referenced_ways.find(from_id);
        if (from_segment_itr->second.way_id != from_id)
        {
            util::Log(logDEBUG) << "Restriction references invalid way: " << from_id;
            return NodeRestriction{SPECIAL_NODEID, SPECIAL_NODEID, SPECIAL_NODEID};
        }

        auto const to_segment_itr = referenced_ways.find(to_id);
        if (to_segment_itr->second.way_id != to_id)
        {
            util::Log(logDEBUG) << "Restriction references invalid way: " << to_id;
            return NodeRestriction{SPECIAL_NODEID, SPECIAL_NODEID, SPECIAL_NODEID};
        }
        return find_node_restriction(from_segment_itr->second, to_segment_itr->second, via_node);
    };

    // Transform an OSMRestriction (based on WayIDs) into an OSRM restriction (base on NodeIDs).
    // Returns true on successful transformation, false in case of invalid references.
    // Based on the auto type deduction, this transfor handles both conditional and
    // unconditional
    // turn restrictions.
    const auto transform = [&](const auto &external_type, auto &internal_type) {
        if (external_type.Type() == RestrictionType::WAY_RESTRICTION)
        {
            auto const &external = external_type.AsWayRestriction();
            // check if we were able to resolve all the involved ways
            auto const from_restriction =
                get_node_restriction_from_OSM_ids(external.from, external.via, MAX_OSM_NODEID);
            auto const to_restriction =
                get_node_restriction_from_OSM_ids(external.via, external.to, MAX_OSM_NODEID);

            // failed to translate either of the involved nodes?
            if (!from_restriction.Valid() || !to_restriction.Valid())
                return false;

            // point located at both via and segment is alway on `second`, to FSSF is the order
            // we
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
            auto restriction =
                get_node_restriction_from_OSM_ids(external.from, external.to, external.via);

            if (!restriction.Valid())
            {
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
    const auto transform_into_internal_types =
        [&](const InputConditionalTurnRestriction &external_restriction) {
            // unconditional restriction
            if (external_restriction.condition.empty() &&
                external_restriction.Type() == RestrictionType::NODE_RESTRICTION)
            {
                TurnRestriction restriction;
                restriction.is_only = external_restriction.is_only;
                if (transform(external_restriction, restriction))
                    unconditional_turn_restrictions.push_back(std::move(restriction));
            }
            // conditional turn restriction
            else
            {
                ConditionalTurnRestriction restriction;
                restriction.is_only = external_restriction.is_only;
                restriction.condition = std::move(external_restriction.condition);
                if (transform(external_restriction, restriction))
                {
                    conditional_turn_restrictions.push_back(std::move(restriction));
                }
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
