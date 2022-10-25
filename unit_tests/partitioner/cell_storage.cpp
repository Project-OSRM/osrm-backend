#include "common/range_tools.hpp"
#include <boost/test/unit_test.hpp>

#include "partitioner/cell_storage.hpp"
#include "util/static_graph.hpp"

using namespace osrm;
using namespace osrm::partitioner;

namespace
{
struct MockEdge
{
    NodeID start;
    NodeID target;
};

auto makeGraph(const std::vector<MockEdge> &mock_edges)
{
    struct EdgeData
    {
        bool forward;
        bool backward;
    };
    using Edge = util::static_graph_details::SortableEdgeWithData<EdgeData>;
    std::vector<Edge> edges;
    std::size_t max_id = 0;
    for (const auto &m : mock_edges)
    {
        max_id = std::max<std::size_t>(max_id, std::max(m.start, m.target));
        edges.push_back(Edge{m.start, m.target, true, false});
        edges.push_back(Edge{m.target, m.start, false, true});
    }
    std::sort(edges.begin(), edges.end());
    return util::StaticGraph<EdgeData>(max_id + 1, edges);
}
} // namespace

BOOST_AUTO_TEST_SUITE(cell_storage_tests)

BOOST_AUTO_TEST_CASE(mutable_cell_storage)
{
    const auto fill_range = [](auto range, const std::vector<EdgeWeight> &values) {
        auto iter = range.begin();
        for (auto v : values)
            *iter++ = v;
        BOOST_CHECK_EQUAL(range.end(), iter);
    };

    // node:                0  1  2  3  4  5  6  7  8  9 10 11
    std::vector<CellID> l1{{0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5}};
    std::vector<CellID> l2{{0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3}};
    std::vector<CellID> l3{{0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}};
    std::vector<CellID> l4{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    MultiLevelPartition mlp{{l1, l2, l3, l4}, {6, 4, 2, 1}};

    std::vector<MockEdge> edges = {
        // edges sorted into border/internal by level
        //  level:  (1) (2) (3) (4)
        {0, 1},   //  i   i   i   i
        {2, 3},   //  i   i   i   i
        {3, 7},   //  b   b   b   i
        {4, 0},   //  b   b   b   i
        {4, 5},   //  i   i   i   i
        {5, 6},   //  b   i   i   i
        {6, 4},   //  b   i   i   i
        {6, 7},   //  i   i   i   i
        {7, 11},  //  b   b   i   i
        {8, 9},   //  i   i   i   i
        {9, 8},   //  i   i   i   i
        {10, 11}, //  i   i   i   i
        {11, 10}  //  i   i   i   i
    };

    auto graph = makeGraph(edges);

    // test non-const storage
    CellStorage storage(mlp, graph);
    auto metric = storage.MakeMetric();

    // Level 1
    auto cell_1_0 = storage.GetCell(metric, 1, 0);
    auto cell_1_1 = storage.GetCell(metric, 1, 1);
    auto cell_1_2 = storage.GetCell(metric, 1, 2);
    auto cell_1_3 = storage.GetCell(metric, 1, 3);
    auto cell_1_4 = storage.GetCell(metric, 1, 4);
    auto cell_1_5 = storage.GetCell(metric, 1, 5);

    (void)cell_1_4; // does not have border nodes

    auto out_range_1_0_0 = cell_1_0.GetOutWeight(0);
    auto out_range_1_2_4 = cell_1_2.GetOutWeight(4);
    auto out_range_1_3_6 = cell_1_3.GetOutWeight(6);
    auto out_range_1_5_11 = cell_1_5.GetOutWeight(11);

    auto in_range_1_1_3 = cell_1_1.GetInWeight(3);
    auto in_range_1_2_5 = cell_1_2.GetInWeight(5);
    auto in_range_1_3_7 = cell_1_3.GetInWeight(7);
    auto in_range_1_5_11 = cell_1_5.GetInWeight(11);

    fill_range(out_range_1_0_0, {});
    fill_range(out_range_1_2_4, {EdgeWeight{1}});
    fill_range(out_range_1_3_6, {EdgeWeight{2}});
    fill_range(out_range_1_5_11, {EdgeWeight{3}});

    CHECK_EQUAL_COLLECTIONS(in_range_1_1_3, std::vector<EdgeWeight>{});
    CHECK_EQUAL_RANGE(in_range_1_2_5, EdgeWeight{1});
    CHECK_EQUAL_RANGE(in_range_1_3_7, EdgeWeight{2});
    CHECK_EQUAL_RANGE(in_range_1_5_11, EdgeWeight{3});

    // Level 2
    auto cell_2_0 = storage.GetCell(metric, 2, 0);
    auto cell_2_1 = storage.GetCell(metric, 2, 1);
    auto cell_2_2 = storage.GetCell(metric, 2, 2);
    auto cell_2_3 = storage.GetCell(metric, 2, 3);

    (void)cell_2_2; // does not have border nodes

    auto out_range_2_0_0 = cell_2_0.GetOutWeight(0);
    auto out_range_2_1_4 = cell_2_1.GetOutWeight(4);
    auto out_range_2_3_11 = cell_2_3.GetOutWeight(11);

    auto in_range_2_0_3 = cell_2_0.GetInWeight(3);
    auto in_range_2_1_4 = cell_2_1.GetInWeight(4);
    auto in_range_2_1_7 = cell_2_1.GetInWeight(7);
    auto in_range_2_3_11 = cell_2_3.GetInWeight(11);

    fill_range(out_range_2_0_0, {EdgeWeight{1}});
    fill_range(out_range_2_1_4, {EdgeWeight{2}, EdgeWeight{3}});
    fill_range(out_range_2_3_11, {EdgeWeight{4}});

    CHECK_EQUAL_RANGE(in_range_2_0_3, EdgeWeight{1});
    CHECK_EQUAL_RANGE(in_range_2_1_4, EdgeWeight{2});
    CHECK_EQUAL_RANGE(in_range_2_1_7, EdgeWeight{3});
    CHECK_EQUAL_RANGE(in_range_2_3_11, EdgeWeight{4});

    // Level 3
    auto cell_3_0 = storage.GetCell(metric, 3, 0);
    auto cell_3_1 = storage.GetCell(metric, 3, 1);

    auto out_range_3_0_0 = cell_3_0.GetOutWeight(0);
    auto out_range_3_1_4 = cell_3_1.GetOutWeight(4);
    auto out_range_3_1_7 = cell_3_1.GetOutWeight(7);

    auto in_range_3_0_3 = cell_3_0.GetInWeight(3);
    auto in_range_3_1_4 = cell_3_1.GetInWeight(4);
    auto in_range_3_1_7 = cell_3_1.GetInWeight(7);

    fill_range(out_range_3_0_0, {EdgeWeight{1}});
    fill_range(out_range_3_1_4, {EdgeWeight{2}, EdgeWeight{3}});
    fill_range(out_range_3_1_7, {EdgeWeight{4}, EdgeWeight{5}});

    CHECK_EQUAL_RANGE(in_range_3_0_3, EdgeWeight{1});
    CHECK_EQUAL_RANGE(in_range_3_1_4, EdgeWeight{2}, EdgeWeight{4});
    CHECK_EQUAL_RANGE(in_range_3_1_7, EdgeWeight{3}, EdgeWeight{5});
}

BOOST_AUTO_TEST_CASE(immutable_cell_storage)
{
    // node:                0  1  2  3  4  5  6  7  8  9 10 11
    std::vector<CellID> l1{{0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5}};
    std::vector<CellID> l2{{0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3}};
    std::vector<CellID> l3{{0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}};
    std::vector<CellID> l4{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    MultiLevelPartition mlp{{l1, l2, l3, l4}, {5, 4, 2, 1}};

    std::vector<MockEdge> edges = {
        // edges sorted into border/internal by level
        //  level:  (1) (2) (3) (4)
        {0, 1},   //  i   i   i   i
        {2, 3},   //  i   i   i   i
        {3, 7},   //  b   b   b   i
        {4, 0},   //  b   b   b   i
        {4, 5},   //  i   i   i   i
        {5, 6},   //  b   i   i   i
        {6, 4},   //  b   i   i   i
        {6, 7},   //  i   i   i   i
        {7, 11},  //  b   b   i   i
        {8, 9},   //  i   i   i   i
        {9, 8},   //  i   i   i   i
        {10, 11}, //  i   i   i   i
        {11, 10}  //  i   i   i   i
    };

    auto graph = makeGraph(edges);

    // nodes sorted into border/internal by level
    //   (1) (2) (3) (4)
    // 0  b   b   b   i
    // 1  i   i   i   i
    // 2  i   i   i   i
    // 3  b   b   b   i
    // 4  b   b   b   i
    // 5  b   i   i   i
    // 6  b   i   i   i
    // 7  b   b   i   i
    // 8  i   i   i   i
    // 9  i   i   i   i
    // 10 i   i   i   i
    // 11 b   b   i   i

    // 1/0: 0 : 1,1,0
    // 1/2: 4 : 1,1,0
    // 1/3: 6 : 1,1,0
    // 1/5: 11 : 1,1,1
    // 1/1: 3 : 1,0,1
    // 1/2: 5 : 1,0,1
    // 1/3: 7 : 1,0,1

    // 2/0: 0 : 1,1,0
    // 2/1: 4 : 1,1,1
    // 2/3: 11 : 1,1,1
    // 2/0: 3 : 1,0,1
    // 2/1: 7 : 1,0,1

    // 3/0: 0 : 1,1,0
    // 3/1: 4 : 1,1,1
    // 3/1: 7 : 1,1,1
    // 3/0: 3 : 1,0,1

    // test const storage
    const CellStorage const_storage(mlp, graph);
    const auto metric = const_storage.MakeMetric();

    auto const_cell_1_0 = const_storage.GetCell(metric, 1, 0);
    auto const_cell_1_1 = const_storage.GetCell(metric, 1, 1);
    auto const_cell_1_2 = const_storage.GetCell(metric, 1, 2);
    auto const_cell_1_3 = const_storage.GetCell(metric, 1, 3);
    auto const_cell_1_4 = const_storage.GetCell(metric, 1, 4);
    auto const_cell_1_5 = const_storage.GetCell(metric, 1, 5);

    CHECK_EQUAL_RANGE(const_cell_1_0.GetSourceNodes(), 0);
    CHECK_EQUAL_COLLECTIONS(const_cell_1_1.GetSourceNodes(), std::vector<NodeID>{});
    CHECK_EQUAL_RANGE(const_cell_1_2.GetSourceNodes(), 4);
    CHECK_EQUAL_RANGE(const_cell_1_3.GetSourceNodes(), 6);
    CHECK_EQUAL_COLLECTIONS(const_cell_1_4.GetSourceNodes(), std::vector<NodeID>{});
    CHECK_EQUAL_RANGE(const_cell_1_5.GetSourceNodes(), 11);

    CHECK_EQUAL_COLLECTIONS(const_cell_1_0.GetDestinationNodes(), std::vector<NodeID>{});
    CHECK_EQUAL_RANGE(const_cell_1_1.GetDestinationNodes(), 3);
    CHECK_EQUAL_RANGE(const_cell_1_2.GetDestinationNodes(), 5);
    CHECK_EQUAL_RANGE(const_cell_1_3.GetDestinationNodes(), 7);
    CHECK_EQUAL_COLLECTIONS(const_cell_1_4.GetDestinationNodes(), std::vector<NodeID>{});
    CHECK_EQUAL_RANGE(const_cell_1_5.GetDestinationNodes(), 11);

    auto out_const_range_1_0_0 = const_cell_1_0.GetOutWeight(0);
    auto out_const_range_1_2_4 = const_cell_1_2.GetOutWeight(4);
    auto out_const_range_1_3_6 = const_cell_1_3.GetOutWeight(6);
    auto out_const_range_1_5_11 = const_cell_1_5.GetOutWeight(11);

    auto in_const_range_1_1_3 = const_cell_1_1.GetInWeight(3);
    auto in_const_range_1_2_5 = const_cell_1_2.GetInWeight(5);
    auto in_const_range_1_3_7 = const_cell_1_3.GetInWeight(7);
    auto in_const_range_1_5_11 = const_cell_1_5.GetInWeight(11);

    REQUIRE_SIZE_RANGE(out_const_range_1_0_0, 0);
    REQUIRE_SIZE_RANGE(out_const_range_1_2_4, 1);
    REQUIRE_SIZE_RANGE(out_const_range_1_3_6, 1);
    REQUIRE_SIZE_RANGE(out_const_range_1_5_11, 1);

    REQUIRE_SIZE_RANGE(in_const_range_1_1_3, 0);
    REQUIRE_SIZE_RANGE(in_const_range_1_2_5, 1);
    REQUIRE_SIZE_RANGE(in_const_range_1_3_7, 1);
    REQUIRE_SIZE_RANGE(in_const_range_1_5_11, 1);

    // Level 2
    auto const_cell_2_0 = const_storage.GetCell(metric, 2, 0);
    auto const_cell_2_1 = const_storage.GetCell(metric, 2, 1);
    auto const_cell_2_2 = const_storage.GetCell(metric, 2, 2);
    auto const_cell_2_3 = const_storage.GetCell(metric, 2, 3);

    CHECK_EQUAL_RANGE(const_cell_2_0.GetSourceNodes(), 0);
    CHECK_EQUAL_RANGE(const_cell_2_1.GetSourceNodes(), 4);
    CHECK_EQUAL_COLLECTIONS(const_cell_2_2.GetSourceNodes(), std::vector<NodeID>{});
    CHECK_EQUAL_RANGE(const_cell_2_3.GetSourceNodes(), 11);

    CHECK_EQUAL_RANGE(const_cell_2_0.GetDestinationNodes(), 3);
    CHECK_EQUAL_RANGE(const_cell_2_1.GetDestinationNodes(), 4, 7);
    CHECK_EQUAL_COLLECTIONS(const_cell_2_2.GetDestinationNodes(), std::vector<NodeID>{});
    CHECK_EQUAL_RANGE(const_cell_2_3.GetDestinationNodes(), 11);

    auto out_const_range_2_0_0 = const_cell_2_0.GetOutWeight(0);
    auto out_const_range_2_1_4 = const_cell_2_1.GetOutWeight(4);
    auto out_const_range_2_3_11 = const_cell_2_3.GetOutWeight(11);

    auto in_const_range_2_0_3 = const_cell_2_0.GetInWeight(3);
    auto in_const_range_2_1_4 = const_cell_2_1.GetInWeight(4);
    auto in_const_range_2_1_7 = const_cell_2_1.GetInWeight(7);
    auto in_const_range_2_3_7 = const_cell_2_3.GetInWeight(11);

    REQUIRE_SIZE_RANGE(out_const_range_2_0_0, 1);
    REQUIRE_SIZE_RANGE(out_const_range_2_1_4, 2);
    REQUIRE_SIZE_RANGE(out_const_range_2_3_11, 1);

    REQUIRE_SIZE_RANGE(in_const_range_2_0_3, 1);
    REQUIRE_SIZE_RANGE(in_const_range_2_1_4, 1);
    REQUIRE_SIZE_RANGE(in_const_range_2_1_7, 1);
    REQUIRE_SIZE_RANGE(in_const_range_2_3_7, 1);

    // Level 3
    auto const_cell_3_0 = const_storage.GetCell(metric, 3, 0);
    auto const_cell_3_1 = const_storage.GetCell(metric, 3, 1);

    CHECK_EQUAL_RANGE(const_cell_3_0.GetSourceNodes(), 0);
    CHECK_EQUAL_RANGE(const_cell_3_1.GetSourceNodes(), 4, 7);

    CHECK_EQUAL_RANGE(const_cell_3_0.GetDestinationNodes(), 3);
    CHECK_EQUAL_RANGE(const_cell_3_1.GetDestinationNodes(), 4, 7);

    auto out_const_range_3_0_0 = const_cell_3_0.GetOutWeight(0);
    auto out_const_range_3_1_4 = const_cell_3_1.GetOutWeight(4);
    auto out_const_range_3_1_7 = const_cell_3_1.GetOutWeight(7);

    auto in_const_range_3_0_3 = const_cell_3_0.GetInWeight(3);
    auto in_const_range_3_1_4 = const_cell_3_1.GetInWeight(4);
    auto in_const_range_3_1_7 = const_cell_3_1.GetInWeight(7);

    REQUIRE_SIZE_RANGE(out_const_range_3_0_0, 1);
    REQUIRE_SIZE_RANGE(out_const_range_3_1_4, 2);
    REQUIRE_SIZE_RANGE(out_const_range_3_1_7, 2);

    REQUIRE_SIZE_RANGE(in_const_range_3_0_3, 1);
    REQUIRE_SIZE_RANGE(in_const_range_3_1_4, 2);
    REQUIRE_SIZE_RANGE(in_const_range_3_1_7, 2);

    // Level 4
    auto const_cell_4_0 = const_storage.GetCell(metric, 4, 0);
    CHECK_EQUAL_COLLECTIONS(const_cell_4_0.GetSourceNodes(), std::vector<NodeID>{});
    CHECK_EQUAL_COLLECTIONS(const_cell_4_0.GetDestinationNodes(), std::vector<NodeID>{});
}

BOOST_AUTO_TEST_SUITE_END()
