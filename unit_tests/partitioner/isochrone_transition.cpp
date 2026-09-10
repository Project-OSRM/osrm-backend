#include "extractor/isochrone_transition.hpp"
#include "extractor/files.hpp"

#include "partitioner/edge_based_graph_reader.hpp"
#include "partitioner/renumber.hpp"

#include "storage/tar.hpp"
#include "util/exception.hpp"

#include "../common/temporary_file.hpp"

#include <boost/test/unit_test.hpp>

#include <vector>

using namespace osrm;
using namespace osrm::extractor;
using namespace osrm::partitioner;

namespace
{
EdgeBasedEdge makeEdge(const NodeID source,
                       const NodeID target,
                       const NodeID turn_id,
                       const EdgeWeight weight,
                       const bool forward,
                       const bool backward)
{ return {source, target, turn_id, weight, EdgeDuration{10}, EdgeDistance{1}, forward, backward}; }

void checkTransition(const IsochroneTransition &transition,
                     const NodeID source,
                     const NodeID target,
                     const NodeID turn_id)
{
    BOOST_CHECK_EQUAL(transition.source, source);
    BOOST_CHECK_EQUAL(transition.target, target);
    BOOST_CHECK_EQUAL(transition.turn_id, turn_id);
}

void checkEqualTransitions(const std::vector<IsochroneTransition> &actual,
                           const std::vector<IsochroneTransition> &expected)
{
    BOOST_REQUIRE_EQUAL(actual.size(), expected.size());
    for (const auto index : util::irange<std::size_t>(0, expected.size()))
        checkTransition(
            actual[index], expected[index].source, expected[index].target, expected[index].turn_id);
}
} // namespace

BOOST_AUTO_TEST_SUITE(isochrone_transition_tests)

BOOST_AUTO_TEST_CASE(make_transitions_preserves_parallel_and_opposite_directions)
{
    const std::vector<EdgeBasedEdge> edges = {
        makeEdge(0, 1, 7, EdgeWeight{10}, true, true),
        makeEdge(0, 1, 8, EdgeWeight{20}, true, false),
        makeEdge(0, 1, 9, EdgeWeight{30}, false, true),
        makeEdge(1, 2, 10, INVALID_EDGE_WEIGHT, true, true),
        makeEdge(2, 3, 11, EdgeWeight{40}, false, false),
        makeEdge(0, 1, 7, EdgeWeight{10}, true, true),
    };

    const auto transitions = makeIsochroneTransitions(edges);

    BOOST_REQUIRE_EQUAL(transitions.size(), 6);
    checkTransition(transitions[0], 0, 1, 7);
    checkTransition(transitions[1], 0, 1, 8);
    checkTransition(transitions[2], 1, 0, 7);
    checkTransition(transitions[3], 1, 0, 9);
    checkTransition(transitions[4], 1, 2, 10);
    checkTransition(transitions[5], 2, 1, 10);
}

BOOST_AUTO_TEST_CASE(edge_based_graph_writes_optional_transitions_only_when_requested)
{
    const std::vector<EdgeBasedEdge> edges = {makeEdge(0, 1, 0, EdgeWeight{10}, true, false)};
    const auto transitions = makeIsochroneTransitions(edges);

    TemporaryFile without_transitions;
    extractor::files::writeEdgeBasedGraph(without_transitions.path, 2, edges, 0xDEADBEEFU);

    {
        storage::tar::FileReader reader{without_transitions.path,
                                        storage::tar::FileReader::VerifyFingerprint};
        BOOST_CHECK(!reader.HasEntry("/common/isochrone_transitions"));
        BOOST_CHECK(!reader.HasEntry("/common/isochrone_transitions.meta"));
    }

    std::vector<IsochroneTransition> actual = {{0, 1, 0}};
    BOOST_CHECK(!extractor::files::readIsochroneTransitions(without_transitions.path, actual));
    BOOST_CHECK(actual.empty());

    TemporaryFile with_transitions;
    extractor::files::writeEdgeBasedGraph(
        with_transitions.path, 2, edges, 0xDEADBEEFU, transitions);

    {
        storage::tar::FileReader reader{with_transitions.path,
                                        storage::tar::FileReader::VerifyFingerprint};
        BOOST_CHECK(reader.HasEntry("/common/isochrone_transitions"));
        BOOST_CHECK(reader.HasEntry("/common/isochrone_transitions.meta"));
    }

    BOOST_REQUIRE(extractor::files::readIsochroneTransitions(with_transitions.path, actual));
    checkEqualTransitions(actual, transitions);
}

BOOST_AUTO_TEST_CASE(read_transitions_rejects_partial_optional_entries)
{
    const IsochroneTransition transition{0, 1, 0};

    TemporaryFile without_data;
    {
        storage::tar::FileWriter writer{without_data.path,
                                        storage::tar::FileWriter::GenerateFingerprint};
        writer.WriteElementCount64("/common/isochrone_transitions", 1);
    }
    std::vector<IsochroneTransition> actual;
    BOOST_CHECK_THROW(extractor::files::readIsochroneTransitions(without_data.path, actual),
                      util::exception);

    TemporaryFile without_count;
    {
        storage::tar::FileWriter writer{without_count.path,
                                        storage::tar::FileWriter::GenerateFingerprint};
        writer.WriteFrom("/common/isochrone_transitions", transition);
    }
    BOOST_CHECK_THROW(extractor::files::readIsochroneTransitions(without_count.path, actual),
                      util::exception);
}

