#include "contractor/contractor.hpp"
#include "contractor/crc32_processor.hpp"
#include "contractor/graph_contractor.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/node_based_edge.hpp"

#include "util/exception.hpp"
#include "util/graph_loader.hpp"
#include "util/integer_range.hpp"
#include "util/io.hpp"
#include "util/simple_logger.hpp"
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
#include <boost/spirit/include/qi.hpp>

#include <tbb/blocked_range.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>
#include <tbb/parallel_invoke.h>
#include <tbb/parallel_sort.h>
#include <tbb/spin_mutex.h>

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

template <> struct hash<std::pair<OSMNodeID, OSMNodeID>>
{
    std::size_t operator()(const std::pair<OSMNodeID, OSMNodeID> &k) const noexcept
    {
        return static_cast<uint64_t>(k.first) ^ (static_cast<uint64_t>(k.second) << 12);
    }
};

template <> struct hash<std::tuple<OSMNodeID, OSMNodeID, OSMNodeID>>
{
    std::size_t operator()(const std::tuple<OSMNodeID, OSMNodeID, OSMNodeID> &k) const noexcept
    {
        std::size_t seed = 0;
        boost::hash_combine(seed, static_cast<uint64_t>(std::get<0>(k)));
        boost::hash_combine(seed, static_cast<uint64_t>(std::get<1>(k)));
        boost::hash_combine(seed, static_cast<uint64_t>(std::get<2>(k)));
        return seed;
    }
};
}

