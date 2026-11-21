#include "extractor/compressed_edge_container.hpp"
#include "extractor/geometry_memory.hpp"
#include "util/typedefs.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <array>
#include <vector>

BOOST_AUTO_TEST_SUITE(compressed_edge_container)

using osrm::extractor::CompressedEdgeContainer;

BOOST_AUTO_TEST_CASE(geometry_memory_resources_compare_by_identity)
{
    const osrm::extractor::GeometryMemory first{};
    const osrm::extractor::GeometryMemory second{};
    BOOST_CHECK(first.is_equal(first));
    BOOST_CHECK(!first.is_equal(second));
}

BOOST_AUTO_TEST_CASE(long_road_test)
{
    //   0   1    2    3
    // 0---1----2----3----4
    CompressedEdgeContainer container;

    // compress 0---1---2 to 0---2
    container.CompressEdge(0, 1, 1, 2, {1}, {1}, {11}, {11});
    BOOST_CHECK(container.HasEntryForID(0));
    BOOST_CHECK(!container.HasEntryForID(1));
    BOOST_CHECK(!container.HasEntryForID(2));
    BOOST_CHECK(!container.HasEntryForID(3));
    BOOST_CHECK_EQUAL(container.GetFirstEdgeTargetID(0), 1);
    BOOST_CHECK_EQUAL(container.GetLastEdgeSourceID(0), 1);

    // compress 2---3---4 to 2---4
    container.CompressEdge(2, 3, 3, 4, {1}, {1}, {11}, {11});
    BOOST_CHECK(container.HasEntryForID(0));
    BOOST_CHECK(!container.HasEntryForID(1));
    BOOST_CHECK(container.HasEntryForID(2));
    BOOST_CHECK(!container.HasEntryForID(3));
    BOOST_CHECK_EQUAL(container.GetFirstEdgeTargetID(2), 3);
    BOOST_CHECK_EQUAL(container.GetLastEdgeSourceID(2), 3);

    // compress 0---2---4 to 0---4
    container.CompressEdge(0, 2, 2, 4, {2}, {2}, {22}, {22});
    BOOST_CHECK(container.HasEntryForID(0));
    BOOST_CHECK(!container.HasEntryForID(1));
    BOOST_CHECK(!container.HasEntryForID(2));
    BOOST_CHECK(!container.HasEntryForID(3));
    BOOST_CHECK_EQUAL(container.GetFirstEdgeTargetID(0), 1);
    BOOST_CHECK_EQUAL(container.GetLastEdgeSourceID(0), 3);
}

BOOST_AUTO_TEST_CASE(t_crossing)
{
    //   0   1   2   3
    // 0---1---2---3---4
    //         | 4
    //         5
    //         | 5
    //         6
    CompressedEdgeContainer container;

    // compress 0---1---2 to 0---2
    container.CompressEdge(0, 1, 1, 2, {1}, {1}, {11}, {11});
    BOOST_CHECK(container.HasEntryForID(0));
    BOOST_CHECK(!container.HasEntryForID(1));
    BOOST_CHECK(!container.HasEntryForID(2));
    BOOST_CHECK(!container.HasEntryForID(3));
    BOOST_CHECK(!container.HasEntryForID(4));
    BOOST_CHECK(!container.HasEntryForID(5));
    BOOST_CHECK_EQUAL(container.GetFirstEdgeTargetID(0), 1);
    BOOST_CHECK_EQUAL(container.GetLastEdgeSourceID(0), 1);

    // compress 2---5---6 to 2---6
    container.CompressEdge(4, 5, 5, 6, {1}, {1}, {11}, {11});
    BOOST_CHECK(container.HasEntryForID(0));
    BOOST_CHECK(!container.HasEntryForID(1));
    BOOST_CHECK(!container.HasEntryForID(2));
    BOOST_CHECK(!container.HasEntryForID(3));
    BOOST_CHECK(container.HasEntryForID(4));
    BOOST_CHECK(!container.HasEntryForID(5));
    BOOST_CHECK_EQUAL(container.GetFirstEdgeTargetID(4), 5);
    BOOST_CHECK_EQUAL(container.GetLastEdgeSourceID(4), 5);

    // compress 2---3---4 to 2---4
    container.CompressEdge(2, 3, 3, 4, {1}, {1}, {11}, {11});
    BOOST_CHECK(container.HasEntryForID(0));
    BOOST_CHECK(!container.HasEntryForID(1));
    BOOST_CHECK(container.HasEntryForID(2));
    BOOST_CHECK(!container.HasEntryForID(3));
    BOOST_CHECK(container.HasEntryForID(4));
    BOOST_CHECK(!container.HasEntryForID(5));
    BOOST_CHECK_EQUAL(container.GetFirstEdgeTargetID(2), 3);
    BOOST_CHECK_EQUAL(container.GetLastEdgeSourceID(2), 3);
}

BOOST_AUTO_TEST_CASE(finalizes_directed_geometry_only_when_both_directions_match)
{
    using Graph = osrm::util::NodeBasedDynamicGraph;
    const std::vector<Graph::InputEdge> edges{{0, 2}, {2, 0}};
    const std::array<NodeID, 2> forward{1, 2};
    const std::array<NodeID, 2> reverse{1, 0};

    for (const bool geometry_matches : {true, false})
    {
        CompressedEdgeContainer container;
        container.CompressEdge(0, 2, 1, 2, {1}, {1}, {11}, {11});
        container.CompressEdge(1, 3, 1, 0, {1}, {1}, {11}, {11});

        Graph graph(3, edges);
        const auto geometry = container.ZipEdges(graph, 0, 1);
        graph.GetEdgeData(0).geometry_id = {geometry, true};
        graph.GetEdgeData(1).geometry_id = {geometry, !geometry_matches};
        BOOST_CHECK_EQUAL(container.finalizeGeometry(graph), geometry_matches);

        BOOST_CHECK_EQUAL(container.HasEntryForID(0), !geometry_matches);
        BOOST_CHECK(std::ranges::equal(
            container.getGeometryNodes(0, graph.GetEdgeData(0).geometry_id), forward));
        BOOST_CHECK(std::ranges::equal(
            container.getGeometryNodes(1, graph.GetEdgeData(1).geometry_id), reverse));
    }
}

BOOST_AUTO_TEST_SUITE_END()
