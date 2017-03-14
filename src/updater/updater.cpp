#include "updater/updater.hpp"

#include "updater/csv_source.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/edge_based_graph_factory.hpp"
#include "extractor/io.hpp"
#include "extractor/node_based_edge.hpp"

#include "storage/io.hpp"
#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/for_each_pair.hpp"
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
inline EdgeWeight convertToDuration(double speed_in_kmh, double distance_in_meters)
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

#if !defined(NDEBUG)
void checkWeightsConsistency(
    const UpdaterConfig &config,
    const std::vector<osrm::extractor::EdgeBasedEdge> &edge_based_edge_list)
{
    using Reader = osrm::storage::io::FileReader;
    using OriginalEdgeData = osrm::extractor::OriginalEdgeData;

    extractor::SegmentDataContainer segment_data;
    extractor::io::read(config.geometry_path, segment_data);

    Reader edges_input_file(config.osrm_input_path.string() + ".edges", Reader::HasNoFingerprint);
    std::vector<OriginalEdgeData> current_edge_data(edges_input_file.ReadElementCount64());
    edges_input_file.ReadInto(current_edge_data);

    for (auto &edge : edge_based_edge_list)
    {
        BOOST_ASSERT(edge.data.edge_id < current_edge_data.size());
        auto geometry_id = current_edge_data[edge.data.edge_id].via_geometry;

        if (geometry_id.forward)
        {
            auto range = segment_data.GetForwardWeights(geometry_id.id);
            EdgeWeight weight = std::accumulate(range.begin(), range.end(), 0);
            if (weight > edge.data.weight)
            {
                util::Log(logWARNING) << geometry_id.id << " vs " << edge.data.edge_id << ":"
                                      << weight << " > " << edge.data.weight;
            }
        }
        else
        {
            auto range = segment_data.GetReverseWeights(geometry_id.id);
            EdgeWeight weight = std::accumulate(range.begin(), range.end(), 0);
            if (weight > edge.data.weight)
            {
                util::Log(logWARNING) << geometry_id.id << " vs " << edge.data.edge_id << ":"
                                      << weight << " > " << edge.data.weight;
            }
        }
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

using UpdatedListAndSegmentData =
    std::tuple<tbb::concurrent_vector<GeometryID>, extractor::SegmentDataContainer>;
UpdatedListAndSegmentData updaterSegmentData(const UpdaterConfig &config,
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

    // vector to count used speeds for logging
    // size offset by one since index 0 is used for speeds not from external file
    using counters_type = std::vector<std::size_t>;
    std::size_t num_counters = config.segment_speed_lookup_paths.size() + 1;
    tbb::enumerable_thread_specific<counters_type> segment_speeds_counters(
        counters_type(num_counters, 0));
    const constexpr auto LUA_SOURCE = 0;

    // The check here is enabled by the `--edge-weight-updates-over-factor` flag it logs a
    // warning if the new duration exceeds a heuristic of what a reasonable duration update is
    std::unique_ptr<extractor::SegmentDataContainer> segment_data_backup;
    if (config.log_edge_updates_factor > 0)
    {
        // copy the old data so we can compare later
        segment_data_backup = std::make_unique<extractor::SegmentDataContainer>(segment_data);
    }

    tbb::concurrent_vector<GeometryID> updated_segments;

    using DirectionalGeometryID = extractor::SegmentDataContainer::DirectionalGeometryID;
    auto range = tbb::blocked_range<DirectionalGeometryID>(0, segment_data.GetNumberOfGeometries());
    tbb::parallel_for(range, [&, LUA_SOURCE](const auto &range) {
        auto &counters = segment_speeds_counters.local();
        std::vector<double> segment_lengths;
        for (auto geometry_id = range.begin(); geometry_id < range.end(); geometry_id++)
        {
            auto nodes_range = segment_data.GetForwardGeometry(geometry_id);

            segment_lengths.clear();
            segment_lengths.reserve(nodes_range.size() + 1);
            util::for_each_pair(nodes_range, [&](const auto &u, const auto &v) {
                segment_lengths.push_back(util::coordinate_calculation::greatCircleDistance(
                    util::Coordinate{internal_to_external_node_map[u]},
                    util::Coordinate{internal_to_external_node_map[v]}));
            });

            auto fwd_weights_range = segment_data.GetForwardWeights(geometry_id);
            auto fwd_durations_range = segment_data.GetForwardDurations(geometry_id);
            auto fwd_datasources_range = segment_data.GetForwardDatasources(geometry_id);
            bool fwd_was_updated = false;
            for (auto segment_offset = 0UL; segment_offset < fwd_weights_range.size();
                 ++segment_offset)
            {
                auto u = internal_to_external_node_map[nodes_range[segment_offset]].node_id;
                auto v = internal_to_external_node_map[nodes_range[segment_offset + 1]].node_id;
                if (auto value = segment_speed_lookup({u, v}))
                {
                    auto new_duration =
                        convertToDuration(value->speed, segment_lengths[segment_offset]);
                    auto new_weight =
                        convertToWeight(value->weight, weight_multiplier, new_duration);
                    fwd_was_updated = true;

                    fwd_weights_range[segment_offset] = new_weight;
                    fwd_durations_range[segment_offset] = new_duration;
                    fwd_datasources_range[segment_offset] = value->source;
                    counters[value->source] += 1;
                }
                else
                {
                    counters[LUA_SOURCE] += 1;
                }
            }
            if (fwd_was_updated)
                updated_segments.push_back(GeometryID{geometry_id, true});

            // In this case we want it oriented from in forward directions
            auto rev_weights_range =
                boost::adaptors::reverse(segment_data.GetReverseWeights(geometry_id));
            auto rev_durations_range =
                boost::adaptors::reverse(segment_data.GetReverseDurations(geometry_id));
            auto rev_datasources_range =
                boost::adaptors::reverse(segment_data.GetReverseDatasources(geometry_id));
            bool rev_was_updated = false;

            for (auto segment_offset = 0UL; segment_offset < fwd_weights_range.size();
                 ++segment_offset)
            {
                auto u = internal_to_external_node_map[nodes_range[segment_offset]].node_id;
                auto v = internal_to_external_node_map[nodes_range[segment_offset + 1]].node_id;
                if (auto value = segment_speed_lookup({v, u}))
                {
                    auto new_duration =
                        convertToDuration(value->speed, segment_lengths[segment_offset]);
                    auto new_weight =
                        convertToWeight(value->weight, weight_multiplier, new_duration);
                    rev_was_updated = true;

                    rev_weights_range[segment_offset] = new_weight;
                    rev_durations_range[segment_offset] = new_duration;
                    rev_datasources_range[segment_offset] = value->source;
                    counters[value->source] += 1;
                }
                else
                {
                    counters[LUA_SOURCE] += 1;
                }
            }
            if (rev_was_updated)
                updated_segments.push_back(GeometryID{geometry_id, false});
        }
    }); // parallel_for

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

    if (config.log_edge_updates_factor > 0)
    {
        BOOST_ASSERT(segment_data_backup);

        for (DirectionalGeometryID geometry_id = 0;
             geometry_id < segment_data.GetNumberOfGeometries();
             geometry_id++)
        {
            auto nodes_range = segment_data.GetForwardGeometry(geometry_id);

            auto new_fwd_durations_range = segment_data.GetForwardDurations(geometry_id);
            auto new_fwd_datasources_range = segment_data.GetForwardDatasources(geometry_id);
            auto new_rev_durations_range =
                boost::adaptors::reverse(segment_data.GetReverseDurations(geometry_id));
            auto new_rev_datasources_range = segment_data.GetForwardDatasources(geometry_id);
            auto old_fwd_durations_range = segment_data_backup->GetForwardDurations(geometry_id);
            auto old_rev_durations_range =
                boost::adaptors::reverse(segment_data_backup->GetReverseDurations(geometry_id));

            for (auto segment_offset = 0UL; segment_offset < new_fwd_durations_range.size();
                 ++segment_offset)
            {
                if (new_fwd_datasources_range[segment_offset] == LUA_SOURCE)
                    continue;

                if (old_fwd_durations_range[segment_offset] >=
                    (new_fwd_durations_range[segment_offset] * config.log_edge_updates_factor))
                {
                    auto from = internal_to_external_node_map[nodes_range[segment_offset]].node_id;
                    auto to =
                        internal_to_external_node_map[nodes_range[segment_offset + 1]].node_id;
                    util::Log(logWARNING)
                        << "[weight updates] Edge weight update from "
                        << old_fwd_durations_range[segment_offset] / 10. << "s to "
                        << new_fwd_durations_range[segment_offset] / 10. << "s Segment: " << from
                        << "," << to << " based on "
                        << config.segment_speed_lookup_paths
                               [new_fwd_datasources_range[segment_offset] - 1];
                }
            }

            for (auto segment_offset = 0UL; segment_offset < new_rev_durations_range.size();
                 ++segment_offset)
            {
                if (new_rev_datasources_range[segment_offset] == LUA_SOURCE)
                    continue;

                if (old_rev_durations_range[segment_offset] >=
                    (new_rev_durations_range[segment_offset] * config.log_edge_updates_factor))
                {
                    auto from =
                        internal_to_external_node_map[nodes_range[segment_offset + 1]].node_id;
                    auto to = internal_to_external_node_map[nodes_range[segment_offset]].node_id;
                    util::Log(logWARNING)
                        << "[weight updates] Edge weight update from "
                        << old_rev_durations_range[segment_offset] / 10. << "s to "
                        << new_rev_durations_range[segment_offset] / 10. << "s Segment: " << from
                        << "," << to << " based on "
                        << config.segment_speed_lookup_paths
                               [new_rev_datasources_range[segment_offset] - 1];
                }
            }
        }
    }

    tbb::parallel_sort(updated_segments.begin(),
                       updated_segments.end(),
                       [](const GeometryID lhs, const GeometryID rhs) {
                           return std::tie(lhs.id, lhs.forward) < std::tie(rhs.id, rhs.forward);
                       });

    return std::make_tuple(std::move(updated_segments), std::move(segment_data));
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

using TurnWeightAndDuration = std::tuple<std::vector<TurnPenalty>, std::vector<TurnPenalty>, bool>;
TurnWeightAndDuration
loadAndUpdateTurnPenalties(const UpdaterConfig &config,
                           const extractor::ProfileProperties &profile_properties,
                           const TurnLookupTable &turn_penalty_lookup)
{
    const auto weight_multiplier = profile_properties.GetWeightMultiplier();
    const auto turn_penalties_index_region =
        mmapFile(config.turn_penalties_index_path, boost::interprocess::read_only);

    std::vector<TurnPenalty> turn_weight_penalties;
    std::vector<TurnPenalty> turn_duration_penalties;

    const auto load_turn_weight_penalties = [&] {
        using storage::io::FileReader;
        FileReader file(config.turn_weight_penalties_path, FileReader::HasNoFingerprint);
        file.DeserializeVector(turn_weight_penalties);
    };

    const auto load_turn_duration_penalties = [&] {
        using storage::io::FileReader;
        FileReader file(config.turn_duration_penalties_path, FileReader::HasNoFingerprint);
        file.DeserializeVector(turn_duration_penalties);
    };

    tbb::parallel_invoke(load_turn_weight_penalties, load_turn_duration_penalties);

    bool fallback_to_duration = false;
    if (turn_duration_penalties.empty())
    { // Copy-on-write for duration penalties as turn weight penalties
        turn_duration_penalties = turn_weight_penalties;
        fallback_to_duration = true;
    }

    // Mapped file pointer for turn indices
    const extractor::lookup::TurnIndexBlock *turn_index_blocks =
        reinterpret_cast<const extractor::lookup::TurnIndexBlock *>(
            turn_penalties_index_region.get_address());
    BOOST_ASSERT(is_aligned<extractor::lookup::TurnIndexBlock>(turn_index_blocks));

    for (std::uint64_t edge_index = 0; edge_index < turn_weight_penalties.size(); ++edge_index)
    {
        // Get the turn penalty and update to the new value if required
        const auto &turn_index = turn_index_blocks[edge_index];
        auto turn_weight_penalty = turn_weight_penalties[edge_index];
        auto turn_duration_penalty = turn_duration_penalties[edge_index];
        if (auto value = turn_penalty_lookup(turn_index))
        {
            turn_duration_penalty =
                boost::numeric_cast<TurnPenalty>(std::round(value->duration * 10.));
            turn_weight_penalty = boost::numeric_cast<TurnPenalty>(std::round(
                std::isfinite(value->weight) ? value->weight * weight_multiplier
                                             : turn_duration_penalty * weight_multiplier / 10.));

            turn_duration_penalties[edge_index] = turn_duration_penalty;
            turn_weight_penalties[edge_index] = turn_weight_penalty;

            // Is fallback of duration to weight values allowed
            fallback_to_duration &= (turn_duration_penalty == turn_weight_penalty);
        }

        if (turn_weight_penalty < 0)
        {
            util::Log(logWARNING) << "Negative turn penalty at " << turn_index.from_id << ", "
                                  << turn_index.via_id << ", " << turn_index.to_id
                                  << ": turn penalty " << turn_weight_penalty;
        }
    }

    return std::make_tuple(std::move(turn_weight_penalties), std::move(turn_duration_penalties), fallback_to_duration);
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

    if (config.segment_speed_lookup_paths.size() + config.turn_penalty_lookup_paths.size() > 255)
        throw util::exception("Limit of 255 segment speed and turn penalty files each reached" +
                              SOURCE_REF);

    util::Log() << "Opening " << config.edge_based_graph_path;

    const auto edge_based_graph_region =
        mmapFile(config.edge_based_graph_path, boost::interprocess::read_only);

    const bool update_edge_weights = !config.segment_speed_lookup_paths.empty();
    const bool update_turn_penalties = !config.turn_penalty_lookup_paths.empty();

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

    storage::io::FileReader edges_input_file(config.osrm_input_path.string() + ".edges",
                                             storage::io::FileReader::HasNoFingerprint);
    std::vector<extractor::OriginalEdgeData> edge_data(edges_input_file.ReadElementCount64());
    edges_input_file.ReadInto(edge_data);

    tbb::concurrent_vector<GeometryID> updated_segments;
    extractor::SegmentDataContainer segment_data;
    if (update_edge_weights)
    {
        auto segment_speed_lookup = csv::readSegmentValues(config.segment_speed_lookup_paths);

        TIMER_START(segment);
        std::tie(updated_segments, segment_data) =
            updaterSegmentData(config, profile_properties, segment_speed_lookup);
        // Now save out the updated compressed geometries
        extractor::io::write(config.geometry_path, segment_data);
        TIMER_STOP(segment);
        util::Log() << "Updating segment data took " << TIMER_MSEC(segment) << "ms.";
    }
    else if (update_turn_penalties)
    {
        extractor::io::read(config.geometry_path, segment_data);
    }

    std::vector<TurnPenalty> turn_weight_penalties;
    std::vector<TurnPenalty> turn_duration_penalties;
    bool fallback_to_duration = true;
    if (update_turn_penalties)
    {
        auto turn_penalty_lookup = csv::readTurnValues(config.turn_penalty_lookup_paths);
        std::tie(turn_weight_penalties, turn_duration_penalties, fallback_to_duration) =
            loadAndUpdateTurnPenalties(config, profile_properties, turn_penalty_lookup);
    }
    else if (update_edge_weights)
    {
        const auto load_turn_weight_penalties = [&] {
            using storage::io::FileReader;
            FileReader file(config.turn_weight_penalties_path, FileReader::HasNoFingerprint);
            file.DeserializeVector(turn_weight_penalties);
        };

        const auto load_turn_duration_penalties = [&] {
            using storage::io::FileReader;
            FileReader file(config.turn_duration_penalties_path, FileReader::HasNoFingerprint);
            file.DeserializeVector(turn_duration_penalties);
        };

        tbb::parallel_invoke(load_turn_weight_penalties, load_turn_duration_penalties);
        if (turn_duration_penalties.empty())
        { // Copy-on-write for duration penalties as turn weight penalties
            turn_duration_penalties = turn_weight_penalties;
            fallback_to_duration = true;
        }

    }

    // Mapped file pointers for edge-based graph edges
    auto edge_based_edge_ptr = reinterpret_cast<const extractor::EdgeBasedEdge *>(
        reinterpret_cast<char *>(edge_based_graph_region.get_address()) +
        sizeof(EdgeBasedGraphHeader));
    BOOST_ASSERT(is_aligned<extractor::EdgeBasedEdge>(edge_based_edge_ptr));

    for (std::uint64_t edge_index = 0; edge_index < graph_header.number_of_edges; ++edge_index)
    {
        // Make a copy of the data from the memory map
        extractor::EdgeBasedEdge inbuffer = edge_based_edge_ptr[edge_index];
        bool needs_update = update_turn_penalties;

        if (update_edge_weights)
        {
            const auto geometry_id = edge_data[edge_index].via_geometry;
            auto updated_iter = std::lower_bound(updated_segments.begin(),
                                                 updated_segments.end(),
                                                 geometry_id,
                                                 [](const GeometryID lhs, const GeometryID rhs) {
                                                     return std::tie(lhs.id, lhs.forward) <
                                                            std::tie(rhs.id, rhs.forward);
                                                 });
            if (updated_iter == updated_segments.end() || updated_iter->id != geometry_id.id ||
                updated_iter->forward != geometry_id.forward)
            {
                needs_update = update_turn_penalties;
            }
            else
            {
                needs_update = true;
            }
        }

        if (needs_update)
        {
            // Find a segment with zero speed and simultaneously compute the new edge weight
            EdgeWeight new_weight = 0;
            EdgeWeight new_duration = 0;
            bool skip_edge = false;

            const auto geometry_id = edge_data[edge_index].via_geometry;
            if (geometry_id.forward)
            {
                const auto weights = segment_data.GetForwardWeights(geometry_id.id);
                for (const auto weight : weights)
                {
                    if (weight == INVALID_EDGE_WEIGHT)
                    {
                        skip_edge = true;
                        break;
                    }
                    new_weight += weight;
                }
                const auto durations = segment_data.GetForwardDurations(geometry_id.id);
                new_duration = std::accumulate(durations.begin(), durations.end(), 0);
            }
            else
            {
                const auto weights = segment_data.GetReverseWeights(geometry_id.id);
                for (const auto weight : weights)
                {
                    if (weight == INVALID_EDGE_WEIGHT)
                    {
                        skip_edge = true;
                        break;
                    }
                    new_weight += weight;
                }
                const auto durations = segment_data.GetReverseDurations(geometry_id.id);
                new_duration = std::accumulate(durations.begin(), durations.end(), 0);
            }

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
            auto turn_weight_penalty = turn_weight_penalties[edge_index];
            auto turn_duration_penalty = turn_duration_penalties[edge_index];
            const auto num_nodes = segment_data.GetForwardGeometry(geometry_id.id).size();
            const auto weight_min_value = static_cast<EdgeWeight>(num_nodes);
            if (turn_weight_penalty + new_weight < weight_min_value)
            {
                util::Log(logWARNING) << "turn penalty " << turn_weight_penalty
                                      << " is too negative: clamping turn weight to "
                                      << weight_min_value;

                turn_weight_penalty = weight_min_value - new_weight;
            }

            turn_weight_penalties[edge_index] = turn_weight_penalty;

            // Is fallback of duration to weight values allowed
            fallback_to_duration &= (turn_duration_penalty == turn_weight_penalty);

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
