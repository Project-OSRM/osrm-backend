#include "extractor/intersection/coordinate_extractor.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "util/coordinate.hpp"
#include "util/node_based_graph.hpp"

#include <boost/test/unit_test.hpp>

#include <vector>

BOOST_AUTO_TEST_SUITE(coordinate_extractor)

BOOST_AUTO_TEST_CASE(reads_reversed_geometry_from_the_far_endpoint)
{
    using osrm::extractor::CompressedEdgeContainer;
    using osrm::extractor::intersection::CoordinateExtractor;
    using osrm::util::Coordinate;
    using osrm::util::FixedLatitude;
    using osrm::util::FixedLongitude;
    using osrm::util::NodeBasedDynamicGraph;

    const std::vector<NodeBasedDynamicGraph::InputEdge> edges{{0, 2}};
    const NodeBasedDynamicGraph graph(3, edges);
    CompressedEdgeContainer geometry;
    geometry.CompressEdge(0, 1, 1, 2, {1}, {1}, {11}, {11});

    const Coordinate first{FixedLongitude{0}, FixedLatitude{0}};
    const Coordinate middle{FixedLongitude{200}, FixedLatitude{0}};
    const Coordinate last{FixedLongitude{400}, FixedLatitude{0}};
    const std::vector<Coordinate> coordinates{first, middle, last};
    const CoordinateExtractor extractor(graph, geometry, coordinates);
    BOOST_CHECK(extractor.GetCoordinateCloseToTurn(0, 0, true, 2) == middle);
    const std::vector<Coordinate> reversed{last, middle, first};
    BOOST_CHECK(extractor.GetCoordinatesAlongRoad(0, 0, true, 2) == reversed);

    const std::vector<Coordinate> repeated_coordinates{first, last, last};
    const CoordinateExtractor repeated(graph, geometry, repeated_coordinates);
    BOOST_CHECK(repeated.GetCoordinateCloseToTurn(0, 0, true, 2) == first);
    const std::vector<Coordinate> distinct{last, first};
    BOOST_CHECK(repeated.GetCoordinatesAlongRoad(0, 0, true, 2) == distinct);
}

BOOST_AUTO_TEST_SUITE_END()
