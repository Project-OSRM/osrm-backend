#include "common/range_tools.hpp"

#include "customizer/cell_customizer.hpp"
#include "partitioner/multi_level_graph.hpp"
#include "partitioner/multi_level_partition.hpp"
#include "util/static_graph.hpp"

#include <boost/test/unit_test.hpp>

using namespace osrm;
using namespace osrm::customizer;
using namespace osrm::partitioner;
using namespace osrm::util;

namespace
{
struct MockEdge
{
    NodeID start;
    NodeID target;
    EdgeWeight weight;
};

auto makeGraph(const MultiLevelPartition &mlp, const std::vector<MockEdge> &mock_edges)
{
    struct EdgeData
    {
        EdgeWeight weight;
        EdgeDuration duration;
        EdgeDistance distance;
        bool forward;
        bool backward;
    };
    using Edge = static_graph_details::SortableEdgeWithData<EdgeData>;
    std::vector<Edge> edges;
    std::size_t max_id = 0;
    for (const auto &m : mock_edges)
    {
        max_id = std::max<std::size_t>(max_id, std::max(m.start, m.target));
        edges.push_back(Edge{m.start,
                             m.target,
                             m.weight,
                             2 * m.weight,
                             static_cast<EdgeDistance>(1.0),
                             true,
                             false});
        edges.push_back(Edge{m.target,
                             m.start,
                             m.weight,
                             2 * m.weight,
                             static_cast<EdgeDistance>(1.0),
                             false,
                             true});
    }
    std::sort(edges.begin(), edges.end());
    return partitioner::MultiLevelGraph<EdgeData, osrm::storage::Ownership::Container>(
        mlp, max_id + 1, edges);
}
}

BOOST_AUTO_TEST_SUITE(cell_customization_tests)

BOOST_AUTO_TEST_CASE(two_level_test)
{
    // 0 --- 1
    // |     |
    // 2 --- 3
    // node:                0  1  2  3
    std::vector<CellID> l1{{0, 0, 1, 1}};
    MultiLevelPartition mlp{{l1}, {2}};

    BOOST_REQUIRE_EQUAL(mlp.GetNumberOfLevels(), 2);

    std::vector<MockEdge> edges = {{0, 1, 1}, {0, 2, 1}, {2, 3, 1}, {3, 1, 1}, {3, 2, 1}};

    auto graph = makeGraph(mlp, edges);
    std::vector<bool> node_filter(graph.GetNumberOfNodes(), true);

    CellStorage storage(mlp, graph);
    auto metric = storage.MakeMetric();
    CellCustomizer customizer(mlp);
    CellCustomizer::Heap heap(graph.GetNumberOfNodes());

    auto cell_1_0 = storage.GetCell(metric, 1, 0);
    auto cell_1_1 = storage.GetCell(metric, 1, 1);

    REQUIRE_SIZE_RANGE(cell_1_0.GetSourceNodes(), 1);
    REQUIRE_SIZE_RANGE(cell_1_0.GetDestinationNodes(), 1);
    REQUIRE_SIZE_RANGE(cell_1_1.GetSourceNodes(), 2);
    REQUIRE_SIZE_RANGE(cell_1_1.GetDestinationNodes(), 2);

    CHECK_EQUAL_RANGE(cell_1_0.GetSourceNodes(), 0);
    CHECK_EQUAL_RANGE(cell_1_0.GetDestinationNodes(), 1);
    CHECK_EQUAL_RANGE(cell_1_1.GetSourceNodes(), 2, 3);
    CHECK_EQUAL_RANGE(cell_1_1.GetDestinationNodes(), 2, 3);

    REQUIRE_SIZE_RANGE(cell_1_0.GetOutWeight(0), 1);
    REQUIRE_SIZE_RANGE(cell_1_0.GetOutWeight(1), 0);
    REQUIRE_SIZE_RANGE(cell_1_0.GetInWeight(1), 1);
    REQUIRE_SIZE_RANGE(cell_1_0.GetInWeight(0), 0);
    REQUIRE_SIZE_RANGE(cell_1_1.GetOutWeight(2), 2);
    REQUIRE_SIZE_RANGE(cell_1_1.GetInWeight(3), 2);

    customizer.Customize(graph, heap, storage, node_filter, metric, 1, 0);
    customizer.Customize(graph, heap, storage, node_filter, metric, 1, 1);

    // cell 0
    // check row source -> destination
    CHECK_EQUAL_RANGE(cell_1_0.GetOutWeight(0), 1);
    // check column destination -> source
    CHECK_EQUAL_RANGE(cell_1_0.GetInWeight(1), 1);

    // cell 1
    // check row source -> destination
    CHECK_EQUAL_RANGE(cell_1_1.GetOutWeight(2), 0, 1);
    CHECK_EQUAL_RANGE(cell_1_1.GetOutWeight(3), 1, 0);
    // check column destination -> source
    CHECK_EQUAL_RANGE(cell_1_1.GetInWeight(2), 0, 1);
    CHECK_EQUAL_RANGE(cell_1_1.GetInWeight(3), 1, 0);
}

