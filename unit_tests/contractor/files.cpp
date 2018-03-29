#include "contractor/files.hpp"
#include "contractor/graph_contractor_adaptors.hpp"

#include "../common/range_tools.hpp"
#include "../common/temporary_file.hpp"
#include "helper.hpp"

#include <boost/test/unit_test.hpp>

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
    std::vector<std::vector<bool>> reference_filters = {
        {false, false, true, true, false, false, true},
        {true, false, true, false, true, false, true},
        {false, false, false, false, false, false, false},
        {true, true, true, true, true, true, true},
    };

    std::unordered_map<std::string, ContractedMetric> reference_metrics = {
        {"duration", {std::move(reference_graph), std::move(reference_filters)}}};

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
}

BOOST_AUTO_TEST_SUITE_END()
