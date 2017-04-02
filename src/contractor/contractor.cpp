#include "contractor/contractor.hpp"
#include "contractor/crc32_processor.hpp"
#include "contractor/graph_contractor.hpp"
#include "contractor/graph_contractor_adaptors.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/edge_based_graph_factory.hpp"
#include "extractor/node_based_edge.hpp"

#include "storage/io.hpp"

#include "updater/updater.hpp"

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/graph_loader.hpp"
#include "util/integer_range.hpp"
#include "util/log.hpp"
#include "util/static_graph.hpp"
#include "util/string_util.hpp"
#include "util/timing_util.hpp"
#include "util/typedefs.hpp"

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>

namespace osrm
{
namespace contractor
{

int Contractor::Run()
{
    if (config.core_factor > 1.0 || config.core_factor < 0)
    {
        throw util::exception("Core factor must be between 0.0 to 1.0 (inclusive)" + SOURCE_REF);
    }

    TIMER_START(preparing);

    util::Log() << "Reading node weights.";
    std::vector<EdgeWeight> node_weights;
    {
        storage::io::FileReader node_file(config.node_file_path,
                                          storage::io::FileReader::VerifyFingerprint);
        node_file.DeserializeVector(node_weights);
    }
    util::Log() << "Done reading node weights.";

    util::Log() << "Loading edge-expanded graph representation";

    std::vector<extractor::EdgeBasedEdge> edge_based_edge_list;

    updater::Updater updater(config.updater_config);
    EdgeID max_edge_id = updater.LoadAndUpdateEdgeExpandedGraph(edge_based_edge_list, node_weights);

    // Contracting the edge-expanded graph

    TIMER_START(contraction);
    std::vector<bool> is_core_node;
    std::vector<float> node_levels;
    if (config.use_cached_priority)
    {
        ReadNodeLevels(node_levels);
    }

    util::DeallocatingVector<QueryEdge> contracted_edge_list;
    { // own scope to not keep the contractor around
        GraphContractor graph_contractor(max_edge_id + 1,
                                         adaptToContractorInput(std::move(edge_based_edge_list)),
                                         std::move(node_levels),
                                         std::move(node_weights));
        graph_contractor.Run(config.core_factor);
        graph_contractor.GetEdges(contracted_edge_list);
        graph_contractor.GetCoreMarker(is_core_node);
        graph_contractor.GetNodeLevels(node_levels);
    }
    TIMER_STOP(contraction);

    util::Log() << "Contraction took " << TIMER_SEC(contraction) << " sec";

    std::size_t number_of_used_edges = WriteContractedGraph(max_edge_id, contracted_edge_list);
    WriteCoreNodeMarker(std::move(is_core_node));
    if (!config.use_cached_priority)
    {
        WriteNodeLevels(std::move(node_levels));
    }

    TIMER_STOP(preparing);

    const auto nodes_per_second =
        static_cast<std::uint64_t>((max_edge_id + 1) / TIMER_SEC(contraction));
    const auto edges_per_second =
        static_cast<std::uint64_t>(number_of_used_edges / TIMER_SEC(contraction));

    util::Log() << "Preprocessing : " << TIMER_SEC(preparing) << " seconds";
    util::Log() << "Contraction: " << nodes_per_second << " nodes/sec and " << edges_per_second
                << " edges/sec";

    util::Log() << "finished preprocessing";

    return 0;
}

void Contractor::ReadNodeLevels(std::vector<float> &node_levels) const
{
    storage::io::FileReader order_file(config.level_output_path,
                                       storage::io::FileReader::HasNoFingerprint);

    const auto level_size = order_file.ReadElementCount32();
    node_levels.resize(level_size);
    order_file.ReadInto(node_levels);
}

void Contractor::WriteNodeLevels(std::vector<float> &&in_node_levels) const
{
    std::vector<float> node_levels(std::move(in_node_levels));

    storage::io::FileWriter node_level_file(config.level_output_path,
                                            storage::io::FileWriter::HasNoFingerprint);

    node_level_file.SerializeVector(node_levels);
}

void Contractor::WriteCoreNodeMarker(std::vector<bool> &&in_is_core_node) const
{
    std::vector<bool> is_core_node(std::move(in_is_core_node));
    std::vector<char> unpacked_bool_flags(std::move(is_core_node.size()));
    for (auto i = 0u; i < is_core_node.size(); ++i)
    {
        unpacked_bool_flags[i] = is_core_node[i] ? 1 : 0;
    }

    storage::io::FileWriter core_marker_output_file(config.core_output_path,
                                                    storage::io::FileWriter::HasNoFingerprint);

    const std::size_t count = unpacked_bool_flags.size();
    core_marker_output_file.WriteElementCount32(count);
    core_marker_output_file.WriteFrom(unpacked_bool_flags.data(), count);
}

std::size_t
Contractor::WriteContractedGraph(unsigned max_node_id,
                                 const util::DeallocatingVector<QueryEdge> &contracted_edge_list)
{
    // Sorting contracted edges in a way that the static query graph can read some in in-place.
    tbb::parallel_sort(contracted_edge_list.begin(), contracted_edge_list.end());
    const std::uint64_t contracted_edge_count = contracted_edge_list.size();
    util::Log() << "Serializing compacted graph of " << contracted_edge_count << " edges";

    storage::io::FileWriter hsgr_output_file(config.graph_output_path,
                                             storage::io::FileWriter::GenerateFingerprint);

    const NodeID max_used_node_id = [&contracted_edge_list] {
        NodeID tmp_max = 0;
        for (const QueryEdge &edge : contracted_edge_list)
        {
            BOOST_ASSERT(SPECIAL_NODEID != edge.source);
            BOOST_ASSERT(SPECIAL_NODEID != edge.target);
            tmp_max = std::max(tmp_max, edge.source);
            tmp_max = std::max(tmp_max, edge.target);
        }
        return tmp_max;
    }();

    util::Log(logDEBUG) << "input graph has " << (max_node_id + 1) << " nodes";
    util::Log(logDEBUG) << "contracted graph has " << (max_used_node_id + 1) << " nodes";

    std::vector<util::StaticGraph<EdgeData>::NodeArrayEntry> node_array;
    // make sure we have at least one sentinel
    node_array.resize(max_node_id + 2);

    util::Log() << "Building node array";
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

    util::Log() << "Serializing node array";

    RangebasedCRC32 crc32_calculator;
    const unsigned edges_crc32 = crc32_calculator(contracted_edge_list);
    util::Log() << "Writing CRC32: " << edges_crc32;

    const std::uint64_t node_array_size = node_array.size();
    // serialize crc32, aka checksum
    hsgr_output_file.WriteOne(edges_crc32);
    // serialize number of nodes
    hsgr_output_file.WriteOne(node_array_size);
    // serialize number of edges
    hsgr_output_file.WriteOne(contracted_edge_count);
    // serialize all nodes
    if (node_array_size > 0)
    {
        hsgr_output_file.WriteFrom(node_array.data(), node_array_size);
    }

    // serialize all edges
    util::Log() << "Building edge array";
    std::size_t number_of_used_edges = 0;

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
        if (current_edge.data.weight <= 0)
        {
            util::Log(logWARNING) << "Edge: " << edge
                                  << ",source: " << contracted_edge_list[edge].source
                                  << ", target: " << contracted_edge_list[edge].target
                                  << ", weight: " << current_edge.data.weight;

            util::Log(logWARNING) << "Failed at adjacency list of node "
                                  << contracted_edge_list[edge].source << "/"
                                  << node_array.size() - 1;
            throw util::exception("Edge weight is <= 0" + SOURCE_REF);
        }
#endif
        hsgr_output_file.WriteOne(current_edge);

        ++number_of_used_edges;
    }

    return number_of_used_edges;
}

} // namespace contractor
} // namespace osrm
