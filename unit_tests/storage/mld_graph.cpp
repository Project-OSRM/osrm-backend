#include "customizer/files.hpp"

#include "../common/temporary_file.hpp"

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <utility>
#include <vector>

BOOST_AUTO_TEST_SUITE(mld_graph_archive)

BOOST_AUTO_TEST_CASE(read_compressed_offsets_into_owned_graph)
{
    using Graph = osrm::customizer::MultiLevelEdgeBasedGraph;
    constexpr NodeID num_nodes = 257;
    constexpr EdgeID num_edges = num_nodes - 1;
    std::vector<Graph::NodeArrayEntry> offsets(num_nodes + 1);
    std::vector<Graph::EdgeArrayEntry> edges(num_edges);
    for (EdgeID edge = 0; edge < num_edges; ++edge)
    {
        offsets[edge].first_edge = edge;
        edges[edge] = {edge + 1, {edge + 1000}};
    }
    offsets[num_nodes - 1].first_edge = num_edges;
    offsets[num_nodes].first_edge = num_edges;
    const Graph original(std::move(offsets),
                         std::move(edges),
                         {},
                         std::vector<EdgeWeight>(num_nodes, EdgeWeight{7}),
                         std::vector<EdgeDuration>(num_nodes, EdgeDuration{9}),
                         std::vector<EdgeDistance>(num_nodes, EdgeDistance{11.f}),
                         std::vector<bool>(num_edges, true),
                         std::vector<bool>(num_edges, false));

    TemporaryFile archive;
    constexpr std::uint32_t checksum = 42;
    osrm::customizer::files::writeGraph(archive.path, original, checksum);
    const osrm::storage::tar::FileReader reader(archive.path,
                                                osrm::storage::tar::FileReader::VerifyFingerprint);
    BOOST_REQUIRE(reader.HasEntry("/mld/multilevelgraph/node_blocks/headers"));
    BOOST_CHECK(!reader.HasEntry("/mld/multilevelgraph/node_array"));

    Graph restored;
    std::uint32_t restored_checksum = 0;
    osrm::customizer::files::readGraph(archive.path, restored, restored_checksum);
    BOOST_CHECK_EQUAL(restored_checksum, checksum);
    BOOST_REQUIRE_EQUAL(restored.GetNumberOfNodes(), num_nodes);
    BOOST_REQUIRE_EQUAL(restored.GetNumberOfEdges(), num_edges);
    for (NodeID node = 0; node < num_nodes; ++node)
    {
        BOOST_CHECK_EQUAL(restored.BeginEdges(node), original.BeginEdges(node));
        BOOST_CHECK_EQUAL(restored.EndEdges(node), original.EndEdges(node));
    }
    for (EdgeID edge = 0; edge < num_edges; ++edge)
    {
        BOOST_CHECK_EQUAL(restored.GetTarget(edge), original.GetTarget(edge));
        BOOST_CHECK_EQUAL(restored.GetEdgeData(edge).turn_id, original.GetEdgeData(edge).turn_id);
    }
}

BOOST_AUTO_TEST_SUITE_END()
