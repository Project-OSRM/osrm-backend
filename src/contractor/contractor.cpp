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
        storage::io::FileReader reader(config.GetPath(".osrm.enw"),
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
        files::readLevels(config.GetPath(".osrm.level"), node_levels);
    }

    util::DeallocatingVector<QueryEdge> contracted_edge_list;
    { // own scope to not keep the contractor around
        GraphContractor graph_contractor(max_edge_id + 1,
                                         adaptToContractorInput(std::move(edge_based_edge_list)),
                                         std::move(node_levels),
                                         std::move(node_weights));
        graph_contractor.Run(config.core_factor);

        contracted_edge_list = graph_contractor.GetEdges<QueryEdge>();
        is_core_node = graph_contractor.GetCoreMarker();
        node_levels = graph_contractor.GetNodeLevels();
    }
    TIMER_STOP(contraction);

    util::Log() << "Contraction took " << TIMER_SEC(contraction) << " sec";

    {
        RangebasedCRC32 crc32_calculator;
        const unsigned checksum = crc32_calculator(contracted_edge_list);

        files::writeGraph(config.GetPath(".osrm.hsgr"),
                          checksum,
                          QueryGraph{max_edge_id + 1, std::move(contracted_edge_list)});
    }

    files::writeCoreMarker(config.GetPath(".osrm.core"), is_core_node);
    if (!config.use_cached_priority)
    {
        files::writeLevels(config.GetPath(".osrm.level"), node_levels);
    }

    TIMER_STOP(preparing);

    util::Log() << "Preprocessing : " << TIMER_SEC(preparing) << " seconds";

    util::Log() << "finished preprocessing";

    return 0;
}

} // namespace contractor
} // namespace osrm
