#include "contractor/contractor.hpp"
#include "contractor/crc32_processor.hpp"
#include "contractor/graph_contractor.hpp"

#include "extractor/node_based_edge.hpp"
#include "extractor/compressed_edge_container.hpp"

#include "util/static_graph.hpp"
#include "util/static_rtree.hpp"
#include "util/graph_loader.hpp"
#include "util/io.hpp"
#include "util/integer_range.hpp"
#include "util/exception.hpp"
#include "util/simple_logger.hpp"
#include "util/string_util.hpp"
#include "util/timing_util.hpp"
#include "util/typedefs.hpp"

#include <fast-cpp-csv-parser/csv.h>

#include <boost/assert.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/functional/hash.hpp>

#include <tbb/parallel_sort.h>

#include <cstdint>
#include <bitset>
#include <chrono>
#include <memory>
#include <thread>
#include <iterator>
#include <tuple>

namespace std
{

template <> struct hash<std::pair<OSMNodeID, OSMNodeID>>
{
    std::size_t operator()(const std::pair<OSMNodeID, OSMNodeID> &k) const
    {
        return static_cast<uint64_t>(k.first) ^ (static_cast<uint64_t>(k.second) << 12);
    }
};

template <> struct hash<std::tuple<OSMNodeID, OSMNodeID, OSMNodeID>>
{
    std::size_t operator()(const std::tuple<OSMNodeID, OSMNodeID, OSMNodeID> &k) const
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
    static_assert(sizeof(extractor::NodeBasedEdge) == 20,
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
    util::SimpleLogger().Write() << "Opening " << edge_based_graph_filename;
    boost::filesystem::ifstream input_stream(edge_based_graph_filename, std::ios::binary);

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

    std::unordered_map<std::pair<OSMNodeID, OSMNodeID>, std::pair<unsigned, uint8_t>>
        segment_speed_lookup;
    std::unordered_map<std::tuple<OSMNodeID, OSMNodeID, OSMNodeID>, std::pair<double, uint8_t>>
        turn_penalty_lookup;

    // If we update the edge weights, this file will hold the datasource information
    // for each segment
    std::vector<uint8_t> m_geometry_datasource;

    if (update_edge_weights || update_turn_penalties)
    {
        std::uint8_t segment_file_id = 1;
        std::uint8_t turn_file_id = 1;

        if (update_edge_weights)
        {
            for (auto segment_speed_filename : segment_speed_filenames)
            {
                util::SimpleLogger().Write()
                    << "Segment speed data supplied, will update edge weights from "
                    << segment_speed_filename;
                io::CSVReader<3> csv_in(segment_speed_filename);
                csv_in.set_header("from_node", "to_node", "speed");
                std::uint64_t from_node_id{};
                std::uint64_t to_node_id{};
                unsigned speed{};
                while (csv_in.read_row(from_node_id, to_node_id, speed))
                {
                    segment_speed_lookup[std::make_pair(OSMNodeID(from_node_id),
                                                        OSMNodeID(to_node_id))] =
                        std::make_pair(speed, segment_file_id);
                }
                ++segment_file_id;

                // Check for overflow
                if (segment_file_id == 0)
                {
                    throw util::exception(
                        "Sorry, there's a limit of 255 segment speed files; you supplied too many");
                }
            }
        }

        if (update_turn_penalties)
        {
            for (auto turn_penalty_filename : turn_penalty_filenames)
            {
                util::SimpleLogger().Write()
                    << "Turn penalty data supplied, will update turn penalties from "
                    << turn_penalty_filename;
                io::CSVReader<4> csv_in(turn_penalty_filename);
                csv_in.set_header("from_node", "via_node", "to_node", "penalty");
                uint64_t from_node_id{};
                uint64_t via_node_id{};
                uint64_t to_node_id{};
                double penalty{};
                while (csv_in.read_row(from_node_id, via_node_id, to_node_id, penalty))
                {
                    turn_penalty_lookup[std::make_tuple(
                        OSMNodeID(from_node_id), OSMNodeID(via_node_id), OSMNodeID(to_node_id))] =
                        std::make_pair(penalty, turn_file_id);
                }
                ++turn_file_id;

                // Check for overflow
                if (turn_file_id == 0)
                {
                    throw util::exception(
                        "Sorry, there's a limit of 255 turn penalty files; you supplied too many");
                }
            }
        }

        std::vector<extractor::QueryNode> internal_to_external_node_map;

        // Here, we have to update the compressed geometry weights
        // First, we need the external-to-internal node lookup table
        {
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
        }

        std::vector<unsigned> m_geometry_indices;
        std::vector<extractor::CompressedEdgeContainer::CompressedEdge> m_geometry_list;

        {
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
                geometry_stream.read(
                    (char *)&(m_geometry_list[0]),
                    number_of_compressed_geometries *
                        sizeof(extractor::CompressedEdgeContainer::CompressedEdge));
            }
        }

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
        {

            using LeafNode = util::StaticRTree<extractor::EdgeBasedNode>::LeafNode;

            std::ifstream leaf_node_file(rtree_leaf_filename, std::ios::binary | std::ios::in | std::ios::ate);
            if (!leaf_node_file)
            {
                throw util::exception("Failed to open " + rtree_leaf_filename);
            }
            std::size_t leaf_nodes_count = leaf_node_file.tellg() / sizeof(LeafNode);
            leaf_node_file.seekg(0, std::ios::beg);

            LeafNode current_node;
            while (leaf_nodes_count > 0)
            {
                leaf_node_file.read(reinterpret_cast<char *>(&current_node), sizeof(current_node));

                for (size_t i = 0; i < current_node.object_count; i++)
                {
                    auto &leaf_object = current_node.objects[i];
                    extractor::QueryNode *u;
                    extractor::QueryNode *v;

                    if (leaf_object.forward_packed_geometry_id != SPECIAL_EDGEID)
                    {
                        const unsigned forward_begin =
                            m_geometry_indices.at(leaf_object.forward_packed_geometry_id);

                        if (leaf_object.fwd_segment_position == 0)
                        {
                            u = &(internal_to_external_node_map[leaf_object.u]);
                            v = &(internal_to_external_node_map[m_geometry_list[forward_begin]
                                                                    .node_id]);
                        }
                        else
                        {
                            u = &(internal_to_external_node_map
                                      [m_geometry_list[forward_begin +
                                                       leaf_object.fwd_segment_position - 1]
                                           .node_id]);
                            v = &(internal_to_external_node_map
                                      [m_geometry_list[forward_begin +
                                                       leaf_object.fwd_segment_position]
                                           .node_id]);
                        }
                        const double segment_length =
                            util::coordinate_calculation::greatCircleDistance(
                                util::Coordinate{u->lon, u->lat}, util::Coordinate{v->lon, v->lat});

                        auto forward_speed_iter =
                            segment_speed_lookup.find(std::make_pair(u->node_id, v->node_id));
                        if (forward_speed_iter != segment_speed_lookup.end())
                        {
                            int new_segment_weight =
                                std::max(1, static_cast<int>(std::floor(
                                                (segment_length * 10.) /
                                                    (forward_speed_iter->second.first / 3.6) +
                                                .5)));
                            m_geometry_list[forward_begin + leaf_object.fwd_segment_position]
                                .weight = new_segment_weight;
                            m_geometry_datasource[forward_begin +
                                                  leaf_object.fwd_segment_position] =
                                forward_speed_iter->second.second;
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
                            v = &(internal_to_external_node_map[m_geometry_list[reverse_begin]
                                                                    .node_id]);
                        }
                        else
                        {
                            u = &(internal_to_external_node_map
                                      [m_geometry_list[reverse_begin + rev_segment_position - 1]
                                           .node_id]);
                            v = &(internal_to_external_node_map
                                      [m_geometry_list[reverse_begin + rev_segment_position]
                                           .node_id]);
                        }
                        const double segment_length =
                            util::coordinate_calculation::greatCircleDistance(
                                util::Coordinate{u->lon, u->lat}, util::Coordinate{v->lon, v->lat});

                        auto reverse_speed_iter =
                            segment_speed_lookup.find(std::make_pair(u->node_id, v->node_id));
                        if (reverse_speed_iter != segment_speed_lookup.end())
                        {
                            int new_segment_weight =
                                std::max(1, static_cast<int>(std::floor(
                                                (segment_length * 10.) /
                                                    (reverse_speed_iter->second.first / 3.6) +
                                                .5)));
                            m_geometry_list[reverse_begin + rev_segment_position].weight =
                                new_segment_weight;
                            m_geometry_datasource[reverse_begin + rev_segment_position] =
                                reverse_speed_iter->second.second;
                        }
                    }
                }
                --leaf_nodes_count;
            }
        }

        // Now save out the updated compressed geometries
        {
            std::ofstream geometry_stream(geometry_filename, std::ios::binary);
            if (!geometry_stream)
            {
                throw util::exception("Failed to open " + geometry_filename + " for writing");
            }
            const unsigned number_of_indices = m_geometry_indices.size();
            const unsigned number_of_compressed_geometries = m_geometry_list.size();
            geometry_stream.write(reinterpret_cast<const char *>(&number_of_indices),
                                  sizeof(unsigned));
            geometry_stream.write(reinterpret_cast<char *>(&(m_geometry_indices[0])),
                                  number_of_indices * sizeof(unsigned));
            geometry_stream.write(reinterpret_cast<const char *>(&number_of_compressed_geometries),
                                  sizeof(unsigned));
            geometry_stream.write(reinterpret_cast<char *>(&(m_geometry_list[0])),
                                  number_of_compressed_geometries *
                                      sizeof(extractor::CompressedEdgeContainer::CompressedEdge));
        }
    }

    {
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
    }

    {
        std::ofstream datasource_stream(datasource_names_filename, std::ios::binary);
        if (!datasource_stream)
        {
            throw util::exception("Failed to open " + datasource_names_filename + " for writing");
        }
        datasource_stream << "lua profile" << std::endl;
        for (auto const &name : segment_speed_filenames)
        {
            datasource_stream << name << std::endl;
        }
    }

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

                auto speed_iter = segment_speed_lookup.find(
                    std::make_pair(previous_osm_node_id, this_osm_node_id));
                if (speed_iter != segment_speed_lookup.end())
                {
                    // This sets the segment weight using the same formula as the
                    // EdgeBasedGraphFactory for consistency.  The *why* of this formula
                    // is lost in the annals of time.
                    int new_segment_weight = std::max(
                        1, static_cast<int>(std::floor(
                               (segment_length * 10.) / (speed_iter->second.first / 3.6) + .5)));
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

            auto turn_iter = turn_penalty_lookup.find(std::make_tuple(from_id, via_id, to_id));
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
    const unsigned max_used_node_id = [&contracted_edge_list]
    {
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
