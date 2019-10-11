#include "common/range_tools.hpp"

#include "customizer/cell_update_record.hpp"
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

BOOST_AUTO_TEST_SUITE(cell_update_record_test)

BOOST_AUTO_TEST_CASE(cell_update_record_basic_test)
{
    // 0 --- 1 --- 4
    // |     |     |
    // 2 --- 3 --- 5
    // node:                0  1  2  3  4  5
    std::vector<CellID> l1{{0, 0, 1, 1, 1, 1}};
    std::vector<CellID> l2{{0, 0, 0, 0, 0, 0}};
    MultiLevelPartition mlp{{l1, l2}, {2, 1}};

    BOOST_REQUIRE_EQUAL(mlp.GetNumberOfLevels(), 3);

    std::vector<MockEdge> edges = {{0, 1, 1}, {0, 2, 1}, {2, 3, 1}, {3, 1, 1}, 
                                   {2, 4, 1}, {3, 5, 1}, {5, 3, 1}, {4, 5, 1}};
    
    auto graph = makeGraph(mlp, edges);
    std::vector<bool> node_filter(true, graph.GetNumberOfNodes());

    CellStorage storage_rec(mlp, graph);
    auto metric_rec = storage_rec.MakeMetric();

    auto cell_1_0 = storage_rec.GetCell(metric_rec, 1, 0);
    auto cell_1_1 = storage_rec.GetCell(metric_rec, 1, 1);
    //auto cell_2_0 = storage_rec.GetCell(metric_rec, 2, 0);

    // level 1
    CHECK_EQUAL_RANGE(cell_1_0.GetSourceNodes(), 0);
    CHECK_EQUAL_RANGE(cell_1_0.GetDestinationNodes(), 1);

    CHECK_EQUAL_RANGE(cell_1_1.GetSourceNodes(), 2, 3);
    CHECK_EQUAL_RANGE(cell_1_1.GetDestinationNodes(), 3);

    CellCustomizer customizer(mlp);
    CellUpdateRecord cell_update_record(mlp, false);
    customizer.Customize(graph, storage_rec, node_filter, metric_rec, cell_update_record);

    // verify customization result
    CHECK_EQUAL_RANGE(cell_1_0.GetOutWeight(0), 1);
    CHECK_EQUAL_RANGE(cell_1_0.GetInWeight(1), 1);

    CHECK_EQUAL_RANGE(cell_1_1.GetOutWeight(2), 1);
    CHECK_EQUAL_RANGE(cell_1_1.GetOutWeight(3), 0);
    CHECK_EQUAL_RANGE(cell_1_1.GetInWeight(3), 1, 0);
}

BOOST_AUTO_TEST_CASE(cell_update_record_test_check)
{
    // 0 --- 1 --- 4
    // |     |     |
    // 2 --- 3 --- 5
    // node:                0  1  2  3  4  5
    std::vector<CellID> l1{{0, 0, 1, 1, 1, 1}};
    std::vector<CellID> l2{{0, 0, 0, 0, 0, 0}};
    MultiLevelPartition mlp{{l1, l2}, {2, 1}};

    BOOST_REQUIRE_EQUAL(mlp.GetNumberOfLevels(), 3);

    updater::NodeSetPtr ss = std::make_shared<updater::NodeSet>();
    ss->insert(2);
    ss->insert(3);
    CellUpdateRecord cr(mlp, true);
    cr.Collect(ss);

    BOOST_REQUIRE_EQUAL(cr.Check(1, 0), false);
    BOOST_REQUIRE_EQUAL(cr.Check(1, 1), true);
    BOOST_REQUIRE_EQUAL(cr.Check(2, 0), true);
}


BOOST_AUTO_TEST_CASE(cell_update_record_with_cost_update)
{
    // 0 --- 1 --- 4
    // |     |     |
    // 2 --- 3 --- 5
    // node:                0  1  2  3  4  5
    std::vector<CellID> l1{{0, 0, 1, 1, 1, 1}};
    std::vector<CellID> l2{{0, 0, 0, 0, 0, 0}};
    MultiLevelPartition mlp{{l1, l2}, {2, 1}};

    BOOST_REQUIRE_EQUAL(mlp.GetNumberOfLevels(), 3);

    // update cost {2, 3, 2}
    // A better way to test is updating cost in the graph after initial customization
    std::vector<MockEdge> edges = {{0, 1, 1}, {0, 2, 1}, {2, 3, 1}, {3, 1, 1}, 
                                   {2, 4, 1}, {3, 5, 1}, {5, 3, 1}, {4, 5, 1}};
    
    auto graph = makeGraph(mlp, edges);
    std::vector<bool> node_filter(true, graph.GetNumberOfNodes());
    CellStorage storage_rec(mlp, graph);
    auto metric_rec = storage_rec.MakeMetric();
    auto cell_1_0 = storage_rec.GetCell(metric_rec, 1, 0);
    auto cell_1_1 = storage_rec.GetCell(metric_rec, 1, 1);

    CellCustomizer customizer(mlp);
    CellUpdateRecord cr(mlp, false);
    customizer.Customize(graph, storage_rec, node_filter, metric_rec, cr);

    // verify customization result
    CHECK_EQUAL_RANGE(cell_1_0.GetOutWeight(0), 1);
    CHECK_EQUAL_RANGE(cell_1_0.GetInWeight(1), 1);

    CHECK_EQUAL_RANGE(cell_1_1.GetOutWeight(2), 1);
    CHECK_EQUAL_RANGE(cell_1_1.GetOutWeight(3), 0);
    CHECK_EQUAL_RANGE(cell_1_1.GetInWeight(3), 1, 0);

    // Init graph 2
    // update cost {2, 3, 2}
    // A better way to test is updating cost in the graph after initial customization
    std::vector<MockEdge> edges2 = {{0, 1, 1}, {0, 2, 1}, {2, 3, 2}, {3, 1, 1}, 
                                   {2, 4, 1}, {3, 5, 1}, {5, 3, 1}, {4, 5, 1}};
    auto graph2 = makeGraph(mlp, edges2);

    updater::NodeSetPtr ss = std::make_shared<updater::NodeSet>();
    ss->insert(2);
    ss->insert(3);
    updater::NodeSetViewerPtr sViewer = std::move(ss);
    CellUpdateRecord cr2(mlp, true);
    cr2.Collect(sViewer);
    customizer.Customize(graph2, storage_rec, node_filter, metric_rec, cr2);
    // verify customization result
    CHECK_EQUAL_RANGE(cell_1_0.GetOutWeight(0), 1);
    CHECK_EQUAL_RANGE(cell_1_0.GetInWeight(1), 1);

    CHECK_EQUAL_RANGE(cell_1_1.GetOutWeight(2), 2);
    CHECK_EQUAL_RANGE(cell_1_1.GetOutWeight(3), 0);
    CHECK_EQUAL_RANGE(cell_1_1.GetInWeight(3), 2, 0);

    BOOST_REQUIRE_EQUAL(cr2.Statistic(), "Cell Update Status(count for levels): (1,1) of (2,1) be updated.  About 66.67% in total.\n");
}

BOOST_AUTO_TEST_SUITE_END()

