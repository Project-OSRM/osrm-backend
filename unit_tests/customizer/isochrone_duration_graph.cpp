#include "customizer/edge_based_graph.hpp"
#include "customizer/files.hpp"
#include "customizer/serialization.hpp"

#include "engine/isochrone/duration_graph_builder.hpp"

#include "extractor/isochrone_transition.hpp"

#include "storage/serialization.hpp"
#include "storage/tar.hpp"

#include "util/exception.hpp"

#include "../common/temporary_file.hpp"

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <vector>

namespace
{
using namespace osrm;

customizer::MultiLevelEdgeBasedGraph
makeGraph(engine::isochrone::DurationGraph isochrone_graph = {})
{
    using Graph = customizer::MultiLevelEdgeBasedGraph;

    std::vector<Graph::NodeArrayEntry> node_array = {{0}, {1}, {1}};
    std::vector<Graph::EdgeArrayEntry> edge_array = {{{1, {0}}}};
    std::vector<Graph::EdgeOffset> node_to_edge_offset = {0, 0, 1};
    std::vector<EdgeWeight> node_weights = {{10}, {0}};
    std::vector<EdgeDuration> node_durations = {{10}, {0}};
    std::vector<EdgeDistance> node_distances = {{1}, {1}};
    std::vector<bool> is_forward_edge = {true};
    std::vector<bool> is_backward_edge = {false};

    return {std::move(node_array),
            std::move(edge_array),
            std::move(node_to_edge_offset),
            std::move(node_weights),
            std::move(node_durations),
            std::move(node_distances),
            std::move(is_forward_edge),
            std::move(is_backward_edge),
            std::move(isochrone_graph)};
}

void writeGraphWithChecksum(storage::tar::FileWriter &writer,
                            const customizer::MultiLevelEdgeBasedGraph &graph)
{
    writer.WriteElementCount64("/mld/connectivity_checksum", 1);
    writer.WriteFrom("/mld/connectivity_checksum", std::uint32_t{0xDEADBEEFU});
    customizer::serialization::write(writer, "/mld/multilevelgraph", graph);
}

} // namespace

BOOST_AUTO_TEST_SUITE(customizer_isochrone_duration_graph)

BOOST_AUTO_TEST_CASE(reads_a_legacy_mld_graph_without_isochrone_data)
{
    TemporaryFile file;
    const auto graph = makeGraph();
    customizer::files::writeGraph(file.path, graph, 0xDEADBEEFU);

    customizer::MultiLevelEdgeBasedGraph loaded;
    std::uint32_t checksum = 0;
    customizer::files::readGraph(file.path, loaded, checksum);

    BOOST_CHECK_EQUAL(checksum, 0xDEADBEEFU);
    BOOST_CHECK(loaded.GetIsochroneGraph().empty());
}

BOOST_AUTO_TEST_CASE(rejects_an_mld_graph_with_partial_isochrone_data)
{
    TemporaryFile file;
    {
        storage::tar::FileWriter writer{file.path, storage::tar::FileWriter::GenerateFingerprint};
        writeGraphWithChecksum(writer, makeGraph());
        storage::serialization::write(
            writer, "/mld/multilevelgraph/isochrone/forward_offsets", std::vector<EdgeID>{0, 1, 1});
    }

    customizer::MultiLevelEdgeBasedGraph loaded;
    std::uint32_t checksum = 0;
    BOOST_CHECK_THROW(customizer::files::readGraph(file.path, loaded, checksum), util::exception);
}

BOOST_AUTO_TEST_CASE(rejects_an_mld_graph_with_malformed_isochrone_data)
{
    TemporaryFile file;
    {
        storage::tar::FileWriter writer{file.path, storage::tar::FileWriter::GenerateFingerprint};
        writeGraphWithChecksum(writer, makeGraph());
        storage::serialization::write(
            writer, "/mld/multilevelgraph/isochrone/forward_offsets", std::vector<EdgeID>{0, 2, 2});
        storage::serialization::write(writer,
                                      "/mld/multilevelgraph/isochrone/forward_arcs",
                                      std::vector<engine::isochrone::DurationGraphArc>{{1, {15}}});
        storage::serialization::write(
            writer, "/mld/multilevelgraph/isochrone/reverse_offsets", std::vector<EdgeID>{0, 0, 1});
        storage::serialization::write(writer,
                                      "/mld/multilevelgraph/isochrone/reverse_arcs",
                                      std::vector<engine::isochrone::DurationGraphArc>{{0, {15}}});
    }

    customizer::MultiLevelEdgeBasedGraph loaded;
    std::uint32_t checksum = 0;
    BOOST_CHECK_THROW(customizer::files::readGraph(file.path, loaded, checksum), util::exception);
}

BOOST_AUTO_TEST_CASE(parallel_transitions_preserve_the_shortest_duration_not_the_route_edge)
{
    // MLD normally coalesces these same-endpoint transitions by routing weight. The isochrone
    // graph must retain the shortest effective duration independently of that selection.
    const std::vector<extractor::IsochroneTransition> transitions = {{0, 1, 0}, {0, 1, 1}};
    const auto isochrone_graph = engine::isochrone::buildDurationGraph(
        2, transitions, {{100}, {0}}, {{10}, {0}}, {{0}, {0}}, {{50}, {5}});

    BOOST_REQUIRE_EQUAL(isochrone_graph.forward_arcs.size(), 1);
    BOOST_CHECK_EQUAL(isochrone_graph.forward_arcs.front().node, 1);
    BOOST_CHECK_EQUAL(isochrone_graph.forward_arcs.front().duration, EdgeDuration{15});

    const auto graph = makeGraph(isochrone_graph);
    BOOST_REQUIRE_EQUAL(graph.GetNumberOfNodes(), 2);
    BOOST_REQUIRE(engine::isochrone::isValidDurationGraph(graph.GetIsochroneGraph(), 2));

    TemporaryFile file;
    BOOST_REQUIRE_NO_THROW(customizer::files::writeGraph(file.path, graph, 0xDEADBEEFU));

    customizer::MultiLevelEdgeBasedGraph loaded;
    std::uint32_t checksum = 0;
    BOOST_REQUIRE_NO_THROW(customizer::files::readGraph(file.path, loaded, checksum));
    const auto &loaded_isochrone_graph = loaded.GetIsochroneGraph();
    BOOST_REQUIRE_EQUAL(loaded_isochrone_graph.forward_arcs.size(), 1);
    BOOST_CHECK_EQUAL(loaded_isochrone_graph.forward_arcs.front().node, 1);
    BOOST_CHECK_EQUAL(loaded_isochrone_graph.forward_arcs.front().duration, EdgeDuration{15});
}

BOOST_AUTO_TEST_SUITE_END()
