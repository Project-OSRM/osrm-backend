#include "util/percent.hpp"
#include "contractor/query_edge.hpp"
#include "util/static_graph.hpp"
#include "util/integer_range.hpp"
#include "util/graph_loader.hpp"
#include "util/simple_logger.hpp"
#include "util/osrm_exception.hpp"

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>

#include <memory>
#include <vector>

using EdgeData = QueryEdge::EdgeData;
using QueryGraph = StaticGraph<EdgeData>;

int main(int argc, char *argv[])
{
    LogPolicy::GetInstance().Unmute();
    try
    {
        if (argc != 2)
        {
            SimpleLogger().Write(logWARNING) << "usage: " << argv[0] << " <file.hsgr>";
            return 1;
        }

        boost::filesystem::path hsgr_path(argv[1]);

        std::vector<QueryGraph::NodeArrayEntry> node_list;
        std::vector<QueryGraph::EdgeArrayEntry> edge_list;
        SimpleLogger().Write() << "loading graph from " << hsgr_path.string();

        unsigned m_check_sum = 0;
        unsigned m_number_of_nodes =
            readHSGRFromStream(hsgr_path, node_list, edge_list, &m_check_sum);
        SimpleLogger().Write() << "expecting " << m_number_of_nodes
                               << " nodes, checksum: " << m_check_sum;
        BOOST_ASSERT_MSG(0 != node_list.size(), "node list empty");
        SimpleLogger().Write() << "loaded " << node_list.size() << " nodes and " << edge_list.size()
                               << " edges";
        auto m_query_graph = std::make_shared<QueryGraph>(node_list, edge_list);

        BOOST_ASSERT_MSG(0 == node_list.size(), "node list not flushed");
        BOOST_ASSERT_MSG(0 == edge_list.size(), "edge list not flushed");

        Percent progress(m_query_graph->GetNumberOfNodes());
        for (const auto node_u : osrm::irange(0u, m_query_graph->GetNumberOfNodes()))
        {
            for (const auto eid : m_query_graph->GetAdjacentEdgeRange(node_u))
            {
                const EdgeData &data = m_query_graph->GetEdgeData(eid);
                if (!data.shortcut)
                {
                    continue;
                }
                const unsigned node_v = m_query_graph->GetTarget(eid);
                const EdgeID edge_id_1 = m_query_graph->FindEdgeInEitherDirection(node_u, data.id);
                if (SPECIAL_EDGEID == edge_id_1)
                {
                    throw osrm::exception("cannot find first segment of edge (" +
                                          std::to_string(node_u) + "," + std::to_string(data.id) +
                                          "," + std::to_string(node_v) + "), eid: " +
                                          std::to_string(eid));
                }
                const EdgeID edge_id_2 = m_query_graph->FindEdgeInEitherDirection(data.id, node_v);
                if (SPECIAL_EDGEID == edge_id_2)
                {
                    throw osrm::exception("cannot find second segment of edge (" +
                                          std::to_string(node_u) + "," + std::to_string(data.id) +
                                          "," + std::to_string(node_v) + "), eid: " +
                                          std::to_string(eid));
                }
            }
            progress.printStatus(node_u);
        }
        m_query_graph.reset();
        SimpleLogger().Write() << "Data file " << argv[0] << " appears to be OK";
    }
    catch (const std::exception &e)
    {
        SimpleLogger().Write(logWARNING) << "[exception] " << e.what();
    }
    return 0;
}
