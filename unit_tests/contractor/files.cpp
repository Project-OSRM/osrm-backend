#include "contractor/files.hpp"
#include "contractor/graph_contractor_adaptors.hpp"

#include "engine/isochrone/duration_graph.hpp"

#include "util/exception.hpp"

#include "../common/range_tools.hpp"
#include "../common/temporary_file.hpp"
#include "helper.hpp"

#include <boost/test/unit_test.hpp>

#include <cstddef>
#include <vector>

namespace
{
using ::EdgeDuration;
using osrm::from_alias;

osrm::engine::isochrone::DurationGraph makeIsochroneGraph()
{
    return {{0, 2, 4, 4, 5, 6, 7},
            {{1, EdgeWeight{10}, EdgeDuration{10}},
             {5, EdgeWeight{20}, EdgeDuration{20}},
             {3, EdgeWeight{30}, EdgeDuration{30}},
             {4, EdgeWeight{40}, EdgeDuration{40}},
             {1, EdgeWeight{50}, EdgeDuration{50}},
             {3, EdgeWeight{60}, EdgeDuration{60}},
             {1, EdgeWeight{70}, EdgeDuration{70}}},
            {0, 0, 3, 3, 5, 6, 7},
            {{0, EdgeWeight{10}, EdgeDuration{10}},
             {3, EdgeWeight{50}, EdgeDuration{50}},
             {5, EdgeWeight{70}, EdgeDuration{70}},
             {1, EdgeWeight{30}, EdgeDuration{30}},
             {4, EdgeWeight{60}, EdgeDuration{60}},
             {1, EdgeWeight{40}, EdgeDuration{40}},
             {0, EdgeWeight{20}, EdgeDuration{20}}}};
}

void checkIsochroneGraphsEqual(const osrm::engine::isochrone::DurationGraph &actual,
                               const osrm::engine::isochrone::DurationGraph &expected)
{
    CHECK_EQUAL_COLLECTIONS(actual.forward_offsets, expected.forward_offsets);
    CHECK_EQUAL_COLLECTIONS(actual.reverse_offsets, expected.reverse_offsets);
    BOOST_REQUIRE_EQUAL(actual.forward_arcs.size(), expected.forward_arcs.size());
    BOOST_REQUIRE_EQUAL(actual.reverse_arcs.size(), expected.reverse_arcs.size());
    for (std::size_t index = 0; index < expected.forward_arcs.size(); ++index)
    {
        BOOST_CHECK_EQUAL(actual.forward_arcs[index].node, expected.forward_arcs[index].node);
        BOOST_CHECK_EQUAL(from_alias<int>(actual.forward_arcs[index].weight),
                          from_alias<int>(expected.forward_arcs[index].weight));
        BOOST_CHECK_EQUAL(from_alias<int>(actual.forward_arcs[index].duration),
                          from_alias<int>(expected.forward_arcs[index].duration));
        BOOST_CHECK_EQUAL(actual.reverse_arcs[index].node, expected.reverse_arcs[index].node);
        BOOST_CHECK_EQUAL(from_alias<int>(actual.reverse_arcs[index].weight),
                          from_alias<int>(expected.reverse_arcs[index].weight));
        BOOST_CHECK_EQUAL(from_alias<int>(actual.reverse_arcs[index].duration),
                          from_alias<int>(expected.reverse_arcs[index].duration));
    }
}
} // namespace

BOOST_AUTO_TEST_SUITE(tar)

using namespace osrm;
using namespace osrm::contractor;
using namespace osrm::unit_test;

BOOST_AUTO_TEST_CASE(read_write_hsgr)
{
    auto reference_connectivity_checksum = 0xDEADBEEF;
    std::vector<TestEdge> edges = {TestEdge{0, 1, 3},
                                   TestEdge{0, 5, 1},
                                   TestEdge{1, 3, 3},
                                   TestEdge{1, 4, 1},
                                   TestEdge{3, 1, 1},
                                   TestEdge{4, 3, 1},
                                   TestEdge{5, 1, 1}};
    auto reference_graph = QueryGraph{6, toEdges<QueryEdge>(makeGraph(edges))};
    auto reference_isochrone_graph = makeIsochroneGraph();
    std::vector<std::vector<bool>> reference_filters = {
        {false, false, true, true, false, false, true},
        {true, false, true, false, true, false, true},
        {false, false, false, false, false, false, false},
        {true, true, true, true, true, true, true},
    };

    std::unordered_map<std::string, ContractedMetric> reference_metrics = {
        {"duration",
         {std::move(reference_graph),
          std::move(reference_filters),
          std::move(reference_isochrone_graph)}}};

    TemporaryFile tmp{TEST_DATA_DIR "/read_write_hsgr_test.osrm.hsgr"};
    contractor::files::writeGraph(tmp.path, reference_metrics, reference_connectivity_checksum);

    unsigned connectivity_checksum;

    std::unordered_map<std::string, ContractedMetric> metrics = {{"duration", {}}};
    contractor::files::readGraph(tmp.path, metrics, connectivity_checksum);

    BOOST_CHECK_EQUAL(connectivity_checksum, reference_connectivity_checksum);
    BOOST_CHECK_EQUAL(metrics["duration"].edge_filter.size(),
                      reference_metrics["duration"].edge_filter.size());
    CHECK_EQUAL_COLLECTIONS(metrics["duration"].edge_filter[0],
                            reference_metrics["duration"].edge_filter[0]);
    CHECK_EQUAL_COLLECTIONS(metrics["duration"].edge_filter[1],
                            reference_metrics["duration"].edge_filter[1]);
    CHECK_EQUAL_COLLECTIONS(metrics["duration"].edge_filter[2],
                            reference_metrics["duration"].edge_filter[2]);
    CHECK_EQUAL_COLLECTIONS(metrics["duration"].edge_filter[3],
                            reference_metrics["duration"].edge_filter[3]);
    checkIsochroneGraphsEqual(metrics["duration"].isochrone_graph,
                              reference_metrics["duration"].isochrone_graph);
}

