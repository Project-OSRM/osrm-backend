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
    auto reference_checksum = 0xFF00FF00;
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

    TemporaryFile tmp{TEST_DATA_DIR "/read_write_hsgr_test.osrm.hsgr"};
    contractor::files::writeGraph(tmp.path,
                                  reference_checksum,
                                  reference_graph,
                                  reference_filters,
                                  reference_connectivity_checksum);

    unsigned checksum;
    unsigned connectivity_checksum;
    QueryGraph graph;
    std::vector<std::vector<bool>> filters;
    contractor::files::readGraph(tmp.path, checksum, graph, filters, connectivity_checksum);

    BOOST_CHECK_EQUAL(checksum, reference_checksum);
    BOOST_CHECK_EQUAL(connectivity_checksum, reference_connectivity_checksum);
    BOOST_CHECK_EQUAL(filters.size(), reference_filters.size());
    CHECK_EQUAL_COLLECTIONS(filters[0], reference_filters[0]);
    CHECK_EQUAL_COLLECTIONS(filters[1], reference_filters[1]);
    CHECK_EQUAL_COLLECTIONS(filters[2], reference_filters[2]);
    CHECK_EQUAL_COLLECTIONS(filters[3], reference_filters[3]);
}

BOOST_AUTO_TEST_SUITE_END()
