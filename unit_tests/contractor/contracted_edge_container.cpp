#include "contractor/contracted_edge_container.hpp"

#include "../common/range_tools.hpp"

#include <boost/test/unit_test.hpp>

using namespace osrm;
using namespace osrm::contractor;

namespace osrm
{
namespace contractor
{

bool operator!=(const QueryEdge &lhs, const QueryEdge &rhs) { return !(lhs == rhs); }

std::ostream &operator<<(std::ostream &out, const QueryEdge::EdgeData &data)
{
    out << "{" << data.turn_id << ", " << data.shortcut << ", " << data.duration << ", "
        << data.distance << ", " << data.weight << ", " << data.forward << ", " << data.backward
        << "}";
    return out;
}

std::ostream &operator<<(std::ostream &out, const QueryEdge &edge)
{
    out << "{" << edge.source << ", " << edge.target << ", " << edge.data << "}";
    return out;
}
}
}

BOOST_AUTO_TEST_SUITE(contracted_edge_container)

BOOST_AUTO_TEST_CASE(merge_edge_of_multiple_graph)
{
    ContractedEdgeContainer container;

    std::vector<QueryEdge> edges;
    edges.push_back(QueryEdge{0, 1, {1, false, 3, 3, 6, true, false}});
    edges.push_back(QueryEdge{1, 2, {2, false, 3, 3, 6, true, false}});
    edges.push_back(QueryEdge{2, 0, {3, false, 3, 3, 6, false, true}});
    edges.push_back(QueryEdge{2, 1, {4, false, 3, 3, 6, false, true}});
    container.Insert(edges);

    edges.clear();
    edges.push_back(QueryEdge{0, 1, {1, false, 3, 3, 6, true, false}});
    edges.push_back(QueryEdge{1, 2, {2, false, 3, 3, 6, true, false}});
    edges.push_back(QueryEdge{2, 0, {3, false, 12, 12, 24, false, true}});
    edges.push_back(QueryEdge{2, 1, {4, false, 12, 12, 24, false, true}});
    container.Merge(edges);

    edges.clear();
    edges.push_back(QueryEdge{1, 4, {5, false, 3, 3, 6, true, false}});
    container.Merge(edges);

    std::vector<QueryEdge> reference_edges;
    reference_edges.push_back(QueryEdge{0, 1, {1, false, 3, 3, 6, true, false}});
    reference_edges.push_back(QueryEdge{1, 2, {2, false, 3, 3, 6, true, false}});
    reference_edges.push_back(QueryEdge{1, 4, {5, false, 3, 3, 6, true, false}});
    reference_edges.push_back(QueryEdge{2, 0, {3, false, 3, 3, 6, false, true}});
    reference_edges.push_back(QueryEdge{2, 0, {3, false, 12, 12, 24, false, true}});
    reference_edges.push_back(QueryEdge{2, 1, {4, false, 3, 3, 6, false, true}});
    reference_edges.push_back(QueryEdge{2, 1, {4, false, 12, 12, 24, false, true}});
    CHECK_EQUAL_COLLECTIONS(container.edges, reference_edges);

    auto filters = container.MakeEdgeFilters();
    BOOST_CHECK_EQUAL(filters.size(), 2);

    REQUIRE_SIZE_RANGE(filters[0], 7);
    CHECK_EQUAL_RANGE(filters[0], true, true, false, true, true, true, true);

    REQUIRE_SIZE_RANGE(filters[1], 7);
    CHECK_EQUAL_RANGE(filters[1], true, true, true, true, false, true, false);
}

BOOST_AUTO_TEST_CASE(merge_edge_of_multiple_disjoint_graph)
{
    ContractedEdgeContainer container;

    std::vector<QueryEdge> edges;
    edges.push_back(QueryEdge{0, 1, {1, false, 3, 3, 6, true, false}});
    edges.push_back(QueryEdge{1, 2, {2, false, 3, 3, 6, true, false}});
    edges.push_back(QueryEdge{2, 0, {3, false, 12, 12, 24, false, true}});
    edges.push_back(QueryEdge{2, 1, {4, false, 12, 12, 24, false, true}});
    container.Merge(edges);

    edges.clear();
    edges.push_back(QueryEdge{1, 4, {5, false, 3, 3, 6, true, false}});
    container.Merge(edges);

    std::vector<QueryEdge> reference_edges;
    reference_edges.push_back(QueryEdge{0, 1, {1, false, 3, 3, 6, true, false}});
    reference_edges.push_back(QueryEdge{1, 2, {2, false, 3, 3, 6, true, false}});
    reference_edges.push_back(QueryEdge{1, 4, {5, false, 3, 3, 6, true, false}});
    reference_edges.push_back(QueryEdge{2, 0, {3, false, 12, 12, 24, false, true}});
    reference_edges.push_back(QueryEdge{2, 1, {4, false, 12, 12, 24, false, true}});
    CHECK_EQUAL_COLLECTIONS(container.edges, reference_edges);

    auto filters = container.MakeEdgeFilters();
    BOOST_CHECK_EQUAL(filters.size(), 2);

    REQUIRE_SIZE_RANGE(filters[0], 5);
    CHECK_EQUAL_RANGE(filters[0], true, true, false, true, true);

    REQUIRE_SIZE_RANGE(filters[1], 5);
    CHECK_EQUAL_RANGE(filters[1], false, false, true, false, false);
}

BOOST_AUTO_TEST_SUITE_END()
