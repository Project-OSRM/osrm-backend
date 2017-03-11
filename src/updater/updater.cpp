#include "updater/updater.hpp"

#include "updater/csv_source.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/edge_based_graph_factory.hpp"
#include "extractor/io.hpp"
#include "extractor/node_based_edge.hpp"

#include "storage/io.hpp"
#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/graph_loader.hpp"
#include "util/integer_range.hpp"
#include "util/io.hpp"
#include "util/log.hpp"
#include "util/static_graph.hpp"
#include "util/static_rtree.hpp"
#include "util/string_util.hpp"
#include "util/timing_util.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/functional/hash.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <tbb/blocked_range.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/parallel_for_each.h>
#include <tbb/parallel_invoke.h>

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <thread>
#include <tuple>
#include <vector>

namespace osrm
{
namespace updater
{
namespace
{

template <typename T> inline bool is_aligned(const void *pointer)
{
    static_assert(sizeof(T) % alignof(T) == 0, "pointer can not be used as an array pointer");
    return reinterpret_cast<uintptr_t>(pointer) % alignof(T) == 0;
}

// Returns duration in deci-seconds
inline EdgeWeight convertToDuration(double distance_in_meters, double speed_in_kmh)
{
    if (speed_in_kmh <= 0.)
        return MAXIMAL_EDGE_DURATION;

    const double speed_in_ms = speed_in_kmh / 3.6;
    const double duration = distance_in_meters / speed_in_ms;
    return std::max<EdgeWeight>(1, static_cast<EdgeWeight>(std::round(duration * 10.)));
}

inline EdgeWeight convertToWeight(double weight, double weight_multiplier, EdgeWeight duration)
{
    if (std::isfinite(weight))
        return std::round(weight * weight_multiplier);

    return duration == MAXIMAL_EDGE_DURATION ? INVALID_EDGE_WEIGHT
                                             : duration * weight_multiplier / 10.;
}

// Returns updated edge weight
void getNewWeight(const UpdaterConfig &config,
                  const double weight_multiplier,
                  const SpeedSource &value,
                  const double &segment_length,
                  const EdgeWeight current_duration,
                  const OSMNodeID from,
                  const OSMNodeID to,
                  EdgeWeight &new_segment_weight,
                  EdgeWeight &new_segment_duration)
{
    // Update the edge duration as distance/speed
    new_segment_duration = convertToDuration(segment_length, value.speed);

    // Update the edge weight or fallback to the new edge duration
    new_segment_weight = convertToWeight(value.weight, weight_multiplier, new_segment_duration);

    // The check here is enabled by the `--edge-weight-updates-over-factor` flag it logs a warning
    // if the new duration exceeds a heuristic of what a reasonable duration update is
    if (config.log_edge_updates_factor > 0 && current_duration != 0)
    {
        if (current_duration >= (new_segment_duration * config.log_edge_updates_factor))
        {
            auto new_secs = new_segment_duration / 10.;
            auto old_secs = current_duration / 10.;
            auto approx_original_speed = (segment_length / old_secs) * 3.6;
            auto speed_file = config.segment_speed_lookup_paths.at(value.source - 1);
            util::Log(logWARNING) << "[weight updates] Edge weight update from " << old_secs
                                  << "s to " << new_secs << "s  New speed: " << value.speed
                                  << " kph"
                                  << ". Old speed: " << approx_original_speed << " kph"
                                  << ". Segment length: " << segment_length << " m"
                                  << ". Segment: " << from << "," << to << " based on "
                                  << speed_file;
        }
    }
}

#if !defined(NDEBUG)
void checkWeightsConsistency(
    const UpdaterConfig &config,
    const std::vector<osrm::extractor::EdgeBasedEdge> &edge_based_edge_list)
{
    using Reader = osrm::storage::io::FileReader;
    using OriginalEdgeData = osrm::extractor::OriginalEdgeData;

    Reader geometry_file(config.geometry_path, Reader::HasNoFingerprint);
    const auto number_of_indices = geometry_file.ReadElementCount32();
    std::vector<unsigned> geometry_indices(number_of_indices);
    geometry_file.ReadInto(geometry_indices);

    const auto number_of_compressed_geometries = geometry_file.ReadElementCount32();
    BOOST_ASSERT(geometry_indices.back() == number_of_compressed_geometries);
    if (number_of_compressed_geometries == 0)
        return;

    std::vector<NodeID> geometry_node_list(number_of_compressed_geometries);
    std::vector<EdgeWeight> forward_weight_list(number_of_compressed_geometries);
    std::vector<EdgeWeight> reverse_weight_list(number_of_compressed_geometries);
    geometry_file.ReadInto(geometry_node_list.data(), number_of_compressed_geometries);
    geometry_file.ReadInto(forward_weight_list.data(), number_of_compressed_geometries);
    geometry_file.ReadInto(reverse_weight_list.data(), number_of_compressed_geometries);

    Reader edges_input_file(config.osrm_input_path.string() + ".edges", Reader::HasNoFingerprint);
    std::vector<OriginalEdgeData> current_edge_data(edges_input_file.ReadElementCount64());
    edges_input_file.ReadInto(current_edge_data);

    for (auto &edge : edge_based_edge_list)
    {
        BOOST_ASSERT(edge.data.edge_id < current_edge_data.size());
        auto geometry_id = current_edge_data[edge.data.edge_id].via_geometry;
        BOOST_ASSERT(geometry_id.id < geometry_indices.size());

        const auto &weights = geometry_id.forward ? forward_weight_list : reverse_weight_list;
        const int shift = static_cast<int>(geometry_id.forward);
        const auto first = weights.begin() + geometry_indices.at(geometry_id.id) + shift;
        const auto last = weights.begin() + geometry_indices.at(geometry_id.id + 1) - 1 + shift;
        EdgeWeight weight = std::accumulate(first, last, 0);

        BOOST_ASSERT(weight <= edge.data.weight);
    }
}
#endif

auto mmapFile(const std::string &filename, boost::interprocess::mode_t mode)
{
    using boost::interprocess::file_mapping;
    using boost::interprocess::mapped_region;

    try
    {
        const file_mapping mapping{filename.c_str(), mode};
        mapped_region region{mapping, mode};
        region.advise(mapped_region::advice_sequential);
        return region;
    }
    catch (const std::exception &e)
    {
        util::Log(logERROR) << "Error while trying to mmap " + filename + ": " + e.what();
        throw;
    }
}

void updaterSegmentData(const UpdaterConfig &config,
                        const extractor::ProfileProperties &profile_properties,
                        const SegmentLookupTable &segment_speed_lookup)
{
    auto weight_multiplier = profile_properties.GetWeightMultiplier();
    std::vector<extractor::QueryNode> internal_to_external_node_map;
    extractor::SegmentDataContainer segment_data;

    const auto load_internal_to_external_node_map = [&] {
        storage::io::FileReader nodes_file(config.node_based_graph_path,
                                           storage::io::FileReader::HasNoFingerprint);

        nodes_file.DeserializeVector(internal_to_external_node_map);
    };

    const auto load_geometries = [&] { extractor::io::read(config.geometry_path, segment_data); };

    // Folds all our actions into independently concurrently executing lambdas
    tbb::parallel_invoke(load_internal_to_external_node_map, load_geometries);

    // Here, we have to update the compressed geometry weights
    // First, we need the external-to-internal node lookup table

    // Now, we iterate over all the segments stored in the StaticRTree, updating
    // the packed geometry weights in the `.geometries` file (note: we do not
    // update the RTree itself, we just use the leaf nodes to iterate over all segments)
    using LeafNode = util::StaticRTree<extractor::EdgeBasedNode>::LeafNode;

    using boost::interprocess::mapped_region;

    auto region = mmapFile(config.rtree_leaf_path.c_str(), boost::interprocess::read_only);
    region.advise(mapped_region::advice_willneed);
    BOOST_ASSERT(is_aligned<LeafNode>(region.get_address()));

    const auto bytes = region.get_size();
    const auto first = static_cast<const LeafNode *>(region.get_address());
    const auto last = first + (bytes / sizeof(LeafNode));

    // vector to count used speeds for logging
    // size offset by one since index 0 is used for speeds not from external file
    using counters_type = std::vector<std::size_t>;
    std::size_t num_counters = config.segment_speed_lookup_paths.size() + 1;
    tbb::enumerable_thread_specific<counters_type> segment_speeds_counters(
        counters_type(num_counters, 0));
    const constexpr auto LUA_SOURCE = 0;

    tbb::parallel_for_each(first, last, [&](const LeafNode &current_node) {
        auto &counters = segment_speeds_counters.local();
        for (size_t i = 0; i < current_node.object_count; i++)
        {
            const auto &leaf_object = current_node.objects[i];

            const auto geometry_id = leaf_object.packed_geometry_id;
            auto nodes_range = segment_data.GetForwardGeometry(geometry_id);

            const auto segment_offset = leaf_object.fwd_segment_position;
            const std::uint32_t u_index = segment_offset;
            const std::uint32_t v_index = segment_offset + 1;

            BOOST_ASSERT(u_index < nodes_range.size());
            BOOST_ASSERT(v_index < nodes_range.size());

            const extractor::QueryNode &u = internal_to_external_node_map[nodes_range[u_index]];
            const extractor::QueryNode &v = internal_to_external_node_map[nodes_range[v_index]];

            const double segment_length = util::coordinate_calculation::greatCircleDistance(
                util::Coordinate{u.lon, u.lat}, util::Coordinate{v.lon, v.lat});

            auto fwd_source = LUA_SOURCE, rev_source = LUA_SOURCE;
            if (auto value = segment_speed_lookup({u.node_id, v.node_id}))
            {
                EdgeWeight new_segment_weight, new_segment_duration;
                getNewWeight(config,
                             weight_multiplier,
                             *value,
                             segment_length,
                             segment_data.ForwardWeight(geometry_id, segment_offset),
                             u.node_id,
                             v.node_id,
                             new_segment_weight,
                             new_segment_duration);

                segment_data.ForwardWeight(geometry_id, segment_offset) = new_segment_weight;
                segment_data.ForwardDuration(geometry_id, segment_offset) = new_segment_duration;
                segment_data.ForwardDatasource(geometry_id, segment_offset) = value->source;
                fwd_source = value->source;
            }

            if (auto value = segment_speed_lookup({v.node_id, u.node_id}))
            {
                EdgeWeight new_segment_weight, new_segment_duration;
                getNewWeight(config,
                             weight_multiplier,
                             *value,
                             segment_length,
                             segment_data.ReverseWeight(geometry_id, segment_offset),
                             v.node_id,
                             u.node_id,
                             new_segment_weight,
                             new_segment_duration);

                segment_data.ReverseWeight(geometry_id, segment_offset) = new_segment_weight;
                segment_data.ReverseDuration(geometry_id, segment_offset) = new_segment_duration;
                segment_data.ReverseDatasource(geometry_id, segment_offset) = value->source;
                rev_source = value->source;
            }

            // count statistics for logging
            counters[fwd_source] += 1;
            counters[rev_source] += 1;
        }
    }); // parallel_for_each

    counters_type merged_counters(num_counters, 0);
    for (const auto &counters : segment_speeds_counters)
    {
        for (std::size_t i = 0; i < counters.size(); i++)
        {
            merged_counters[i] += counters[i];
        }
    }

    for (std::size_t i = 0; i < merged_counters.size(); i++)
    {
        if (i == LUA_SOURCE)
        {
            util::Log() << "Used " << merged_counters[LUA_SOURCE]
                        << " speeds from LUA profile or input map";
        }
        else
        {
            // segments_speeds_counters has 0 as LUA, segment_speed_filenames not, thus we need
            // to susbstract 1 to avoid off-by-one error
            util::Log() << "Used " << merged_counters[i] << " speeds from "
                        << config.segment_speed_lookup_paths[i - 1];
        }
    }

    // Now save out the updated compressed geometries
    extractor::io::write(config.geometry_path, segment_data);
}

void saveDatasourcesNames(const UpdaterConfig &config)
{
    extractor::Datasources sources;
    DatasourceID source = 0;
    sources.SetSourceName(source++, "lua profile");

    // Only write the filename, without path or extension.
    // This prevents information leakage, and keeps names short
    // for rendering in the debug tiles.
    for (auto const &name : config.segment_speed_lookup_paths)
    {
        sources.SetSourceName(source++, boost::filesystem::path(name).stem().string());
    }

    extractor::io::write(config.datasource_names_path, sources);
}

}

Updater::NumNodesAndEdges Updater::LoadAndUpdateEdgeExpandedGraph() const
{
    std::vector<extractor::EdgeBasedEdge> edge_based_edge_list;
    std::vector<EdgeWeight> node_weights;
    auto max_edge_id = Updater::LoadAndUpdateEdgeExpandedGraph(edge_based_edge_list, node_weights);
    return std::make_tuple(max_edge_id + 1, std::move(edge_based_edge_list));
}

EdgeID
Updater::LoadAndUpdateEdgeExpandedGraph(std::vector<extractor::EdgeBasedEdge> &edge_based_edge_list,
                                        std::vector<EdgeWeight> &node_weights) const
{
    TIMER_START(load_edges);

    // Propagate profile properties to contractor configuration structure
    extractor::ProfileProperties profile_properties;
    storage::io::FileReader profile_properties_file(config.profile_properties_path,
                                                    storage::io::FileReader::HasNoFingerprint);
    profile_properties_file.ReadInto<extractor::ProfileProperties>(&profile_properties, 1);
    auto weight_multiplier = profile_properties.GetWeightMultiplier();

    if (config.segment_speed_lookup_paths.size() + config.turn_penalty_lookup_paths.size() > 255)
        throw util::exception("Limit of 255 segment speed and turn penalty files each reached" +
                              SOURCE_REF);

    util::Log() << "Opening " << config.edge_based_graph_path;

    const auto edge_based_graph_region =
        mmapFile(config.edge_based_graph_path, boost::interprocess::read_only);

    const bool update_edge_weights = !config.segment_speed_lookup_paths.empty();
    const bool update_turn_penalties = !config.turn_penalty_lookup_paths.empty();

    const auto turn_penalties_index_region = [&] {
        if (update_edge_weights || update_turn_penalties)
        {
            return mmapFile(config.turn_penalties_index_path, boost::interprocess::read_only);
        }
        return boost::interprocess::mapped_region();
    }();

    const auto edge_segment_region = [&] {
        if (update_edge_weights || update_turn_penalties)
        {
            return mmapFile(config.edge_segment_lookup_path, boost::interprocess::read_only);
        }
        return boost::interprocess::mapped_region();
    }();

// Set the struct packing to 1 byte word sizes.  This prevents any padding.  We only use
// this struct once, so any alignment penalty is trivial.  If this is *not* done, then
// the struct will be padded out by an extra 4 bytes, and sizeof() will mean we read
// too much data from the original file.
#pragma pack(push, r1, 1)
    struct EdgeBasedGraphHeader
    {
        util::FingerPrint fingerprint;
        std::uint64_t number_of_edges;
        EdgeID max_edge_id;
    };
#pragma pack(pop, r1)

    BOOST_ASSERT(is_aligned<EdgeBasedGraphHeader>(edge_based_graph_region.get_address()));
    const EdgeBasedGraphHeader graph_header =
        *(reinterpret_cast<const EdgeBasedGraphHeader *>(edge_based_graph_region.get_address()));

    const util::FingerPrint expected_fingerprint = util::FingerPrint::GetValid();
    if (!graph_header.fingerprint.IsValid())
    {
        util::Log(logERROR) << config.edge_based_graph_path << " does not have a valid fingerprint";
        throw util::exception("Invalid fingerprint");
    }

    if (!expected_fingerprint.IsDataCompatible(graph_header.fingerprint))
    {
        util::Log(logERROR) << config.edge_based_graph_path
                            << " is not compatible with this version of OSRM.";
        util::Log(logERROR) << "It was prepared with OSRM "
                            << graph_header.fingerprint.GetMajorVersion() << "."
                            << graph_header.fingerprint.GetMinorVersion() << "."
                            << graph_header.fingerprint.GetPatchVersion() << " but you are running "
                            << OSRM_VERSION;
        util::Log(logERROR) << "Data is only compatible between minor releases.";
        throw util::exception("Incompatible file version" + SOURCE_REF);
    }

    edge_based_edge_list.reserve(graph_header.number_of_edges);
    util::Log() << "Reading " << graph_header.number_of_edges << " edges from the edge based graph";

    auto segment_speed_lookup = csv::readSegmentValues(config.segment_speed_lookup_paths);
    auto turn_penalty_lookup = csv::readTurnValues(config.turn_penalty_lookup_paths);

    if (update_edge_weights)
    {
        TIMER_START(segment);
        updaterSegmentData(config, profile_properties, segment_speed_lookup);
        TIMER_STOP(segment);
        util::Log() << "Updating segment data took " << TIMER_MSEC(segment) << "ms.";
    }

    std::vector<TurnPenalty> turn_weight_penalties;
    std::vector<TurnPenalty> turn_duration_penalties;

    const auto maybe_load_turn_weight_penalties = [&] {
        if (!update_edge_weights && !update_turn_penalties)
            return;
        using storage::io::FileReader;
        FileReader file(config.turn_weight_penalties_path, FileReader::HasNoFingerprint);
        file.DeserializeVector(turn_weight_penalties);
    };

    const auto maybe_load_turn_duration_penalties = [&] {
        if (!update_edge_weights && !update_turn_penalties)
            return;
        using storage::io::FileReader;
        FileReader file(config.turn_duration_penalties_path, FileReader::HasNoFingerprint);
        file.DeserializeVector(turn_duration_penalties);
    };

    tbb::parallel_invoke(maybe_load_turn_weight_penalties, maybe_load_turn_duration_penalties);

    if ((update_edge_weights || update_turn_penalties) && turn_duration_penalties.empty())
    { // Copy-on-write for duration penalties as turn weight penalties
        turn_duration_penalties = turn_weight_penalties;
    }

    // Mapped file pointer for turn indices
    const extractor::lookup::TurnIndexBlock *turn_index_blocks =
        reinterpret_cast<const extractor::lookup::TurnIndexBlock *>(
            turn_penalties_index_region.get_address());
    BOOST_ASSERT(is_aligned<extractor::lookup::TurnIndexBlock>(turn_index_blocks));

    // Mapped file pointers for edge-based graph edges
    auto edge_based_edge_ptr = reinterpret_cast<const extractor::EdgeBasedEdge *>(
        reinterpret_cast<char *>(edge_based_graph_region.get_address()) +
        sizeof(EdgeBasedGraphHeader));
    BOOST_ASSERT(is_aligned<extractor::EdgeBasedEdge>(edge_based_edge_ptr));

    auto edge_segment_byte_ptr = reinterpret_cast<const char *>(edge_segment_region.get_address());

    bool fallback_to_duration = true;
    for (std::uint64_t edge_index = 0; edge_index < graph_header.number_of_edges; ++edge_index)
    {
        // Make a copy of the data from the memory map
        extractor::EdgeBasedEdge inbuffer = edge_based_edge_ptr[edge_index];

        if (update_edge_weights || update_turn_penalties)
        {
            using extractor::lookup::SegmentHeaderBlock;
            using extractor::lookup::SegmentBlock;

            auto header = reinterpret_cast<const SegmentHeaderBlock *>(edge_segment_byte_ptr);
            BOOST_ASSERT(is_aligned<SegmentHeaderBlock>(header));
            edge_segment_byte_ptr += sizeof(SegmentHeaderBlock);

            auto first = reinterpret_cast<const SegmentBlock *>(edge_segment_byte_ptr);
            BOOST_ASSERT(is_aligned<SegmentBlock>(first));
            edge_segment_byte_ptr += sizeof(SegmentBlock) * (header->num_osm_nodes - 1);
            auto last = reinterpret_cast<const SegmentBlock *>(edge_segment_byte_ptr);

            // Find a segment with zero speed and simultaneously compute the new edge weight
            EdgeWeight new_weight = 0;
            EdgeWeight new_duration = 0;
            auto osm_node_id = header->previous_osm_node_id;
            bool skip_edge =
                std::find_if(first, last, [&](const auto &segment) {
                    auto segment_weight = segment.segment_weight;
                    auto segment_duration = segment.segment_duration;
                    if (auto value = segment_speed_lookup({osm_node_id, segment.this_osm_node_id}))
                    {
                        // If we hit a 0-speed edge, then it's effectively not traversible.
                        // We don't want to include it in the edge_based_edge_list.
                        if (value->speed == 0)
                            return true;

                        segment_duration = convertToDuration(segment.segment_length, value->speed);

                        segment_weight =
                            convertToWeight(value->weight, weight_multiplier, segment_duration);
                    }

                    // Update the edge weight and the next OSM node ID
                    osm_node_id = segment.this_osm_node_id;
                    new_weight += segment_weight;
                    new_duration += segment_duration;
                    return false;
                }) != last;

            // Update the node-weight cache. This is the weight of the edge-based-node only,
            // it doesn't include the turn. We may visit the same node multiple times, but
            // we should always assign the same value here.
            if (node_weights.size() > 0)
                node_weights[inbuffer.source] = new_weight;

            // We found a zero-speed edge, so we'll skip this whole edge-based-edge which
            // effectively removes it from the routing network.
            if (skip_edge)
                continue;

            // Get the turn penalty and update to the new value if required
            const auto &turn_index = turn_index_blocks[edge_index];
            auto turn_weight_penalty = turn_weight_penalties[edge_index];
            auto turn_duration_penalty = turn_duration_penalties[edge_index];
            if (auto value = turn_penalty_lookup(turn_index))
            {
                turn_duration_penalty =
                    boost::numeric_cast<TurnPenalty>(std::round(value->duration * 10.));
                turn_weight_penalty = boost::numeric_cast<TurnPenalty>(
                    std::round(std::isfinite(value->weight)
                                   ? value->weight * weight_multiplier
                                   : turn_duration_penalty * weight_multiplier / 10.));

                const auto weight_min_value = static_cast<EdgeWeight>(header->num_osm_nodes);
                if (turn_weight_penalty + new_weight < weight_min_value)
                {
                    util::Log(logWARNING) << "turn penalty " << turn_weight_penalty << " for turn "
                                          << turn_index.from_id << ", " << turn_index.via_id << ", "
                                          << turn_index.to_id
                                          << " is too negative: clamping turn weight to "
                                          << weight_min_value;

                    turn_weight_penalty = weight_min_value - new_weight;
                }

                turn_duration_penalties[edge_index] = turn_duration_penalty;
                turn_weight_penalties[edge_index] = turn_weight_penalty;

                // Is fallback of duration to weight values allowed
                fallback_to_duration &= (turn_duration_penalty == turn_weight_penalty);
            }

            // Update edge weight
            inbuffer.data.weight = new_weight + turn_weight_penalty;
            inbuffer.data.duration = new_duration + turn_duration_penalty;
        }

        edge_based_edge_list.emplace_back(std::move(inbuffer));
    }

    if (update_turn_penalties)
    {
        if (fallback_to_duration)
        { // Turn duration penalties are identical to turn weight penalties
            // Save empty data vector, so turn weight penalties will be used by data facade.
            turn_duration_penalties.clear();
        }

        const auto save_penalties = [](const auto &filename, const auto &data) -> void {
            storage::io::FileWriter file(filename, storage::io::FileWriter::HasNoFingerprint);
            file.SerializeVector(data);
        };

        tbb::parallel_invoke(
            [&] { save_penalties(config.turn_weight_penalties_path, turn_weight_penalties); },
            [&] { save_penalties(config.turn_duration_penalties_path, turn_duration_penalties); });
    }

#if !defined(NDEBUG)
    if (config.turn_penalty_lookup_paths.empty())
    { // don't check weights consistency with turn updates that can break assertion
        // condition with turn weight penalties negative updates
        checkWeightsConsistency(config, edge_based_edge_list);
    }
#endif

    saveDatasourcesNames(config);

    TIMER_STOP(load_edges);
    util::Log() << "Done reading edges in " << TIMER_MSEC(load_edges) << "ms.";
    return graph_header.max_edge_id;
}
}
}