BOOST_AUTO_TEST_CASE(read_transitions_rejects_unsorted_or_duplicate_entries)
{
    const std::vector<EdgeBasedEdge> edges = {
        makeEdge(0, 1, 0, EdgeWeight{10}, true, false),
        makeEdge(1, 0, 1, EdgeWeight{10}, true, false),
    };
    std::vector<IsochroneTransition> actual;

    TemporaryFile unsorted;
    const std::vector<IsochroneTransition> unsorted_transitions = {{1, 0, 1}, {0, 1, 0}};
    extractor::files::writeEdgeBasedGraph(
        unsorted.path, 2, edges, 0xDEADBEEFU, unsorted_transitions);
    BOOST_CHECK_THROW(extractor::files::readIsochroneTransitions(unsorted.path, actual),
                      util::exception);

    TemporaryFile duplicate;
    const std::vector<IsochroneTransition> duplicate_transitions = {{0, 1, 0}, {0, 1, 0}};
    extractor::files::writeEdgeBasedGraph(
        duplicate.path, 2, edges, 0xDEADBEEFU, duplicate_transitions);
    BOOST_CHECK_THROW(extractor::files::readIsochroneTransitions(duplicate.path, actual),
                      util::exception);
}

BOOST_AUTO_TEST_CASE(repeated_partitioning_reuses_and_renumbers_preserved_transitions)
{
    // The two forward raw edges have the same endpoints. The MLD graph retains only the lower
    // weight edge, so reconstructing the transition list from a re-partitioned .ebg would lose
    // turn_id 1.
    const std::vector<EdgeBasedEdge> raw_edges = {
        makeEdge(0, 1, 0, EdgeWeight{10}, true, false),
        makeEdge(0, 1, 1, EdgeWeight{20}, true, false),
    };
    const auto preserved_transitions = makeIsochroneTransitions(raw_edges);

    TemporaryFile first_partition;
    extractor::files::writeEdgeBasedGraph(first_partition.path, 2, raw_edges, 0xDEADBEEFU);
    auto graph = LoadEdgeBasedGraph(first_partition.path);
    const auto collapsed_edges = graphToEdges(graph);
    BOOST_REQUIRE_EQUAL(collapsed_edges.size(), 1);
    BOOST_REQUIRE_EQUAL(makeIsochroneTransitions(collapsed_edges).size(), 1);

    TemporaryFile second_partition;
    extractor::files::writeEdgeBasedGraph(
        second_partition.path, 2, collapsed_edges, 0xDEADBEEFU, preserved_transitions);

    std::vector<IsochroneTransition> reloaded_transitions;
    auto reloaded_graph = LoadEdgeBasedGraph(second_partition.path, reloaded_transitions);
    checkEqualTransitions(reloaded_transitions, preserved_transitions);

    const std::vector<std::uint32_t> permutation = {1, 0};
    renumber(reloaded_graph, permutation);
    renumber(reloaded_transitions, permutation);

    TemporaryFile third_partition;
    extractor::files::writeEdgeBasedGraph(third_partition.path,
                                          reloaded_graph.GetNumberOfNodes(),
                                          graphToEdges(reloaded_graph),
                                          reloaded_graph.connectivity_checksum,
                                          reloaded_transitions);

    std::vector<IsochroneTransition> twice_reloaded_transitions;
    LoadEdgeBasedGraph(third_partition.path, twice_reloaded_transitions);
    BOOST_REQUIRE_EQUAL(twice_reloaded_transitions.size(), 2);
    checkTransition(twice_reloaded_transitions[0], 1, 0, 0);
    checkTransition(twice_reloaded_transitions[1], 1, 0, 1);
}

BOOST_AUTO_TEST_CASE(already_partitioned_graph_requires_preserved_transitions)
{
    const std::vector<EdgeBasedEdge> collapsed_edges = {
        makeEdge(0, 1, 0, EdgeWeight{10}, true, false)};
    TemporaryFile graph_without_transitions;
    extractor::files::writeEdgeBasedGraph(
        graph_without_transitions.path, 2, collapsed_edges, 0xDEADBEEFU);

    std::vector<IsochroneTransition> transitions;
    BOOST_CHECK_THROW(LoadEdgeBasedGraph(graph_without_transitions.path, transitions, true),
                      util::exception);

    BOOST_CHECK_NO_THROW(LoadEdgeBasedGraph(graph_without_transitions.path, transitions, false));
    BOOST_REQUIRE_EQUAL(transitions.size(), 1);
    checkTransition(transitions.front(), 0, 1, 0);
}

BOOST_AUTO_TEST_CASE(transitions_require_valid_endpoints)
{
    const std::vector<IsochroneTransition> valid = {{0, 1, 8}, {1, 0, 9}};
    BOOST_CHECK(isValidIsochroneTransitions(valid, 2));

    const std::vector<IsochroneTransition> invalid_source = {{2, 1, 8}};
    BOOST_CHECK(!isValidIsochroneTransitions(invalid_source, 2));

    const std::vector<IsochroneTransition> invalid_target = {{0, 2, 8}};
    BOOST_CHECK(!isValidIsochroneTransitions(invalid_target, 2));

    const std::vector<IsochroneTransition> invalid_turn_id = {{0, 1, SPECIAL_NODEID}};
    BOOST_CHECK(!isValidIsochroneTransitions(invalid_turn_id, 2));
}

BOOST_AUTO_TEST_SUITE_END()
