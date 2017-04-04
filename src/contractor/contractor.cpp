#include "contractor/contractor.hpp"
#include "contractor/crc32_processor.hpp"
#include "contractor/files.hpp"
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
        storage::io::FileReader reader(config.node_file_path,
                                       storage::io::FileReader::VerifyFingerprint);
        storage::serialization::read(reader, node_weights);
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

    WriteContractedGraph(max_edge_id, std::move(contracted_edge_list));
    WriteCoreNodeMarker(std::move(is_core_node));
    if (!config.use_cached_priority)
    {
        WriteNodeLevels(std::move(node_levels));
    }

    TIMER_STOP(preparing);

    util::Log() << "Preprocessing : " << TIMER_SEC(preparing) << " seconds";

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

    storage::io::FileWriter writer(config.level_output_path,
                                   storage::io::FileWriter::HasNoFingerprint);

    storage::serialization::write(writer, node_levels);
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

void Contractor::WriteContractedGraph(unsigned max_node_id,
                                      util::DeallocatingVector<QueryEdge> contracted_edge_list)
{
    // Sorting contracted edges in a way that the static query graph can read some in in-place.
    tbb::parallel_sort(contracted_edge_list.begin(), contracted_edge_list.end());
    auto new_end = std::unique(contracted_edge_list.begin(), contracted_edge_list.end());
    contracted_edge_list.resize(new_end - contracted_edge_list.begin());

    RangebasedCRC32 crc32_calculator;
    const unsigned checksum = crc32_calculator(contracted_edge_list);

    QueryGraph query_graph{max_node_id + 1, contracted_edge_list};

    files::writeGraph(config.graph_output_path, checksum, query_graph);
}

} // namespace contractor
} // namespace osrm