namespace osrm
{
namespace contractor
{

int Contractor::Run()
{
#ifdef WIN32
#pragma message("Memory consumption on Windows can be higher due to different bit packing")
#else
    static_assert(sizeof(extractor::NodeBasedEdge) == 24,
                  "changing extractor::NodeBasedEdge type has influence on memory consumption!");
    static_assert(sizeof(extractor::EdgeBasedEdge) == 16,
                  "changing EdgeBasedEdge type has influence on memory consumption!");
#endif

    if (config.core_factor > 1.0 || config.core_factor < 0)
    {
        throw util::exception("Core factor must be between 0.0 to 1.0 (inclusive)");
    }

    TIMER_START(preparing);

    util::SimpleLogger().Write() << "Loading edge-expanded graph representation";

    util::DeallocatingVector<extractor::EdgeBasedEdge> edge_based_edge_list;

    std::size_t max_edge_id = LoadEdgeExpandedGraph(
        config.edge_based_graph_path, edge_based_edge_list, config.edge_segment_lookup_path,
        config.edge_penalty_path, config.segment_speed_lookup_paths,
        config.turn_penalty_lookup_paths, config.node_based_graph_path, config.geometry_path,
        config.datasource_names_path, config.datasource_indexes_path, config.rtree_leaf_path);

    // Contracting the edge-expanded graph

    TIMER_START(contraction);
    std::vector<bool> is_core_node;
    std::vector<float> node_levels;
    if (config.use_cached_priority)
    {
        ReadNodeLevels(node_levels);
    }

    util::SimpleLogger().Write() << "Reading node weights.";
    std::vector<EdgeWeight> node_weights;
    std::string node_file_name = config.osrm_input_path.string() + ".enw";
    if (util::deserializeVector(node_file_name, node_weights))
    {
        util::SimpleLogger().Write() << "Done reading node weights.";
    }
    else
    {
        throw util::exception("Failed reading node weights.");
    }

    util::DeallocatingVector<QueryEdge> contracted_edge_list;
    ContractGraph(max_edge_id, edge_based_edge_list, contracted_edge_list, std::move(node_weights),
                  is_core_node, node_levels);
    TIMER_STOP(contraction);

    util::SimpleLogger().Write() << "Contraction took " << TIMER_SEC(contraction) << " sec";

    std::size_t number_of_used_edges = WriteContractedGraph(max_edge_id, contracted_edge_list);
    WriteCoreNodeMarker(std::move(is_core_node));
    if (!config.use_cached_priority)
    {
        WriteNodeLevels(std::move(node_levels));
    }

    TIMER_STOP(preparing);

    util::SimpleLogger().Write() << "Preprocessing : " << TIMER_SEC(preparing) << " seconds";
    util::SimpleLogger().Write() << "Contraction: " << ((max_edge_id + 1) / TIMER_SEC(contraction))
                                 << " nodes/sec and "
                                 << number_of_used_edges / TIMER_SEC(contraction) << " edges/sec";

    util::SimpleLogger().Write() << "finished preprocessing";

    return 0;
}

// Utilities for LoadEdgeExpandedGraph to restore my sanity
namespace
{

struct Segment final
{
    OSMNodeID from, to;
};

struct SpeedSource final
{
    unsigned speed;
    std::uint8_t source;
};

struct SegmentSpeedSource final
{
    Segment segment;
    SpeedSource speed_source;
};

using SegmentSpeedSourceFlatMap = std::vector<SegmentSpeedSource>;

// Binary Search over a flattened key,val Segment storage
SegmentSpeedSourceFlatMap::iterator find(SegmentSpeedSourceFlatMap &map, const Segment &key)
{
    const auto last = end(map);

    const auto by_segment = [](const SegmentSpeedSource &lhs, const SegmentSpeedSource &rhs) {
        return std::tie(lhs.segment.from, lhs.segment.to) >
               std::tie(rhs.segment.from, rhs.segment.to);
    };

    auto it = std::lower_bound(begin(map), last, SegmentSpeedSource{key, {0, 0}}, by_segment);

    if (it != last && (std::tie(it->segment.from, it->segment.to) == std::tie(key.from, key.to)))
        return it;

    return last;
}

// Convenience aliases. TODO: make actual types at some point in time.
// TODO: turn penalties need flat map + binary search optimization, take a look at segment speeds

using Turn = std::tuple<OSMNodeID, OSMNodeID, OSMNodeID>;
using TurnHasher = std::hash<Turn>;
using PenaltySource = std::pair<double, std::uint8_t>;
using TurnPenaltySourceMap = tbb::concurrent_unordered_map<Turn, PenaltySource, TurnHasher>;

// Functions for parsing files and creating lookup tables

SegmentSpeedSourceFlatMap
parse_segment_lookup_from_csv_files(const std::vector<std::string> &segment_speed_filenames)
{
    // TODO: shares code with turn penalty lookup parse function

    using Mutex = tbb::spin_mutex;

    // Loaded and parsed in parallel, at the end we combine results in a flattened map-ish view
    SegmentSpeedSourceFlatMap flatten;
    Mutex flatten_mutex;

    const auto parse_segment_speed_file = [&](const std::size_t idx) {
        const auto file_id = idx + 1; // starts at one, zero means we assigned the weight
        const auto filename = segment_speed_filenames[idx];

        std::ifstream segment_speed_file{filename, std::ios::binary};
        if (!segment_speed_file)
            throw util::exception{"Unable to open segment speed file " + filename};

        SegmentSpeedSourceFlatMap local;

        std::uint64_t from_node_id{};
        std::uint64_t to_node_id{};
        unsigned speed{};

        for (std::string line; std::getline(segment_speed_file, line);)
        {
            using namespace boost::spirit::qi;

            auto it = begin(line);
            const auto last = end(line);

            // The ulong_long -> uint64_t will likely break on 32bit platforms
            const auto ok = parse(it, last,                                          //
                                  (ulong_long >> ',' >> ulong_long >> ',' >> uint_), //
                                  from_node_id, to_node_id, speed);                  //

            if (!ok || it != last)
                throw util::exception{"Segment speed file " + filename + " malformed"};

            SegmentSpeedSource val{
                {static_cast<OSMNodeID>(from_node_id), static_cast<OSMNodeID>(to_node_id)},
                {speed, static_cast<std::uint8_t>(file_id)}};

            local.push_back(std::move(val));
        }

        util::SimpleLogger().Write() << "Loaded speed file " << filename << " with " << local.size()
                                     << " speeds";

        {
            Mutex::scoped_lock _{flatten_mutex};

            flatten.insert(end(flatten), std::make_move_iterator(begin(local)),
                           std::make_move_iterator(end(local)));
        }
    };

    tbb::parallel_for(std::size_t{0}, segment_speed_filenames.size(), parse_segment_speed_file);

    // With flattened map-ish view of all the files, sort and unique them on from,to,source
    // The greater '>' is used here since we want to give files later on higher precedence
    const auto sort_by = [](const SegmentSpeedSource &lhs, const SegmentSpeedSource &rhs) {
        return std::tie(lhs.segment.from, lhs.segment.to, lhs.speed_source.source) >
               std::tie(rhs.segment.from, rhs.segment.to, rhs.speed_source.source);
    };

    std::stable_sort(begin(flatten), end(flatten), sort_by);

    // Unique only on from,to to take the source precedence into account and remove duplicates
    const auto unique_by = [](const SegmentSpeedSource &lhs, const SegmentSpeedSource &rhs) {
        return std::tie(lhs.segment.from, lhs.segment.to) ==
               std::tie(rhs.segment.from, rhs.segment.to);
    };

    const auto it = std::unique(begin(flatten), end(flatten), unique_by);

    flatten.erase(it, end(flatten));

    util::SimpleLogger().Write() << "In total loaded " << segment_speed_filenames.size()
                                 << " speed file(s) with a total of " << flatten.size()
                                 << " unique values";

    return flatten;
}

TurnPenaltySourceMap
parse_turn_penalty_lookup_from_csv_files(const std::vector<std::string> &turn_penalty_filenames)
{
    // TODO: shares code with turn penalty lookup parse function
    TurnPenaltySourceMap map;

    const auto parse_turn_penalty_file = [&](const std::size_t idx) {
        const auto file_id = idx + 1; // starts at one, zero means we assigned the weight
        const auto filename = turn_penalty_filenames[idx];

        std::ifstream turn_penalty_file{filename, std::ios::binary};
        if (!turn_penalty_file)
            throw util::exception{"Unable to open turn penalty file " + filename};

        std::uint64_t from_node_id{};
        std::uint64_t via_node_id{};
        std::uint64_t to_node_id{};
        double penalty{};

        for (std::string line; std::getline(turn_penalty_file, line);)
        {
            using namespace boost::spirit::qi;

            auto it = begin(line);
            const auto last = end(line);

            // The ulong_long -> uint64_t will likely break on 32bit platforms
            const auto ok =
                parse(it, last,                                                                 //
                      (ulong_long >> ',' >> ulong_long >> ',' >> ulong_long >> ',' >> double_), //
                      from_node_id, via_node_id, to_node_id, penalty);                          //

            if (!ok || it != last)
                throw util::exception{"Turn penalty file " + filename + " malformed"};

            map[std::make_tuple(OSMNodeID(from_node_id), OSMNodeID(via_node_id),
                                OSMNodeID(to_node_id))] = std::make_pair(penalty, file_id);
        }
    };

    tbb::parallel_for(std::size_t{0}, turn_penalty_filenames.size(), parse_turn_penalty_file);

    return map;
}
} // anon ns

std::size_t Contractor::LoadEdgeExpandedGraph(
    std::string const &edge_based_graph_filename,
    util::DeallocatingVector<extractor::EdgeBasedEdge> &edge_based_edge_list,
    const std::string &edge_segment_lookup_filename,
    const std::string &edge_penalty_filename,
    const std::vector<std::string> &segment_speed_filenames,
    const std::vector<std::string> &turn_penalty_filenames,
    const std::string &nodes_filename,
    const std::string &geometry_filename,
    const std::string &datasource_names_filename,
    const std::string &datasource_indexes_filename,
    const std::string &rtree_leaf_filename)
{
    if (segment_speed_filenames.size() > 255 || turn_penalty_filenames.size() > 255)
        throw util::exception("Limit of 255 segment speed and turn penalty files each reached");

    util::SimpleLogger().Write() << "Opening " << edge_based_graph_filename;
    boost::filesystem::ifstream input_stream(edge_based_graph_filename, std::ios::binary);
    if (!input_stream)
        throw util::exception("Could not load edge based graph file");

    const bool update_edge_weights = !segment_speed_filenames.empty();
    const bool update_turn_penalties = !turn_penalty_filenames.empty();

    boost::filesystem::ifstream edge_segment_input_stream;
    boost::filesystem::ifstream edge_fixed_penalties_input_stream;

    if (update_edge_weights || update_turn_penalties)
    {
        edge_segment_input_stream.open(edge_segment_lookup_filename, std::ios::binary);
        edge_fixed_penalties_input_stream.open(edge_penalty_filename, std::ios::binary);
        if (!edge_segment_input_stream || !edge_fixed_penalties_input_stream)
        {
            throw util::exception("Could not load .edge_segment_lookup or .edge_penalties, did you "
                                  "run osrm-extract with '--generate-edge-lookup'?");
        }
    }

    const util::FingerPrint fingerprint_valid = util::FingerPrint::GetValid();
    util::FingerPrint fingerprint_loaded;
    input_stream.read((char *)&fingerprint_loaded, sizeof(util::FingerPrint));
    fingerprint_loaded.TestContractor(fingerprint_valid);

    // TODO std::size_t can vary on systems. Our files are not transferable, but we might want to
    // consider using a fixed size type for I/O
    std::size_t number_of_edges = 0;
    std::size_t max_edge_id = SPECIAL_EDGEID;
    input_stream.read((char *)&number_of_edges, sizeof(std::size_t));
    input_stream.read((char *)&max_edge_id, sizeof(std::size_t));

    edge_based_edge_list.resize(number_of_edges);
    util::SimpleLogger().Write() << "Reading " << number_of_edges
                                 << " edges from the edge based graph";

    SegmentSpeedSourceFlatMap segment_speed_lookup;
    TurnPenaltySourceMap turn_penalty_lookup;

    const auto parse_segment_speeds = [&] {
        if (update_edge_weights)
            segment_speed_lookup = parse_segment_lookup_from_csv_files(segment_speed_filenames);
    };

    const auto parse_turn_penalties = [&] {
        if (update_turn_penalties)
            turn_penalty_lookup = parse_turn_penalty_lookup_from_csv_files(turn_penalty_filenames);
    };

    // If we update the edge weights, this file will hold the datasource information for each
    // segment; the other files will also be conditionally filled concurrently if we make an update
    std::vector<uint8_t> m_geometry_datasource;

    std::vector<extractor::QueryNode> internal_to_external_node_map;
    std::vector<unsigned> m_geometry_indices;
    std::vector<extractor::CompressedEdgeContainer::CompressedEdge> m_geometry_list;

    const auto maybe_load_internal_to_external_node_map = [&] {
        if (!(update_edge_weights || update_turn_penalties))
            return;

        boost::filesystem::ifstream nodes_input_stream(nodes_filename, std::ios::binary);

        if (!nodes_input_stream)
        {
            throw util::exception("Failed to open " + nodes_filename);
        }

        unsigned number_of_nodes = 0;
        nodes_input_stream.read((char *)&number_of_nodes, sizeof(unsigned));
        internal_to_external_node_map.resize(number_of_nodes);

        // Load all the query nodes into a vector
        nodes_input_stream.read(reinterpret_cast<char *>(&(internal_to_external_node_map[0])),
                                number_of_nodes * sizeof(extractor::QueryNode));
    };

    const auto maybe_load_geometries = [&] {
        if (!(update_edge_weights || update_turn_penalties))
            return;

        std::ifstream geometry_stream(geometry_filename, std::ios::binary);
        if (!geometry_stream)
        {
            throw util::exception("Failed to open " + geometry_filename);
        }
        unsigned number_of_indices = 0;
        unsigned number_of_compressed_geometries = 0;

        geometry_stream.read((char *)&number_of_indices, sizeof(unsigned));

        m_geometry_indices.resize(number_of_indices);
        if (number_of_indices > 0)
        {
            geometry_stream.read((char *)&(m_geometry_indices[0]),
                                 number_of_indices * sizeof(unsigned));
        }

        geometry_stream.read((char *)&number_of_compressed_geometries, sizeof(unsigned));

        BOOST_ASSERT(m_geometry_indices.back() == number_of_compressed_geometries);
        m_geometry_list.resize(number_of_compressed_geometries);

        if (number_of_compressed_geometries > 0)
        {
            geometry_stream.read((char *)&(m_geometry_list[0]),
                                 number_of_compressed_geometries *
                                     sizeof(extractor::CompressedEdgeContainer::CompressedEdge));
        }
    };

    // Folds all our actions into independently concurrently executing lambdas
    tbb::parallel_invoke(parse_segment_speeds, parse_turn_penalties, //
                         maybe_load_internal_to_external_node_map, maybe_load_geometries);

    if (update_edge_weights || update_turn_penalties)
    {
        // Here, we have to update the compressed geometry weights
        // First, we need the external-to-internal node lookup table

        // This is a list of the "data source id" for every segment in the compressed
        // geometry container.  We assume that everything so far has come from the
        // profile (data source 0).  Here, we replace the 0's with the index of the
        // CSV file that supplied the value that gets used for that segment, then
        // we write out this list so that it can be returned by the debugging
        // vector tiles later on.
        m_geometry_datasource.resize(m_geometry_list.size(), 0);

        // Now, we iterate over all the segments stored in the StaticRTree, updating
        // the packed geometry weights in the `.geometries` file (note: we do not
        // update the RTree itself, we just use the leaf nodes to iterate over all segments)
        using LeafNode = util::StaticRTree<extractor::EdgeBasedNode>::LeafNode;

        using boost::interprocess::file_mapping;
        using boost::interprocess::mapped_region;
        using boost::interprocess::read_only;

        const file_mapping mapping{rtree_leaf_filename.c_str(), read_only};
        mapped_region region{mapping, read_only};
        region.advise(mapped_region::advice_willneed);

        const auto bytes = region.get_size();
        const auto first = static_cast<const LeafNode *>(region.get_address());
        const auto last = first + (bytes / sizeof(LeafNode));

        // vector to count used speeds for logging
        // size offset by one since index 0 is used for speeds not from external file
        std::vector<std::atomic<std::uint64_t>> segment_speeds_counters(
            segment_speed_filenames.size() + 1);
        for (auto &each : segment_speeds_counters)
            each.store(0);
        const constexpr auto LUA_SOURCE = 0;

        tbb::parallel_for_each(first, last, [&](const LeafNode &current_node) {
            for (size_t i = 0; i < current_node.object_count; i++)
            {
                const auto &leaf_object = current_node.objects[i];
                extractor::QueryNode *u;
                extractor::QueryNode *v;

                if (leaf_object.forward_packed_geometry_id != SPECIAL_EDGEID)
                {
                    const unsigned forward_begin =
                        m_geometry_indices.at(leaf_object.forward_packed_geometry_id);

                    if (leaf_object.fwd_segment_position == 0)
                    {
                        u = &(internal_to_external_node_map[leaf_object.u]);
                        v = &(
                            internal_to_external_node_map[m_geometry_list[forward_begin].node_id]);
                    }
                    else
                    {
                        u = &(internal_to_external_node_map
                                  [m_geometry_list[forward_begin +
                                                   leaf_object.fwd_segment_position - 1]
                                       .node_id]);
                        v = &(internal_to_external_node_map
                                  [m_geometry_list[forward_begin + leaf_object.fwd_segment_position]
                                       .node_id]);
                    }
                    const double segment_length = util::coordinate_calculation::greatCircleDistance(
                        util::Coordinate{u->lon, u->lat}, util::Coordinate{v->lon, v->lat});

                    auto forward_speed_iter =
                        find(segment_speed_lookup, Segment{u->node_id, v->node_id});
                    if (forward_speed_iter != segment_speed_lookup.end())
                    {
                        int new_segment_weight =
                            std::max(1, static_cast<int>(std::floor(
                                            (segment_length * 10.) /
                                                (forward_speed_iter->speed_source.speed / 3.6) +
                                            .5)));
                        m_geometry_list[forward_begin + leaf_object.fwd_segment_position].weight =
                            new_segment_weight;
                        m_geometry_datasource[forward_begin + leaf_object.fwd_segment_position] =
                            forward_speed_iter->speed_source.source;

                        // count statistics for logging
                        segment_speeds_counters[forward_speed_iter->speed_source.source] += 1;
                    }
                    else
                    {
                        // count statistics for logging
                        segment_speeds_counters[LUA_SOURCE] += 1;
                    }
                }
                if (leaf_object.reverse_packed_geometry_id != SPECIAL_EDGEID)
                {
                    const unsigned reverse_begin =
                        m_geometry_indices.at(leaf_object.reverse_packed_geometry_id);
                    const unsigned reverse_end =
                        m_geometry_indices.at(leaf_object.reverse_packed_geometry_id + 1);

                    int rev_segment_position =
                        (reverse_end - reverse_begin) - leaf_object.fwd_segment_position - 1;
                    if (rev_segment_position == 0)
                    {
                        u = &(internal_to_external_node_map[leaf_object.v]);
                        v = &(
                            internal_to_external_node_map[m_geometry_list[reverse_begin].node_id]);
                    }
                    else
                    {
                        u = &(
                            internal_to_external_node_map[m_geometry_list[reverse_begin +
                                                                          rev_segment_position - 1]
                                                              .node_id]);
                        v = &(internal_to_external_node_map
                                  [m_geometry_list[reverse_begin + rev_segment_position].node_id]);
                    }
                    const double segment_length = util::coordinate_calculation::greatCircleDistance(
                        util::Coordinate{u->lon, u->lat}, util::Coordinate{v->lon, v->lat});

                    auto reverse_speed_iter =
                        find(segment_speed_lookup, Segment{u->node_id, v->node_id});
                    if (reverse_speed_iter != segment_speed_lookup.end())
                    {
                        int new_segment_weight =
                            std::max(1, static_cast<int>(std::floor(
                                            (segment_length * 10.) /
                                                (reverse_speed_iter->speed_source.speed / 3.6) +
                                            .5)));
                        m_geometry_list[reverse_begin + rev_segment_position].weight =
                            new_segment_weight;
                        m_geometry_datasource[reverse_begin + rev_segment_position] =
                            reverse_speed_iter->speed_source.source;

                        // count statistics for logging
                        segment_speeds_counters[reverse_speed_iter->speed_source.source] += 1;
                    }
                    else
                    {
                        // count statistics for logging
                        segment_speeds_counters[LUA_SOURCE] += 1;
                    }
                }
            }
        }); // parallel_for_each

        for (std::size_t i = 0; i < segment_speeds_counters.size(); i++)
        {
            if (i == LUA_SOURCE)
            {
                util::SimpleLogger().Write() << "Used " << segment_speeds_counters[LUA_SOURCE]
                                             << " speeds from LUA profile or input map";
            }
            else
            {
                // segments_speeds_counters has 0 as LUA, segment_speed_filenames not, thus we need
                // to susbstract 1 to avoid off-by-one error
                util::SimpleLogger().Write() << "Used " << segment_speeds_counters[i]
                                             << " speeds from " << segment_speed_filenames[i - 1];
            }
        }
    }

    const auto maybe_save_geometries = [&] {
        if (!(update_edge_weights || update_turn_penalties))
            return;

        // Now save out the updated compressed geometries
        std::ofstream geometry_stream(geometry_filename, std::ios::binary);
        if (!geometry_stream)
        {
            throw util::exception("Failed to open " + geometry_filename + " for writing");
        }
        const unsigned number_of_indices = m_geometry_indices.size();
        const unsigned number_of_compressed_geometries = m_geometry_list.size();
        geometry_stream.write(reinterpret_cast<const char *>(&number_of_indices), sizeof(unsigned));
        geometry_stream.write(reinterpret_cast<char *>(&(m_geometry_indices[0])),
                              number_of_indices * sizeof(unsigned));
        geometry_stream.write(reinterpret_cast<const char *>(&number_of_compressed_geometries),
                              sizeof(unsigned));
        geometry_stream.write(reinterpret_cast<char *>(&(m_geometry_list[0])),
                              number_of_compressed_geometries *
                                  sizeof(extractor::CompressedEdgeContainer::CompressedEdge));
    };

    const auto save_datasource_indexes = [&] {
        std::ofstream datasource_stream(datasource_indexes_filename, std::ios::binary);
        if (!datasource_stream)
        {
            throw util::exception("Failed to open " + datasource_indexes_filename + " for writing");
        }
        auto number_of_datasource_entries = m_geometry_datasource.size();
        datasource_stream.write(reinterpret_cast<const char *>(&number_of_datasource_entries),
                                sizeof(number_of_datasource_entries));
        if (number_of_datasource_entries > 0)
        {
            datasource_stream.write(reinterpret_cast<char *>(&(m_geometry_datasource[0])),
                                    number_of_datasource_entries * sizeof(uint8_t));
        }
    };

    const auto save_datastore_names = [&] {
        std::ofstream datasource_stream(datasource_names_filename, std::ios::binary);
        if (!datasource_stream)
        {
            throw util::exception("Failed to open " + datasource_names_filename + " for writing");
        }
        datasource_stream << "lua profile" << std::endl;
        for (auto const &name : segment_speed_filenames)
        {
            // Only write the filename, without path or extension.
            // This prevents information leakage, and keeps names short
            // for rendering in the debug tiles.
            const boost::filesystem::path p(name);
            datasource_stream << p.stem().string() << std::endl;
        }
    };

    tbb::parallel_invoke(maybe_save_geometries, save_datasource_indexes, save_datastore_names);

    // TODO: can we read this in bulk?  util::DeallocatingVector isn't necessarily
    // all stored contiguously
    for (; number_of_edges > 0; --number_of_edges)
    {
        extractor::EdgeBasedEdge inbuffer;
        input_stream.read((char *)&inbuffer, sizeof(extractor::EdgeBasedEdge));
        if (update_edge_weights || update_turn_penalties)
        {
            // Processing-time edge updates
            unsigned fixed_penalty;
            edge_fixed_penalties_input_stream.read(reinterpret_cast<char *>(&fixed_penalty),
                                                   sizeof(fixed_penalty));

            int new_weight = 0;

            unsigned num_osm_nodes = 0;
            edge_segment_input_stream.read(reinterpret_cast<char *>(&num_osm_nodes),
                                           sizeof(num_osm_nodes));
            OSMNodeID previous_osm_node_id;
            edge_segment_input_stream.read(reinterpret_cast<char *>(&previous_osm_node_id),
                                           sizeof(previous_osm_node_id));
            OSMNodeID this_osm_node_id;
            double segment_length;
            int segment_weight;
            int compressed_edge_nodes = static_cast<int>(num_osm_nodes);
            --num_osm_nodes;
            for (; num_osm_nodes != 0; --num_osm_nodes)
            {
                edge_segment_input_stream.read(reinterpret_cast<char *>(&this_osm_node_id),
                                               sizeof(this_osm_node_id));
                edge_segment_input_stream.read(reinterpret_cast<char *>(&segment_length),
                                               sizeof(segment_length));
                edge_segment_input_stream.read(reinterpret_cast<char *>(&segment_weight),
                                               sizeof(segment_weight));

                auto speed_iter =
                    find(segment_speed_lookup, Segment{previous_osm_node_id, this_osm_node_id});
                if (speed_iter != segment_speed_lookup.end())
                {
                    // This sets the segment weight using the same formula as the
                    // EdgeBasedGraphFactory for consistency.  The *why* of this formula
                    // is lost in the annals of time.
                    int new_segment_weight = std::max(
                        1,
                        static_cast<int>(std::floor(
                            (segment_length * 10.) / (speed_iter->speed_source.speed / 3.6) + .5)));
                    new_weight += new_segment_weight;
                }
                else
                {
                    // If no lookup found, use the original weight value for this segment
                    new_weight += segment_weight;
                }

                previous_osm_node_id = this_osm_node_id;
            }

            OSMNodeID from_id;
            OSMNodeID via_id;
            OSMNodeID to_id;
            edge_fixed_penalties_input_stream.read(reinterpret_cast<char *>(&from_id),
                                                   sizeof(from_id));
            edge_fixed_penalties_input_stream.read(reinterpret_cast<char *>(&via_id),
                                                   sizeof(via_id));
            edge_fixed_penalties_input_stream.read(reinterpret_cast<char *>(&to_id), sizeof(to_id));

            const auto turn_iter =
                turn_penalty_lookup.find(std::make_tuple(from_id, via_id, to_id));
            if (turn_iter != turn_penalty_lookup.end())
            {
                int new_turn_weight = static_cast<int>(turn_iter->second.first * 10);

                if (new_turn_weight + new_weight < compressed_edge_nodes)
                {
                    util::SimpleLogger().Write(logWARNING)
                        << "turn penalty " << turn_iter->second.first << " for turn " << from_id
                        << ", " << via_id << ", " << to_id
                        << " is too negative: clamping turn weight to " << compressed_edge_nodes;
                }

                inbuffer.weight = std::max(new_turn_weight + new_weight, compressed_edge_nodes);
            }
            else
            {
                inbuffer.weight = fixed_penalty + new_weight;
            }
        }

        edge_based_edge_list.emplace_back(std::move(inbuffer));
    }

    util::SimpleLogger().Write() << "Done reading edges";
    return max_edge_id;
}

void Contractor::ReadNodeLevels(std::vector<float> &node_levels) const
{
    boost::filesystem::ifstream order_input_stream(config.level_output_path, std::ios::binary);

    unsigned level_size;
    order_input_stream.read((char *)&level_size, sizeof(unsigned));
    node_levels.resize(level_size);
    order_input_stream.read((char *)node_levels.data(), sizeof(float) * node_levels.size());
}

void Contractor::WriteNodeLevels(std::vector<float> &&in_node_levels) const
{
    std::vector<float> node_levels(std::move(in_node_levels));

    boost::filesystem::ofstream order_output_stream(config.level_output_path, std::ios::binary);

    unsigned level_size = node_levels.size();
    order_output_stream.write((char *)&level_size, sizeof(unsigned));
    order_output_stream.write((char *)node_levels.data(), sizeof(float) * node_levels.size());
}

void Contractor::WriteCoreNodeMarker(std::vector<bool> &&in_is_core_node) const
{
    std::vector<bool> is_core_node(std::move(in_is_core_node));
    std::vector<char> unpacked_bool_flags(std::move(is_core_node.size()));
    for (auto i = 0u; i < is_core_node.size(); ++i)
    {
        unpacked_bool_flags[i] = is_core_node[i] ? 1 : 0;
    }

    boost::filesystem::ofstream core_marker_output_stream(config.core_output_path,
                                                          std::ios::binary);
    unsigned size = unpacked_bool_flags.size();
    core_marker_output_stream.write((char *)&size, sizeof(unsigned));
    core_marker_output_stream.write((char *)unpacked_bool_flags.data(),
                                    sizeof(char) * unpacked_bool_flags.size());
}

std::size_t
Contractor::WriteContractedGraph(unsigned max_node_id,
                                 const util::DeallocatingVector<QueryEdge> &contracted_edge_list)
{
    // Sorting contracted edges in a way that the static query graph can read some in in-place.
    tbb::parallel_sort(contracted_edge_list.begin(), contracted_edge_list.end());
    const unsigned contracted_edge_count = contracted_edge_list.size();
    util::SimpleLogger().Write() << "Serializing compacted graph of " << contracted_edge_count
                                 << " edges";

    const util::FingerPrint fingerprint = util::FingerPrint::GetValid();
    boost::filesystem::ofstream hsgr_output_stream(config.graph_output_path, std::ios::binary);
    hsgr_output_stream.write((char *)&fingerprint, sizeof(util::FingerPrint));
    const unsigned max_used_node_id = [&contracted_edge_list] {
        unsigned tmp_max = 0;
        for (const QueryEdge &edge : contracted_edge_list)
        {
            BOOST_ASSERT(SPECIAL_NODEID != edge.source);
            BOOST_ASSERT(SPECIAL_NODEID != edge.target);
            tmp_max = std::max(tmp_max, edge.source);
            tmp_max = std::max(tmp_max, edge.target);
        }
        return tmp_max;
    }();

    util::SimpleLogger().Write(logDEBUG) << "input graph has " << (max_node_id + 1) << " nodes";
    util::SimpleLogger().Write(logDEBUG) << "contracted graph has " << (max_used_node_id + 1)
                                         << " nodes";

    std::vector<util::StaticGraph<EdgeData>::NodeArrayEntry> node_array;
    // make sure we have at least one sentinel
    node_array.resize(max_node_id + 2);

    util::SimpleLogger().Write() << "Building node array";
    util::StaticGraph<EdgeData>::EdgeIterator edge = 0;
    util::StaticGraph<EdgeData>::EdgeIterator position = 0;
    util::StaticGraph<EdgeData>::EdgeIterator last_edge;

    // initializing 'first_edge'-field of nodes:
    for (const auto node : util::irange(0u, max_used_node_id + 1))
    {
        last_edge = edge;
        while ((edge < contracted_edge_count) && (contracted_edge_list[edge].source == node))
        {
            ++edge;
        }
        node_array[node].first_edge = position; //=edge
        position += edge - last_edge;           // remove
    }

    for (const auto sentinel_counter :
         util::irange<unsigned>(max_used_node_id + 1, node_array.size()))
    {
        // sentinel element, guarded against underflow
        node_array[sentinel_counter].first_edge = contracted_edge_count;
    }

    util::SimpleLogger().Write() << "Serializing node array";

    RangebasedCRC32 crc32_calculator;
    const unsigned edges_crc32 = crc32_calculator(contracted_edge_list);
    util::SimpleLogger().Write() << "Writing CRC32: " << edges_crc32;

    const unsigned node_array_size = node_array.size();
    // serialize crc32, aka checksum
    hsgr_output_stream.write((char *)&edges_crc32, sizeof(unsigned));
    // serialize number of nodes
    hsgr_output_stream.write((char *)&node_array_size, sizeof(unsigned));
    // serialize number of edges
    hsgr_output_stream.write((char *)&contracted_edge_count, sizeof(unsigned));
    // serialize all nodes
    if (node_array_size > 0)
    {
        hsgr_output_stream.write((char *)&node_array[0],
                                 sizeof(util::StaticGraph<EdgeData>::NodeArrayEntry) *
                                     node_array_size);
    }

    // serialize all edges
    util::SimpleLogger().Write() << "Building edge array";
    int number_of_used_edges = 0;

    util::StaticGraph<EdgeData>::EdgeArrayEntry current_edge;
    for (const auto edge : util::irange<std::size_t>(0UL, contracted_edge_list.size()))
    {
        // some self-loops are required for oneway handling. Need to assertthat we only keep these
        // (TODO)
        // no eigen loops
        // BOOST_ASSERT(contracted_edge_list[edge].source != contracted_edge_list[edge].target ||
        // node_represents_oneway[contracted_edge_list[edge].source]);
        current_edge.target = contracted_edge_list[edge].target;
        current_edge.data = contracted_edge_list[edge].data;

        // every target needs to be valid
        BOOST_ASSERT(current_edge.target <= max_used_node_id);
#ifndef NDEBUG
        if (current_edge.data.distance <= 0)
        {
            util::SimpleLogger().Write(logWARNING)
                << "Edge: " << edge << ",source: " << contracted_edge_list[edge].source
                << ", target: " << contracted_edge_list[edge].target
                << ", dist: " << current_edge.data.distance;

            util::SimpleLogger().Write(logWARNING) << "Failed at adjacency list of node "
                                                   << contracted_edge_list[edge].source << "/"
                                                   << node_array.size() - 1;
            return 1;
        }
#endif
        hsgr_output_stream.write((char *)&current_edge,
                                 sizeof(util::StaticGraph<EdgeData>::EdgeArrayEntry));

        ++number_of_used_edges;
    }

    return number_of_used_edges;
}

/**
 \brief Build contracted graph.
 */
void Contractor::ContractGraph(
    const unsigned max_edge_id,
    util::DeallocatingVector<extractor::EdgeBasedEdge> &edge_based_edge_list,
    util::DeallocatingVector<QueryEdge> &contracted_edge_list,
    std::vector<EdgeWeight> &&node_weights,
    std::vector<bool> &is_core_node,
    std::vector<float> &inout_node_levels) const
{
    std::vector<float> node_levels;
    node_levels.swap(inout_node_levels);

    GraphContractor graph_contractor(max_edge_id + 1, edge_based_edge_list, std::move(node_levels),
                                     std::move(node_weights));
    graph_contractor.Run(config.core_factor);
    graph_contractor.GetEdges(contracted_edge_list);
    graph_contractor.GetCoreMarker(is_core_node);
    graph_contractor.GetNodeLevels(inout_node_levels);
}
}
}
