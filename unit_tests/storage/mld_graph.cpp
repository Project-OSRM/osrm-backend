#include "customizer/files.hpp"
#include "partitioner/files.hpp"

#include "../common/temporary_file.hpp"

#include <boost/test/unit_test.hpp>

#include <array>
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

    osrm::customizer::MultiLevelEdgeBasedGraphView unbound_view;
    std::uint32_t view_checksum = 0;
    BOOST_CHECK_THROW(osrm::customizer::files::readGraph(archive.path, unbound_view, view_checksum),
                      osrm::util::exception);

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

BOOST_AUTO_TEST_CASE(rejects_unbound_compressed_partition_view)
{
    const osrm::partitioner::MultiLevelPartition original{{std::vector<CellID>(512, 0)}, {1}};
    TemporaryFile archive;
    osrm::partitioner::files::writePartition(archive.path, original);
    const osrm::storage::tar::FileReader reader(archive.path,
                                                osrm::storage::tar::FileReader::VerifyFingerprint);
    BOOST_REQUIRE(reader.HasEntry("/mld/multilevelpartition/partition_blocks/headers"));

    osrm::partitioner::MultiLevelPartitionView unbound_view;
    BOOST_CHECK_THROW(osrm::partitioner::files::readPartition(archive.path, unbound_view),
                      osrm::util::exception);

    osrm::partitioner::MultiLevelPartitionView::LevelData levels;
    osrm::partitioner::MultiLevelPartitionView plain_view(&levels, {}, {});
    BOOST_CHECK_THROW(osrm::partitioner::files::readPartition(archive.path, plain_view),
                      osrm::util::exception);
}

BOOST_AUTO_TEST_CASE(packed_edge_view_rejects_invalid_byte_counts)
{
    using Graph = osrm::util::StaticGraph<osrm::customizer::EdgeBasedGraphEdgeData,
                                          osrm::storage::Ownership::View>;
    std::array<Graph::NodeArrayEntry, 2> offsets{{{0}, {1}}};
    Graph graph({offsets.data(), offsets.size()}, {});
    for (const auto bytes : {0u, 7u, 8u, 9u, 20u})
    {
        std::vector<std::uint8_t> payload(bytes);
        BOOST_CHECK_THROW(graph.bindPackedEdges({payload.data(), payload.size()}),
                          osrm::util::exception);
    }

    std::array<std::uint8_t, 14> payload{};
    BOOST_CHECK_NO_THROW(graph.bindPackedEdges({payload.data(), payload.size()}));
    BOOST_CHECK_EQUAL(graph.GetNumberOfEdges(), 1);
    BOOST_CHECK_EQUAL(graph.GetTarget(0), 0);
}

BOOST_AUTO_TEST_CASE(packed_edge_deserialization_checks_bound_storage)
{
    using EdgeData = osrm::customizer::EdgeBasedGraphEdgeData;
    using View = osrm::util::StaticGraph<EdgeData, osrm::storage::Ownership::View>;
    using Graph = osrm::util::StaticGraph<EdgeData>;
    std::vector<View::NodeArrayEntry> offsets{{0}, {1}};
    std::array<std::uint8_t, 14> storage{};
    View view({offsets.data(), offsets.size()}, {});
    view.bindPackedEdges({storage.data(), storage.size()});
    for (const auto bytes : {7u, 8u, 14u, 20u})
    {
        TemporaryFile archive;
        {
            osrm::storage::tar::FileWriter writer(
                archive.path, osrm::storage::tar::FileWriter::GenerateFingerprint);
            osrm::storage::serialization::write(writer, "/graph/node_array", offsets);
            osrm::storage::serialization::write(
                writer, "/graph/edge_array6", std::vector<std::uint8_t>(bytes));
        }
        osrm::storage::tar::FileReader reader(archive.path,
                                              osrm::storage::tar::FileReader::VerifyFingerprint);
        Graph owned;
        if (bytes == storage.size())
        {
            BOOST_REQUIRE_NO_THROW(osrm::util::serialization::read(reader, "/graph", view));
            BOOST_REQUIRE_NO_THROW(osrm::util::serialization::read(reader, "/graph", owned));
            BOOST_CHECK_EQUAL(view.GetNumberOfEdges(), owned.GetNumberOfEdges());
            BOOST_CHECK_EQUAL(view.GetTarget(0), owned.GetTarget(0));
        }
        else
        {
            BOOST_CHECK_THROW(osrm::util::serialization::read(reader, "/graph", view),
                              osrm::util::exception);
            BOOST_CHECK_THROW(osrm::util::serialization::read(reader, "/graph", owned),
                              osrm::util::exception);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
