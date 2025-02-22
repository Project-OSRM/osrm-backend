#include "partitioner/bisection_graph.hpp"
#include "partitioner/graph_generator.hpp"

#include "util/debug.hpp"

#include <algorithm>
#include <random>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace osrm::partitioner;
using namespace osrm::util;

BOOST_AUTO_TEST_SUITE(bisection_graph)

BOOST_AUTO_TEST_CASE(access_nodes)
{
    // 40 entries of left/right edges
    double step_size = 0.01;
    int rows = 10;
    int cols = 4;
    const auto coordinates = makeGridCoordinates(rows, cols, step_size, 0, 0);

    std::vector<EdgeWithSomeAdditionalData> grid_edges;
    auto graph = makeBisectionGraph(coordinates, adaptToBisectionEdge(std::move(grid_edges)));

    const auto to_row = [cols](const NodeID nid) { return nid / cols; };
    const auto to_col = [cols](const NodeID nid) { return nid % cols; };

    const auto get_expected = [&](const NodeID id)
    {
        const auto expected_lon = FloatLongitude{to_col(id) * step_size};
        const auto expected_lat = FloatLatitude{to_row(id) * step_size};
        Coordinate compare(expected_lon, expected_lat);
        return compare;
    };

    // check const access
    BOOST_CHECK_EQUAL(graph.NumberOfNodes(), 40);
    {
        NodeID increasing = 0;
        for (const auto &node : graph.Nodes())
        {
            const auto id = graph.GetID(node);
            BOOST_CHECK_EQUAL(id, increasing++);
            BOOST_CHECK_EQUAL(node.coordinate, get_expected(id));
        }
    }
    // non-const access
    {
        NodeID increasing = 0;
        for (auto &node : graph.Nodes())
        {
            const auto id = graph.GetID(node);
            BOOST_CHECK_EQUAL(id, increasing++);
            BOOST_CHECK_EQUAL(node.coordinate, get_expected(id));
        }
    }
    // iterator access
    {
        const auto begin = graph.Begin();
        const auto end = graph.End();
        NodeID increasing = 0;
        for (auto itr = begin; itr != end; ++itr)
        {
            const auto id = static_cast<NodeID>(std::distance(begin, itr));
            BOOST_CHECK_EQUAL(id, increasing++);
            BOOST_CHECK_EQUAL(itr->coordinate, get_expected(id));
        }
    }
    // const iterator access
    {
        const auto begin = graph.CBegin();
        const auto end = graph.CEnd();
        NodeID increasing = 0;
        for (auto itr = begin; itr != end; ++itr)
        {
            const auto id = static_cast<NodeID>(std::distance(begin, itr));
            BOOST_CHECK_EQUAL(id, increasing++);
            BOOST_CHECK_EQUAL(itr->coordinate, get_expected(id));
        }
    }
}

BOOST_AUTO_TEST_CASE(access_edges)
{
    // 40 entries of left/right edges
    double step_size = 0.01;
    int rows = 10;
    int cols = 4;
    const auto coordinates = makeGridCoordinates(rows, cols, step_size, 0, 0);

    auto grid_edges = makeGridEdges(rows, cols, 0);
    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(grid_edges.begin(), grid_edges.end(), rng);
    groupEdgesBySource(grid_edges.begin(), grid_edges.end());

    const auto graph = makeBisectionGraph(coordinates, adaptToBisectionEdge(std::move(grid_edges)));

    const auto to_row = [cols](const NodeID nid) { return nid / cols; };
    const auto to_col = [cols](const NodeID nid) { return nid % cols; };

    BOOST_CHECK_EQUAL(graph.NumberOfNodes(), 40);
    for (const auto &node : graph.Nodes())
    {
        for (const auto &edge : graph.Edges(node))
        {
            BOOST_CHECK(edge.target < graph.NumberOfNodes());
            BOOST_CHECK(std::abs(static_cast<int>(to_row(graph.GetID(node))) -
                                 static_cast<int>(to_row(edge.target))) <= 1);
            BOOST_CHECK(std::abs(static_cast<int>(to_col(graph.GetID(node))) -
                                 static_cast<int>(to_col(edge.target))) <= 1);
        }
        // itr of node
        for (auto itr = graph.BeginEdges(node); itr != graph.EndEdges(node); ++itr)
        {
            BOOST_CHECK(itr->target < graph.NumberOfNodes());
            BOOST_CHECK(std::abs(static_cast<int>(to_row(graph.GetID(node))) -
                                 static_cast<int>(to_row(itr->target))) <= 1);
            BOOST_CHECK(std::abs(static_cast<int>(to_col(graph.GetID(node))) -
                                 static_cast<int>(to_col(itr->target))) <= 1);
        }

        // access via ID of ndoe
        const auto id = graph.GetID(node);
        for (const auto &edge : graph.Edges(id))
        {
            BOOST_CHECK(edge.target < graph.NumberOfNodes());
            BOOST_CHECK(std::abs(static_cast<int>(to_row(graph.GetID(node))) -
                                 static_cast<int>(to_row(edge.target))) <= 1);
            BOOST_CHECK(std::abs(static_cast<int>(to_col(graph.GetID(node))) -
                                 static_cast<int>(to_col(edge.target))) <= 1);
        }

        for (auto itr = graph.BeginEdges(id); itr != graph.EndEdges(id); ++itr)
        {
            BOOST_CHECK(itr->target < graph.NumberOfNodes());
            BOOST_CHECK(std::abs(static_cast<int>(to_row(graph.GetID(node))) -
                                 static_cast<int>(to_row(itr->target))) <= 1);
            BOOST_CHECK(std::abs(static_cast<int>(to_col(graph.GetID(node))) -
                                 static_cast<int>(to_col(itr->target))) <= 1);
        }
        for (auto eid = graph.BeginEdgeID(id); eid != graph.EndEdgeID(id); ++eid)
        {
            const auto &itr = graph.Edge(eid);
            BOOST_CHECK(itr.target < graph.NumberOfNodes());
            BOOST_CHECK(std::abs(static_cast<int>(to_row(graph.GetID(node))) -
                                 static_cast<int>(to_row(itr.target))) <= 1);
            BOOST_CHECK(std::abs(static_cast<int>(to_col(graph.GetID(node))) -
                                 static_cast<int>(to_col(itr.target))) <= 1);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
