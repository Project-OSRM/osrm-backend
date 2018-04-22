#include "updater/updater.hpp"
#include "updater/csv_source.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/edge_based_graph_factory.hpp"
#include "extractor/files.hpp"
#include "extractor/node_based_edge.hpp"
#include "extractor/packed_osm_ids.hpp"
#include "extractor/restriction.hpp"
#include "extractor/serialization.hpp"

#include "guidance/files.hpp"

#include "storage/io.hpp"

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/for_each_pair.hpp"
#include "util/integer_range.hpp"
#include "util/log.hpp"
#include "util/mmap_tar.hpp"
#include "util/opening_hours.hpp"
#include "util/static_graph.hpp"
#include "util/static_rtree.hpp"
#include "util/string_util.hpp"
#include "util/timezones.hpp"
#include "util/timing_util.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <tbb/blocked_range.h>
#include <tbb/concurrent_vector.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_invoke.h>
#include <tbb/parallel_sort.h>

#include <algorithm>
#include <atomic>
#include <bitset>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <thread>
#include <tuple>
#include <vector>

namespace std
{
template <typename T1, typename T2, typename T3> struct hash<std::tuple<T1, T2, T3>>
{
    size_t operator()(const std::tuple<T1, T2, T3> &t) const
    {
        return hash_val(std::get<0>(t), std::get<1>(t), std::get<2>(t));
    }
};

template <typename T1, typename T2> struct hash<std::tuple<T1, T2>>
{
    size_t operator()(const std::tuple<T1, T2> &t) const
    {
        return hash_val(std::get<0>(t), std::get<1>(t));
    }
};
}

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
inline SegmentDuration convertToDuration(double speed_in_kmh, double distance_in_meters)
{
    if (speed_in_kmh <= 0.)
        return INVALID_SEGMENT_DURATION;

    const auto speed_in_ms = speed_in_kmh / 3.6;
    const auto duration = distance_in_meters / speed_in_ms;
    auto segment_duration = std::max<SegmentDuration>(
        1, boost::numeric_cast<SegmentDuration>(std::round(duration * 10.)));
    if (segment_duration >= INVALID_SEGMENT_DURATION)
    {
        util::Log(logWARNING) << "Clamping segment duration " << segment_duration << " to "
                              << MAX_SEGMENT_DURATION;
        segment_duration = MAX_SEGMENT_DURATION;
    }
    return segment_duration;
}

#if !defined(NDEBUG)
void checkWeightsConsistency(
    const UpdaterConfig &config,
    const std::vector<osrm::extractor::EdgeBasedEdge> &edge_based_edge_list)
{
    extractor::SegmentDataContainer segment_data;
    extractor::files::readSegmentData(config.GetPath(".osrm.geometry"), segment_data);

    extractor::EdgeBasedNodeDataContainer node_data;
    extractor::files::readNodeData(config.GetPath(".osrm.ebg_nodes"), node_data);

    for (auto &edge : edge_based_edge_list)
    {
        const auto node_id = edge.source;
        const auto geometry_id = node_data.GetGeometryID(node_id);

        if (geometry_id.forward)
        {
            auto range = segment_data.GetForwardWeights(geometry_id.id);
            EdgeWeight weight = std::accumulate(range.begin(), range.end(), EdgeWeight{0});
            if (weight > edge.data.weight)
            {
                util::Log(logWARNING) << geometry_id.id << " vs " << edge.data.turn_id << ":"
                                      << weight << " > " << edge.data.weight;
            }
        }
        else
        {
            auto range = segment_data.GetReverseWeights(geometry_id.id);
            EdgeWeight weight = std::accumulate(range.begin(), range.end(), EdgeWeight{0});
            if (weight > edge.data.weight)
            {
                util::Log(logWARNING) << geometry_id.id << " vs " << edge.data.turn_id << ":"
                                      << weight << " > " << edge.data.weight;
            }
        }
    }
}
#endif

static const constexpr std::size_t LUA_SOURCE = 0;