BOOST_AUTO_TEST_CASE(read_legacy_hsgr_without_isochrone_index)
{
    constexpr auto reference_connectivity_checksum = 0xDEADBEEFU;
    std::vector<TestEdge> edges = {TestEdge{0, 1, 3}, TestEdge{1, 2, 5}, TestEdge{2, 2, 7}};
    auto reference_graph = QueryGraph{3, toEdges<QueryEdge>(makeGraph(edges))};
    std::vector<std::vector<bool>> reference_filters = {{true, true, true}};

    TemporaryFile tmp{TEST_DATA_DIR "/legacy_hsgr_without_isochrone.osrm.hsgr"};
    {
        storage::tar::FileWriter writer(tmp.path, storage::tar::FileWriter::GenerateFingerprint);
        writer.WriteElementCount64("/ch/connectivity_checksum", 1);
        writer.WriteFrom("/ch/connectivity_checksum", reference_connectivity_checksum);
        util::serialization::write(
            writer, "/ch/metrics/duration/contracted_graph", reference_graph);
        writer.WriteElementCount64("/ch/metrics/duration/exclude", reference_filters.size());
        storage::serialization::write(
            writer, "/ch/metrics/duration/exclude/0/edge_filter", reference_filters.front());
    }

    unsigned connectivity_checksum;
    std::unordered_map<std::string, ContractedMetric> metrics = {{"duration", {}}};
    contractor::files::readGraph(tmp.path, metrics, connectivity_checksum);

    BOOST_CHECK_EQUAL(connectivity_checksum, reference_connectivity_checksum);
    BOOST_CHECK(metrics["duration"].isochrone_graph.empty());
}

BOOST_AUTO_TEST_CASE(write_omits_an_empty_isochrone_index)
{
    constexpr auto reference_connectivity_checksum = 0xDEADBEEFU;
    std::vector<TestEdge> edges = {TestEdge{0, 1, 3}, TestEdge{1, 2, 5}, TestEdge{2, 2, 7}};
    auto graph = QueryGraph{3, toEdges<QueryEdge>(makeGraph(edges))};
    std::vector<std::vector<bool>> filters = {{true, true, true}};
    std::unordered_map<std::string, ContractedMetric> reference_metrics = {
        {"duration", {std::move(graph), std::move(filters), {}}}};

    TemporaryFile tmp{TEST_DATA_DIR "/hsgr_without_isochrone_index.osrm.hsgr"};
    contractor::files::writeGraph(tmp.path, reference_metrics, reference_connectivity_checksum);

    {
        storage::tar::FileReader reader{tmp.path, storage::tar::FileReader::VerifyFingerprint};
        for (const auto *name :
             {"forward_offsets", "forward_arcs", "reverse_offsets", "reverse_arcs"})
        {
            const auto block = std::string("/ch/metrics/duration/isochrone/") + name;
            BOOST_CHECK(!reader.HasEntry(block));
            BOOST_CHECK(!reader.HasEntry(block + ".meta"));
        }
    }

    unsigned connectivity_checksum;
    std::unordered_map<std::string, ContractedMetric> metrics = {{"duration", {}}};
    contractor::files::readGraph(tmp.path, metrics, connectivity_checksum);
    BOOST_CHECK_EQUAL(connectivity_checksum, reference_connectivity_checksum);
    BOOST_CHECK(metrics["duration"].isochrone_graph.empty());
}

BOOST_AUTO_TEST_CASE(rejects_hsgr_with_invalid_isochrone_index)
{
    constexpr auto connectivity_checksum = 0xDEADBEEFU;
    std::vector<TestEdge> edges = {TestEdge{0, 1, 3}, TestEdge{1, 2, 5}};
    auto graph = QueryGraph{3, toEdges<QueryEdge>(makeGraph(edges))};
    auto isochrone_graph = makeIsochroneGraph();
    ++isochrone_graph.forward_offsets.back();
    std::vector<std::vector<bool>> filters = {{true, true}};
    std::unordered_map<std::string, ContractedMetric> reference_metrics = {
        {"duration", {std::move(graph), std::move(filters), std::move(isochrone_graph)}}};

    TemporaryFile tmp{TEST_DATA_DIR "/invalid_isochrone_index.osrm.hsgr"};
    BOOST_CHECK_THROW(
        contractor::files::writeGraph(tmp.path, reference_metrics, connectivity_checksum),
        util::exception);
}

BOOST_AUTO_TEST_SUITE_END()