BOOST_AUTO_TEST_CASE(four_levels_test)
{
    // node:                0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
    std::vector<CellID> l1{{0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3}};
    std::vector<CellID> l2{{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}};
    std::vector<CellID> l3{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    MultiLevelPartition mlp{{l1, l2, l3}, {4, 2, 1}};

    BOOST_REQUIRE_EQUAL(mlp.GetNumberOfLevels(), 4);

    std::vector<MockEdge> edges = {
        {0, 1, 1}, // cell (0, 0, 0)
        {0, 2, 1},    {3, 1, 1},   {3, 2, 1},

        {4, 5, 1}, // cell (1, 0, 0)
        {4, 6, 1},    {4, 7, 1},   {5, 4, 1}, {5, 6, 1}, {5, 7, 1}, {6, 4, 1},
        {6, 5, 1},    {6, 7, 1},   {7, 4, 1}, {7, 5, 1}, {7, 6, 1},

        {9, 11, 1}, // cell (2, 1, 0)
        {10, 8, 1},   {11, 10, 1},

        {13, 12, 10}, // cell (3, 1, 0)
        {15, 14, 1},

        {2, 4, 1},  // edge between cells (0, 0, 0) -> (1, 0, 0)
        {5, 12, 1}, // edge between cells (1, 0, 0) -> (3, 1, 0)
        {8, 3, 1},  // edge between cells (2, 1, 0) -> (0, 0, 0)
        {9, 3, 1},  // edge between cells (2, 1, 0) -> (0, 0, 0)
        {12, 5, 1}, // edge between cells (3, 1, 0) -> (1, 0, 0)
        {13, 7, 1}, // edge between cells (3, 1, 0) -> (1, 0, 0)
        {14, 9, 1}, // edge between cells (2, 1, 0) -> (0, 0, 0)
        {14, 11, 1} // edge between cells (2, 1, 0) -> (0, 0, 0)
    };

    auto graph = makeGraph(mlp, edges);
    std::vector<bool> node_filter(true, graph.GetNumberOfNodes());

    CellStorage storage(mlp, graph);
    auto metric = storage.MakeMetric();

    auto cell_1_0 = storage.GetCell(metric, 1, 0);
    auto cell_1_1 = storage.GetCell(metric, 1, 1);
    auto cell_1_2 = storage.GetCell(metric, 1, 2);
    auto cell_1_3 = storage.GetCell(metric, 1, 3);
    auto cell_2_0 = storage.GetCell(metric, 2, 0);
    auto cell_2_1 = storage.GetCell(metric, 2, 1);
    auto cell_3_0 = storage.GetCell(metric, 3, 0);

    REQUIRE_SIZE_RANGE(cell_1_0.GetSourceNodes(), 1);
    REQUIRE_SIZE_RANGE(cell_1_0.GetDestinationNodes(), 1);
    CHECK_EQUAL_RANGE(cell_1_0.GetSourceNodes(), 3);
    CHECK_EQUAL_RANGE(cell_1_0.GetDestinationNodes(), 2);
    REQUIRE_SIZE_RANGE(cell_1_0.GetOutWeight(3), 1);
    REQUIRE_SIZE_RANGE(cell_1_0.GetInWeight(2), 1);

    REQUIRE_SIZE_RANGE(cell_1_1.GetSourceNodes(), 3);
    REQUIRE_SIZE_RANGE(cell_1_1.GetDestinationNodes(), 3);
    CHECK_EQUAL_RANGE(cell_1_1.GetSourceNodes(), 4, 5, 7);
    CHECK_EQUAL_RANGE(cell_1_1.GetDestinationNodes(), 4, 5, 7);
    REQUIRE_SIZE_RANGE(cell_1_1.GetOutWeight(4), 3);
    REQUIRE_SIZE_RANGE(cell_1_1.GetOutWeight(5), 3);
    REQUIRE_SIZE_RANGE(cell_1_1.GetOutWeight(7), 3);
    REQUIRE_SIZE_RANGE(cell_1_1.GetInWeight(4), 3);
    REQUIRE_SIZE_RANGE(cell_1_1.GetInWeight(5), 3);
    REQUIRE_SIZE_RANGE(cell_1_1.GetInWeight(7), 3);

    REQUIRE_SIZE_RANGE(cell_1_2.GetSourceNodes(), 2);
    REQUIRE_SIZE_RANGE(cell_1_2.GetDestinationNodes(), 2);
    CHECK_EQUAL_RANGE(cell_1_2.GetSourceNodes(), 9, 11);
    CHECK_EQUAL_RANGE(cell_1_2.GetDestinationNodes(), 8, 11);
    REQUIRE_SIZE_RANGE(cell_1_2.GetOutWeight(9), 2);
    REQUIRE_SIZE_RANGE(cell_1_2.GetOutWeight(11), 2);
    REQUIRE_SIZE_RANGE(cell_1_2.GetInWeight(8), 2);
    REQUIRE_SIZE_RANGE(cell_1_2.GetInWeight(11), 2);

    REQUIRE_SIZE_RANGE(cell_1_3.GetSourceNodes(), 1);
    REQUIRE_SIZE_RANGE(cell_1_3.GetDestinationNodes(), 2);
    CHECK_EQUAL_RANGE(cell_1_3.GetSourceNodes(), 13);
    CHECK_EQUAL_RANGE(cell_1_3.GetDestinationNodes(), 12, 14);
    REQUIRE_SIZE_RANGE(cell_1_3.GetOutWeight(13), 2);
    REQUIRE_SIZE_RANGE(cell_1_3.GetInWeight(12), 1);
    REQUIRE_SIZE_RANGE(cell_1_3.GetInWeight(14), 1);

    REQUIRE_SIZE_RANGE(cell_2_0.GetSourceNodes(), 3);
    REQUIRE_SIZE_RANGE(cell_2_0.GetDestinationNodes(), 2);
    CHECK_EQUAL_RANGE(cell_2_0.GetSourceNodes(), 3, 5, 7);
    CHECK_EQUAL_RANGE(cell_2_0.GetDestinationNodes(), 5, 7);
    REQUIRE_SIZE_RANGE(cell_2_0.GetOutWeight(3), 2);
    REQUIRE_SIZE_RANGE(cell_2_0.GetOutWeight(5), 2);
    REQUIRE_SIZE_RANGE(cell_2_0.GetOutWeight(7), 2);
    REQUIRE_SIZE_RANGE(cell_2_0.GetInWeight(5), 3);
    REQUIRE_SIZE_RANGE(cell_2_0.GetInWeight(7), 3);

    REQUIRE_SIZE_RANGE(cell_2_1.GetSourceNodes(), 2);
    REQUIRE_SIZE_RANGE(cell_2_1.GetDestinationNodes(), 3);
    CHECK_EQUAL_RANGE(cell_2_1.GetSourceNodes(), 9, 13);
    CHECK_EQUAL_RANGE(cell_2_1.GetDestinationNodes(), 8, 9, 12);
    REQUIRE_SIZE_RANGE(cell_2_1.GetOutWeight(9), 3);
    REQUIRE_SIZE_RANGE(cell_2_1.GetOutWeight(13), 3);
    REQUIRE_SIZE_RANGE(cell_2_1.GetInWeight(8), 2);
    REQUIRE_SIZE_RANGE(cell_2_1.GetInWeight(9), 2);
    REQUIRE_SIZE_RANGE(cell_2_1.GetInWeight(12), 2);

    REQUIRE_SIZE_RANGE(cell_3_0.GetSourceNodes(), 0);
    REQUIRE_SIZE_RANGE(cell_3_0.GetDestinationNodes(), 0);

    CellCustomizer customizer(mlp);
    CellCustomizer::Heap heap(graph.GetNumberOfNodes());

    customizer.Customize(graph, heap, storage, node_filter, metric, 1, 0);
    customizer.Customize(graph, heap, storage, node_filter, metric, 1, 1);
    customizer.Customize(graph, heap, storage, node_filter, metric, 1, 2);
    customizer.Customize(graph, heap, storage, node_filter, metric, 1, 3);

    customizer.Customize(graph, heap, storage, node_filter, metric, 2, 0);
    customizer.Customize(graph, heap, storage, node_filter, metric, 2, 1);

    // level 1
    // cell 0
    CHECK_EQUAL_RANGE(cell_1_0.GetOutWeight(3), 1);
    CHECK_EQUAL_RANGE(cell_1_0.GetInWeight(2), 1);

    // cell 1
    CHECK_EQUAL_RANGE(cell_1_1.GetOutWeight(4), 0, 1, 1);
    CHECK_EQUAL_RANGE(cell_1_1.GetOutWeight(5), 1, 0, 1);
    CHECK_EQUAL_RANGE(cell_1_1.GetOutWeight(7), 1, 1, 0);
    CHECK_EQUAL_RANGE(cell_1_1.GetInWeight(4), 0, 1, 1);
    CHECK_EQUAL_RANGE(cell_1_1.GetInWeight(5), 1, 0, 1);
    CHECK_EQUAL_RANGE(cell_1_1.GetInWeight(7), 1, 1, 0);

    // cell 2
    CHECK_EQUAL_RANGE(cell_1_2.GetOutWeight(9), 3, 1);
    CHECK_EQUAL_RANGE(cell_1_2.GetOutWeight(11), 2, 0);
    CHECK_EQUAL_RANGE(cell_1_2.GetInWeight(8), 3, 2);
    CHECK_EQUAL_RANGE(cell_1_2.GetInWeight(11), 1, 0);

    // cell 3
    CHECK_EQUAL_RANGE(cell_1_3.GetOutWeight(13), 10, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_1_3.GetInWeight(12), 10);
    CHECK_EQUAL_RANGE(cell_1_3.GetInWeight(14), INVALID_EDGE_WEIGHT);

    // level 2
    // cell 0
    CHECK_EQUAL_RANGE(cell_2_0.GetOutWeight(3), 3, 3);
    CHECK_EQUAL_RANGE(cell_2_0.GetOutWeight(5), 0, 1);
    CHECK_EQUAL_RANGE(cell_2_0.GetOutWeight(7), 1, 0);
    CHECK_EQUAL_RANGE(cell_2_0.GetInWeight(5), 3, 0, 1);
    CHECK_EQUAL_RANGE(cell_2_0.GetInWeight(7), 3, 1, 0);

    // cell 1
    CHECK_EQUAL_RANGE(cell_2_1.GetOutWeight(9), 3, 0, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_2_1.GetOutWeight(13), INVALID_EDGE_WEIGHT, INVALID_EDGE_WEIGHT, 10);
    CHECK_EQUAL_RANGE(cell_2_1.GetInWeight(8), 3, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_2_1.GetInWeight(9), 0, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_2_1.GetInWeight(12), INVALID_EDGE_WEIGHT, 10);

    CHECK_EQUAL_RANGE(cell_2_1.GetOutDuration(9), 6, 0, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_2_1.GetOutDuration(13), INVALID_EDGE_WEIGHT, INVALID_EDGE_WEIGHT, 20);
    CHECK_EQUAL_RANGE(cell_2_1.GetInDuration(8), 6, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_2_1.GetInDuration(9), 0, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_2_1.GetInDuration(12), INVALID_EDGE_WEIGHT, 20);

    CellStorage storage_rec(mlp, graph);
    auto metric_rec = storage_rec.MakeMetric();
    customizer.Customize(graph, storage_rec, node_filter, metric_rec);

    CHECK_EQUAL_COLLECTIONS(cell_2_1.GetOutWeight(9),
                            storage_rec.GetCell(metric_rec, 2, 1).GetOutWeight(9));
    CHECK_EQUAL_COLLECTIONS(cell_2_1.GetOutWeight(13),
                            storage_rec.GetCell(metric_rec, 2, 1).GetOutWeight(13));
    CHECK_EQUAL_COLLECTIONS(cell_2_1.GetInWeight(8),
                            storage_rec.GetCell(metric_rec, 2, 1).GetInWeight(8));
    CHECK_EQUAL_COLLECTIONS(cell_2_1.GetInWeight(9),
                            storage_rec.GetCell(metric_rec, 2, 1).GetInWeight(9));
    CHECK_EQUAL_COLLECTIONS(cell_2_1.GetInWeight(12),
                            storage_rec.GetCell(metric_rec, 2, 1).GetInWeight(12));
}

BOOST_AUTO_TEST_CASE(exclude_test)
{
    // 0 --- 1 --- 5 --- 6
    // |  /  |     |     |
    // 2 ----3 --- 4 --- 7
    // \__________/
    std::vector<MockEdge> edges = {
        {0, 1, 1}, {0, 2, 1}, {1, 0, 1}, {1, 2, 10}, {1, 3, 1}, {1, 5, 1}, {2, 0, 1}, {2, 1, 10},
        {2, 3, 1}, {2, 4, 1}, {3, 1, 1}, {3, 2, 1},  {3, 4, 1}, {4, 2, 1}, {4, 3, 1}, {4, 5, 1},
        {4, 7, 1}, {5, 1, 1}, {5, 4, 1}, {5, 6, 1},  {6, 5, 1}, {6, 7, 1}, {7, 4, 1}, {7, 6, 1},
    };

    // node:                0  1  2  3  4  5  6  7
    std::vector<CellID> l1{{0, 0, 1, 1, 3, 2, 2, 3}};
    std::vector<CellID> l2{{0, 0, 0, 0, 1, 1, 1, 1}};
    std::vector<CellID> l3{{0, 0, 0, 0, 0, 0, 0, 0}};
    MultiLevelPartition mlp{{l1, l2, l3}, {4, 2, 1}};

    BOOST_REQUIRE_EQUAL(mlp.GetNumberOfLevels(), 4);

    auto graph = makeGraph(mlp, edges);
    // exclude node 0, 3 and 7
    std::vector<bool> node_filter = {false, true, true, false, true, true, true, false};

    CellCustomizer customizer(mlp);
    CellStorage storage(mlp, graph);
    auto metric = storage.MakeMetric();
    customizer.Customize(graph, storage, node_filter, metric);

    auto cell_1_0 = storage.GetCell(metric, 1, 0);
    auto cell_1_1 = storage.GetCell(metric, 1, 1);
    auto cell_1_2 = storage.GetCell(metric, 1, 2);
    auto cell_1_3 = storage.GetCell(metric, 1, 3);

    REQUIRE_SIZE_RANGE(cell_1_0.GetSourceNodes(), 2);
    REQUIRE_SIZE_RANGE(cell_1_0.GetDestinationNodes(), 2);
    REQUIRE_SIZE_RANGE(cell_1_1.GetSourceNodes(), 2);
    REQUIRE_SIZE_RANGE(cell_1_1.GetDestinationNodes(), 2);
    REQUIRE_SIZE_RANGE(cell_1_2.GetSourceNodes(), 2);
    REQUIRE_SIZE_RANGE(cell_1_2.GetDestinationNodes(), 2);
    REQUIRE_SIZE_RANGE(cell_1_3.GetSourceNodes(), 2);
    REQUIRE_SIZE_RANGE(cell_1_3.GetDestinationNodes(), 2);

    CHECK_EQUAL_RANGE(cell_1_0.GetSourceNodes(), 0, 1);
    CHECK_EQUAL_RANGE(cell_1_0.GetDestinationNodes(), 0, 1);
    CHECK_EQUAL_RANGE(cell_1_1.GetSourceNodes(), 2, 3);
    CHECK_EQUAL_RANGE(cell_1_1.GetDestinationNodes(), 2, 3);
    CHECK_EQUAL_RANGE(cell_1_2.GetSourceNodes(), 5, 6);
    CHECK_EQUAL_RANGE(cell_1_2.GetDestinationNodes(), 5, 6);
    CHECK_EQUAL_RANGE(cell_1_3.GetSourceNodes(), 4, 7);
    CHECK_EQUAL_RANGE(cell_1_3.GetDestinationNodes(), 4, 7);

    REQUIRE_SIZE_RANGE(cell_1_0.GetOutWeight(0), 2);
    REQUIRE_SIZE_RANGE(cell_1_0.GetOutWeight(1), 2);
    REQUIRE_SIZE_RANGE(cell_1_0.GetInWeight(0), 2);
    REQUIRE_SIZE_RANGE(cell_1_0.GetInWeight(1), 2);
    REQUIRE_SIZE_RANGE(cell_1_1.GetOutWeight(2), 2);
    REQUIRE_SIZE_RANGE(cell_1_1.GetOutWeight(3), 2);
    REQUIRE_SIZE_RANGE(cell_1_1.GetInWeight(2), 2);
    REQUIRE_SIZE_RANGE(cell_1_1.GetInWeight(3), 2);
    REQUIRE_SIZE_RANGE(cell_1_2.GetOutWeight(5), 2);
    REQUIRE_SIZE_RANGE(cell_1_2.GetOutWeight(6), 2);
    REQUIRE_SIZE_RANGE(cell_1_2.GetInWeight(5), 2);
    REQUIRE_SIZE_RANGE(cell_1_2.GetInWeight(6), 2);
    REQUIRE_SIZE_RANGE(cell_1_3.GetOutWeight(4), 2);
    REQUIRE_SIZE_RANGE(cell_1_3.GetOutWeight(7), 2);
    REQUIRE_SIZE_RANGE(cell_1_3.GetInWeight(4), 2);
    REQUIRE_SIZE_RANGE(cell_1_3.GetInWeight(7), 2);

    CHECK_EQUAL_RANGE(cell_1_0.GetOutWeight(0), INVALID_EDGE_WEIGHT, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_1_0.GetOutWeight(1), INVALID_EDGE_WEIGHT, 0);
    CHECK_EQUAL_RANGE(cell_1_0.GetInWeight(0), INVALID_EDGE_WEIGHT, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_1_0.GetInWeight(1), INVALID_EDGE_WEIGHT, 0);
    CHECK_EQUAL_RANGE(cell_1_1.GetOutWeight(2), 0, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_1_1.GetOutWeight(3), INVALID_EDGE_WEIGHT, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_1_1.GetInWeight(2), 0, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_1_1.GetInWeight(3), INVALID_EDGE_WEIGHT, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_1_2.GetOutWeight(5), 0, 1);
    CHECK_EQUAL_RANGE(cell_1_2.GetOutWeight(6), 1, 0);
    CHECK_EQUAL_RANGE(cell_1_2.GetInWeight(5), 0, 1);
    CHECK_EQUAL_RANGE(cell_1_2.GetInWeight(6), 1, 0);
    CHECK_EQUAL_RANGE(cell_1_3.GetOutWeight(4), 0, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_1_3.GetOutWeight(7), INVALID_EDGE_WEIGHT, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_1_3.GetInWeight(4), 0, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_1_3.GetInWeight(7), INVALID_EDGE_WEIGHT, INVALID_EDGE_WEIGHT);

    auto cell_2_0 = storage.GetCell(metric, 2, 0);
    auto cell_2_1 = storage.GetCell(metric, 2, 1);

    REQUIRE_SIZE_RANGE(cell_2_0.GetSourceNodes(), 3);
    REQUIRE_SIZE_RANGE(cell_2_0.GetDestinationNodes(), 3);
    REQUIRE_SIZE_RANGE(cell_2_1.GetSourceNodes(), 2);
    REQUIRE_SIZE_RANGE(cell_2_1.GetDestinationNodes(), 2);

    CHECK_EQUAL_RANGE(cell_2_0.GetSourceNodes(), 1, 2, 3);
    CHECK_EQUAL_RANGE(cell_2_0.GetDestinationNodes(), 1, 2, 3);
    CHECK_EQUAL_RANGE(cell_2_1.GetSourceNodes(), 4, 5);
    CHECK_EQUAL_RANGE(cell_2_1.GetDestinationNodes(), 4, 5);

    REQUIRE_SIZE_RANGE(cell_2_0.GetOutWeight(1), 3);
    REQUIRE_SIZE_RANGE(cell_2_0.GetOutWeight(2), 3);
    REQUIRE_SIZE_RANGE(cell_2_0.GetOutWeight(3), 3);
    REQUIRE_SIZE_RANGE(cell_2_0.GetInWeight(1), 3);
    REQUIRE_SIZE_RANGE(cell_2_0.GetInWeight(2), 3);
    REQUIRE_SIZE_RANGE(cell_2_0.GetInWeight(3), 3);
    REQUIRE_SIZE_RANGE(cell_2_1.GetOutWeight(4), 2);
    REQUIRE_SIZE_RANGE(cell_2_1.GetOutWeight(5), 2);
    REQUIRE_SIZE_RANGE(cell_2_1.GetInWeight(4), 2);
    REQUIRE_SIZE_RANGE(cell_2_1.GetInWeight(5), 2);

    CHECK_EQUAL_RANGE(cell_2_0.GetOutWeight(1), 0, 10, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_2_0.GetOutWeight(2), 10, 0, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(
        cell_2_0.GetOutWeight(3), INVALID_EDGE_WEIGHT, INVALID_EDGE_WEIGHT, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_2_0.GetInWeight(1), 0, 10, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(cell_2_0.GetInWeight(2), 10, 0, INVALID_EDGE_WEIGHT);
    CHECK_EQUAL_RANGE(
        cell_2_0.GetInWeight(3), INVALID_EDGE_WEIGHT, INVALID_EDGE_WEIGHT, INVALID_EDGE_WEIGHT);

    CHECK_EQUAL_RANGE(cell_2_1.GetOutWeight(4), 0, 1);
    CHECK_EQUAL_RANGE(cell_2_1.GetOutWeight(5), 1, 0);
    CHECK_EQUAL_RANGE(cell_2_1.GetInWeight(4), 0, 1);
    CHECK_EQUAL_RANGE(cell_2_1.GetInWeight(5), 1, 0);
}

BOOST_AUTO_TEST_SUITE_END()