tbb::concurrent_vector<GeometryID>
updateSegmentData(const UpdaterConfig &config,
                  const extractor::ProfileProperties &profile_properties,
                  const SegmentLookupTable &segment_speed_lookup,
                  extractor::SegmentDataContainer &segment_data,
                  std::vector<util::Coordinate> &coordinates,
                  extractor::PackedOSMIDs &osm_node_ids)
{
    // vector to count used speeds for logging
    // size offset by one since index 0 is used for speeds not from external file
    using counters_type = std::vector<std::size_t>;
    std::size_t num_counters = config.segment_speed_lookup_paths.size() + 1;
    tbb::enumerable_thread_specific<counters_type> segment_speeds_counters(
        counters_type(num_counters, 0));

    // closure to convert SpeedSource value to weight and count fallbacks to durations
    std::atomic<std::uint32_t> fallbacks_to_duration{0};
    auto convertToWeight = [&profile_properties, &fallbacks_to_duration](
        const SegmentWeight &existing_weight, const SpeedSource &value, double distance_in_meters) {

        double rate = std::numeric_limits<double>::quiet_NaN();

        // if value.rate is not set, we fall back to duration
        //    this happens when there is no 4th column in the input CSV
        // if value.rate is set but NaN, we keep the existing weight
        //    this happens when there is an empty 4th column in the input CSV
        // otherwise, we use the value as the new rate
        if (!value.rate)
        {
            ++fallbacks_to_duration;
            rate = value.speed / 3.6;
        }
        else
        {
            rate = *value.rate;
            if (!std::isfinite(rate))
            {
                return existing_weight;
            }
        }

        if (rate <= 0.)
            return INVALID_SEGMENT_WEIGHT;

        const auto weight_multiplier = profile_properties.GetWeightMultiplier();
        const auto weight = distance_in_meters / rate;
        auto segment_weight = std::max<SegmentWeight>(
            1, boost::numeric_cast<SegmentWeight>(std::round(weight * weight_multiplier)));
        if (segment_weight >= INVALID_SEGMENT_WEIGHT)
        {
            util::Log(logWARNING) << "Clamping segment weight " << segment_weight << " to "
                                  << MAX_SEGMENT_WEIGHT;
            segment_weight = MAX_SEGMENT_WEIGHT;
        }
        return segment_weight;
    };

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
    tbb::parallel_for(range, [&](const auto &range) {
        auto &counters = segment_speeds_counters.local();
        std::vector<double> segment_lengths;
        for (auto geometry_id = range.begin(); geometry_id < range.end(); geometry_id++)
        {
            auto nodes_range = segment_data.GetForwardGeometry(geometry_id);

            segment_lengths.clear();
            segment_lengths.reserve(nodes_range.size() + 1);
            util::for_each_pair(nodes_range, [&](const auto &u, const auto &v) {
                segment_lengths.push_back(util::coordinate_calculation::greatCircleDistance(
                    coordinates[u], coordinates[v]));
            });

            auto fwd_weights_range = segment_data.GetForwardWeights(geometry_id);
            auto fwd_durations_range = segment_data.GetForwardDurations(geometry_id);
            auto fwd_datasources_range = segment_data.GetForwardDatasources(geometry_id);
            bool fwd_was_updated = false;
            for (const auto segment_offset : util::irange<std::size_t>(0, fwd_weights_range.size()))
            {
                auto u = osm_node_ids[nodes_range[segment_offset]];
                auto v = osm_node_ids[nodes_range[segment_offset + 1]];

                // Self-loops are artifical segments (e.g. traffic light nodes), do not
                // waste time updating them with traffic data
                if (u == v)
                    continue;

                if (auto value = segment_speed_lookup({u, v}))
                {
                    auto segment_length = segment_lengths[segment_offset];
                    auto new_duration = convertToDuration(value->speed, segment_length);
                    auto new_weight =
                        convertToWeight(fwd_weights_range[segment_offset], *value, segment_length);
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

            for (const auto segment_offset : util::irange<std::size_t>(0, rev_weights_range.size()))
            {
                auto u = osm_node_ids[nodes_range[segment_offset]];
                auto v = osm_node_ids[nodes_range[segment_offset + 1]];

                // Self-loops are artifical segments (e.g. traffic light nodes), do not
                // waste time updating them with traffic data
                if (u == v)
                    continue;

                if (auto value = segment_speed_lookup({v, u}))
                {
                    auto segment_length = segment_lengths[segment_offset];
                    auto new_duration = convertToDuration(value->speed, segment_length);
                    auto new_weight =
                        convertToWeight(rev_weights_range[segment_offset], *value, segment_length);
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

    if (!profile_properties.fallback_to_duration && fallbacks_to_duration > 0)
    {
        util::Log(logWARNING) << "Speed values were used to update " << fallbacks_to_duration
                              << " segments for '" << profile_properties.GetWeightName()
                              << "' profile";
    }

    if (config.log_edge_updates_factor > 0)
    {
        BOOST_ASSERT(segment_data_backup);

        for (const auto geometry_id :
             util::irange<DirectionalGeometryID>(0, segment_data.GetNumberOfGeometries()))
        {
            auto nodes_range = segment_data.GetForwardGeometry(geometry_id);

            auto new_fwd_durations_range = segment_data.GetForwardDurations(geometry_id);
            auto new_fwd_datasources_range = segment_data.GetForwardDatasources(geometry_id);
            auto new_rev_durations_range =
                boost::adaptors::reverse(segment_data.GetReverseDurations(geometry_id));
            auto new_rev_datasources_range = segment_data.GetReverseDatasources(geometry_id);
            auto old_fwd_durations_range = segment_data_backup->GetForwardDurations(geometry_id);
            auto old_rev_durations_range =
                boost::adaptors::reverse(segment_data_backup->GetReverseDurations(geometry_id));

            for (const auto segment_offset :
                 util::irange<std::size_t>(0, new_fwd_durations_range.size()))
            {
                if (new_fwd_datasources_range[segment_offset] == LUA_SOURCE)
                    continue;

                if (old_fwd_durations_range[segment_offset] >=
                    (new_fwd_durations_range[segment_offset] * config.log_edge_updates_factor))
                {
                    auto from = osm_node_ids[nodes_range[segment_offset]];
                    auto to = osm_node_ids[nodes_range[segment_offset + 1]];
                    util::Log(logWARNING)
                        << "[weight updates] Edge weight update from "
                        << old_fwd_durations_range[segment_offset] / 10. << "s to "
                        << new_fwd_durations_range[segment_offset] / 10. << "s Segment: " << from
                        << "," << to << " based on "
                        << config.segment_speed_lookup_paths
                               [new_fwd_datasources_range[segment_offset] - 1];
                }
            }

            for (const auto segment_offset :
                 util::irange<std::size_t>(0, new_rev_durations_range.size()))
            {
                if (new_rev_datasources_range[segment_offset] == LUA_SOURCE)
                    continue;

                if (old_rev_durations_range[segment_offset] >=
                    (new_rev_durations_range[segment_offset] * config.log_edge_updates_factor))
                {
                    auto from = osm_node_ids[nodes_range[segment_offset + 1]];
                    auto to = osm_node_ids[nodes_range[segment_offset]];
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

    return updated_segments;
}

void saveDatasourcesNames(const UpdaterConfig &config)
{
    extractor::Datasources sources;
    DatasourceID source = 0;
    sources.SetSourceName(source, "lua profile");
    source++;

    // Only write the filename, without path or extension.
    // This prevents information leakage, and keeps names short
    // for rendering in the debug tiles.
    for (auto const &name : config.segment_speed_lookup_paths)
    {
        sources.SetSourceName(source, boost::filesystem::path(name).stem().string());
        source++;
    }

    extractor::files::writeDatasources(config.GetPath(".osrm.datasource_names"), sources);
}

std::vector<std::uint64_t>
updateTurnPenalties(const UpdaterConfig &config,
                    const extractor::ProfileProperties &profile_properties,
                    const TurnLookupTable &turn_penalty_lookup,
                    std::vector<TurnPenalty> &turn_weight_penalties,
                    std::vector<TurnPenalty> &turn_duration_penalties,
                    extractor::PackedOSMIDs osm_node_ids)
{
    const auto weight_multiplier = profile_properties.GetWeightMultiplier();

    // Mapped file pointer for turn indices
    boost::iostreams::mapped_file_source turn_index_region;
    const extractor::lookup::TurnIndexBlock *turn_index_blocks;
    {
        auto map =
            util::mmapTarFile(config.GetPath(".osrm.turn_penalties_index"), turn_index_region);
        turn_index_blocks = reinterpret_cast<const extractor::lookup::TurnIndexBlock *>(
            map["/extractor/turn_index"].first);
    }

    // Get the turn penalty and update to the new value if required
    std::vector<std::uint64_t> updated_turns;
    for (std::uint64_t edge_index = 0; edge_index < turn_weight_penalties.size(); ++edge_index)
    {
        // edges are stored by internal OSRM ids, these need to be mapped back to OSM ids
        const extractor::lookup::TurnIndexBlock internal_turn = turn_index_blocks[edge_index];
        const Turn osm_turn{osm_node_ids[internal_turn.from_id],
                            osm_node_ids[internal_turn.via_id],
                            osm_node_ids[internal_turn.to_id]};
        // original turn weight/duration values
        auto turn_weight_penalty = turn_weight_penalties[edge_index];
        auto turn_duration_penalty = turn_duration_penalties[edge_index];

        if (auto value = turn_penalty_lookup(osm_turn))
        {
            turn_duration_penalty =
                boost::numeric_cast<TurnPenalty>(std::round(value->duration * 10.));
            turn_weight_penalty = boost::numeric_cast<TurnPenalty>(std::round(
                std::isfinite(value->weight) ? value->weight * weight_multiplier
                                             : turn_duration_penalty * weight_multiplier / 10.));

            turn_duration_penalties[edge_index] = turn_duration_penalty;
            turn_weight_penalties[edge_index] = turn_weight_penalty;
            updated_turns.push_back(edge_index);
        }

        if (turn_weight_penalty < 0)
        {
            util::Log(logWARNING) << "Negative turn penalty at " << osm_turn.from << ", "
                                  << osm_turn.via << ", " << osm_turn.to << ": turn penalty "
                                  << turn_weight_penalty;
        }
    }

    return updated_turns;
}

bool IsRestrictionValid(const Timezoner &tz_handler, const extractor::ConditionalTurnPenalty &turn)
{
    BOOST_ASSERT(!turn.conditions.empty());

    // we utilize the via node (first on ways) to represent the turn restriction
    auto const via = turn.location;

    const auto lon = static_cast<double>(toFloating(via.lon));
    const auto lat = static_cast<double>(toFloating(via.lat));
    const auto &conditions = turn.conditions;

    // Get local time of the restriction
    const auto &local_time = tz_handler(point_t{lon, lat});
    if (!local_time)
        return false;

    // TODO: check restriction type [:<transportation mode>][:<direction>]
    // http://wiki.openstreetmap.org/wiki/Conditional_restrictions#Tagging

    // TODO: parsing will fail for combined conditions, e.g. Sa-Su AND weight>7
    // http://wiki.openstreetmap.org/wiki/Conditional_restrictions#Combined_conditions:_AND

    return osrm::util::CheckOpeningHours(conditions, *local_time);
}

std::vector<std::uint64_t>
updateConditionalTurns(std::vector<TurnPenalty> &turn_weight_penalties,
                       const std::vector<extractor::ConditionalTurnPenalty> &conditional_turns,
                       Timezoner time_zone_handler)
{
    std::vector<std::uint64_t> updated_turns;
    if (conditional_turns.size() == 0)
        return updated_turns;

    for (const auto &penalty : conditional_turns)
    {
        if (IsRestrictionValid(time_zone_handler, penalty))
        {
            turn_weight_penalties[penalty.turn_offset] = INVALID_TURN_PENALTY;
            updated_turns.push_back(penalty.turn_offset);
        }
    }
    return updated_turns;
}
}

EdgeID
Updater::LoadAndUpdateEdgeExpandedGraph(std::vector<extractor::EdgeBasedEdge> &edge_based_edge_list,
                                        std::vector<EdgeWeight> &node_weights,
                                        std::uint32_t &connectivity_checksum) const
{
    std::vector<EdgeDuration> node_durations(node_weights.size());
    return LoadAndUpdateEdgeExpandedGraph(
        edge_based_edge_list, node_weights, node_durations, connectivity_checksum);
}

EdgeID
Updater::LoadAndUpdateEdgeExpandedGraph(std::vector<extractor::EdgeBasedEdge> &edge_based_edge_list,
                                        std::vector<EdgeWeight> &node_weights,
                                        std::vector<EdgeDuration> &node_durations,
                                        std::uint32_t &connectivity_checksum) const
{
    TIMER_START(load_edges);

    EdgeID number_of_edge_based_nodes = 0;
    std::vector<util::Coordinate> coordinates;
    extractor::PackedOSMIDs osm_node_ids;

    extractor::files::readEdgeBasedNodeWeightsDurations(
        config.GetPath(".osrm.enw"), node_weights, node_durations);

    extractor::files::readEdgeBasedGraph(config.GetPath(".osrm.ebg"),
                                         number_of_edge_based_nodes,
                                         edge_based_edge_list,
                                         connectivity_checksum);
    extractor::files::readNodes(config.GetPath(".osrm.nbg_nodes"), coordinates, osm_node_ids);

    const bool update_conditional_turns =
        !config.GetPath(".osrm.restrictions").empty() && config.valid_now;
    const bool update_edge_weights = !config.segment_speed_lookup_paths.empty();
    const bool update_turn_penalties = !config.turn_penalty_lookup_paths.empty();

    if (!update_edge_weights && !update_turn_penalties && !update_conditional_turns)
    {
        saveDatasourcesNames(config);
        return number_of_edge_based_nodes;
    }

    if (config.segment_speed_lookup_paths.size() + config.turn_penalty_lookup_paths.size() > 255)
        throw util::exception("Limit of 255 segment speed and turn penalty files each reached" +
                              SOURCE_REF);

    extractor::EdgeBasedNodeDataContainer node_data;
    extractor::SegmentDataContainer segment_data;
    extractor::ProfileProperties profile_properties;
    std::vector<TurnPenalty> turn_weight_penalties;
    std::vector<TurnPenalty> turn_duration_penalties;
    if (update_edge_weights || update_turn_penalties || update_conditional_turns)
    {
        tbb::parallel_invoke(
            [&] {
                extractor::files::readSegmentData(config.GetPath(".osrm.geometry"), segment_data);
            },
            [&] { extractor::files::readNodeData(config.GetPath(".osrm.ebg_nodes"), node_data); },

            [&] {
                extractor::files::readTurnWeightPenalty(
                    config.GetPath(".osrm.turn_weight_penalties"), turn_weight_penalties);
            },
            [&] {
                extractor::files::readTurnDurationPenalty(
                    config.GetPath(".osrm.turn_duration_penalties"), turn_duration_penalties);
            },
            [&] {
                extractor::files::readProfileProperties(config.GetPath(".osrm.properties"),
                                                        profile_properties);
            });
    }

    std::vector<extractor::ConditionalTurnPenalty> conditional_turns;
    if (update_conditional_turns)
    {
        extractor::files::readConditionalRestrictions(config.GetPath(".osrm.restrictions"),
                                                      conditional_turns);
    }

    tbb::concurrent_vector<GeometryID> updated_segments;
    if (update_edge_weights)
    {
        auto segment_speed_lookup = csv::readSegmentValues(config.segment_speed_lookup_paths);

        TIMER_START(segment);
        updated_segments = updateSegmentData(config,
                                             profile_properties,
                                             segment_speed_lookup,
                                             segment_data,
                                             coordinates,
                                             osm_node_ids);
        // Now save out the updated compressed geometries
        extractor::files::writeSegmentData(config.GetPath(".osrm.geometry"), segment_data);
        TIMER_STOP(segment);
        util::Log() << "Updating segment data took " << TIMER_MSEC(segment) << "ms.";
    }

    auto turn_penalty_lookup = csv::readTurnValues(config.turn_penalty_lookup_paths);
    if (update_turn_penalties)
    {
        auto updated_turn_penalties = updateTurnPenalties(config,
                                                          profile_properties,
                                                          turn_penalty_lookup,
                                                          turn_weight_penalties,
                                                          turn_duration_penalties,
                                                          osm_node_ids);
        const auto offset = updated_segments.size();
        updated_segments.resize(offset + updated_turn_penalties.size());
        // we need to re-compute all edges that have updated turn penalties.
        // this marks it for re-computation
        std::transform(updated_turn_penalties.begin(),
                       updated_turn_penalties.end(),
                       updated_segments.begin() + offset,
                       [&node_data, &edge_based_edge_list](const std::uint64_t turn_id) {
                           const auto node_id = edge_based_edge_list[turn_id].source;
                           return node_data.GetGeometryID(node_id);
                       });
    }

    if (update_conditional_turns)
    {
        // initialize instance of class that handles time zone resolution
        if (config.valid_now <= 0)
        {
            util::Log(logERROR) << "Given UTC time is invalid: " << config.valid_now;
            throw;
        }
        const Timezoner time_zone_handler = Timezoner(config.tz_file_path, config.valid_now);

        auto updated_turn_penalties =
            updateConditionalTurns(turn_weight_penalties, conditional_turns, time_zone_handler);
        const auto offset = updated_segments.size();
        updated_segments.resize(offset + updated_turn_penalties.size());
        // we need to re-compute all edges that have updated turn penalties.
        // this marks it for re-computation
        std::transform(updated_turn_penalties.begin(),
                       updated_turn_penalties.end(),
                       updated_segments.begin() + offset,
                       [&node_data, &edge_based_edge_list](const std::uint64_t turn_id) {
                           const auto node_id = edge_based_edge_list[turn_id].source;
                           return node_data.GetGeometryID(node_id);
                       });
    }

    tbb::parallel_sort(updated_segments.begin(),
                       updated_segments.end(),
                       [](const GeometryID lhs, const GeometryID rhs) {
                           return std::tie(lhs.id, lhs.forward) < std::tie(rhs.id, rhs.forward);
                       });

    using WeightAndDuration = std::tuple<EdgeWeight, EdgeWeight>;
    const auto compute_new_weight_and_duration =
        [&](const GeometryID geometry_id) -> WeightAndDuration {
        EdgeWeight new_weight = 0;
        EdgeWeight new_duration = 0;
        if (geometry_id.forward)
        {
            const auto weights = segment_data.GetForwardWeights(geometry_id.id);
            for (const auto weight : weights)
            {
                if (weight == INVALID_SEGMENT_WEIGHT)
                {
                    new_weight = INVALID_EDGE_WEIGHT;
                    break;
                }
                new_weight += weight;
            }
            const auto durations = segment_data.GetForwardDurations(geometry_id.id);
            new_duration = std::accumulate(durations.begin(), durations.end(), EdgeWeight{0});
        }
        else
        {
            const auto weights = segment_data.GetReverseWeights(geometry_id.id);
            for (const auto weight : weights)
            {
                if (weight == INVALID_SEGMENT_WEIGHT)
                {
                    new_weight = INVALID_EDGE_WEIGHT;
                    break;
                }
                new_weight += weight;
            }
            const auto durations = segment_data.GetReverseDurations(geometry_id.id);
            new_duration = std::accumulate(durations.begin(), durations.end(), EdgeWeight{0});
        }
        return std::make_tuple(new_weight, new_duration);
    };

    std::vector<WeightAndDuration> accumulated_segment_data(updated_segments.size());
    tbb::parallel_for(tbb::blocked_range<std::size_t>(0, updated_segments.size()),
                      [&](const auto &range) {
                          for (auto index = range.begin(); index < range.end(); ++index)
                          {
                              accumulated_segment_data[index] =
                                  compute_new_weight_and_duration(updated_segments[index]);
                          }
                      });

    const auto update_edge = [&](extractor::EdgeBasedEdge &edge) {
        const auto node_id = edge.source;
        const auto geometry_id = node_data.GetGeometryID(node_id);
        auto updated_iter = std::lower_bound(updated_segments.begin(),
                                             updated_segments.end(),
                                             geometry_id,
                                             [](const GeometryID lhs, const GeometryID rhs) {
                                                 return std::tie(lhs.id, lhs.forward) <
                                                        std::tie(rhs.id, rhs.forward);
                                             });
        if (updated_iter != updated_segments.end() && updated_iter->id == geometry_id.id &&
            updated_iter->forward == geometry_id.forward)
        {
            // Find a segment with zero speed and simultaneously compute the new edge
            // weight
            EdgeWeight new_weight;
            EdgeWeight new_duration;
            std::tie(new_weight, new_duration) =
                accumulated_segment_data[updated_iter - updated_segments.begin()];

            // Update the node-weight cache. This is the weight of the edge-based-node
            // only, it doesn't include the turn. We may visit the same node multiple times,
            // but we should always assign the same value here.
            BOOST_ASSERT(edge.source < node_weights.size());
            node_weights[edge.source] =
                node_weights[edge.source] & 0x80000000 ? new_weight | 0x80000000 : new_weight;
            node_durations[edge.source] = new_duration;

            // We found a zero-speed edge, so we'll skip this whole edge-based-edge
            // which
            // effectively removes it from the routing network.
            if (new_weight == INVALID_EDGE_WEIGHT)
            {
                edge.data.weight = INVALID_EDGE_WEIGHT;
                return;
            }

            // Get the turn penalty and update to the new value if required
            auto turn_weight_penalty = turn_weight_penalties[edge.data.turn_id];
            auto turn_duration_penalty = turn_duration_penalties[edge.data.turn_id];
            const auto num_nodes = segment_data.GetForwardGeometry(geometry_id.id).size();
            const auto weight_min_value = static_cast<EdgeWeight>(num_nodes);
            if (turn_weight_penalty + new_weight < weight_min_value)
            {
                if (turn_weight_penalty < 0)
                {
                    util::Log(logWARNING) << "turn penalty " << turn_weight_penalty
                                          << " is too negative: clamping turn weight to "
                                          << weight_min_value;
                    turn_weight_penalty = weight_min_value - new_weight;
                    turn_weight_penalties[edge.data.turn_id] = turn_weight_penalty;
                }
                else
                {
                    new_weight = weight_min_value;
                }
            }

            // Update edge weight
            edge.data.weight = new_weight + turn_weight_penalty;
            edge.data.duration = new_duration + turn_duration_penalty;
        }
    };

    if (updated_segments.size() > 0)
    {
        tbb::parallel_for(tbb::blocked_range<std::size_t>(0, edge_based_edge_list.size()),
                          [&](const auto &range) {
                              for (auto index = range.begin(); index < range.end(); ++index)
                              {
                                  update_edge(edge_based_edge_list[index]);
                              }
                          });
    }

    if (update_turn_penalties || update_conditional_turns)
    {
        tbb::parallel_invoke(
            [&] {
                extractor::files::writeTurnWeightPenalty(
                    config.GetPath(".osrm.turn_weight_penalties"), turn_weight_penalties);
            },
            [&] {
                extractor::files::writeTurnDurationPenalty(
                    config.GetPath(".osrm.turn_duration_penalties"), turn_duration_penalties);
            });
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
    return number_of_edge_based_nodes;
}
}
}
