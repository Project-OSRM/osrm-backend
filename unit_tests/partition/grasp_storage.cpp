#include "common/range_tools.hpp"
#include <boost/test/unit_test.hpp>

#include "partition/cell_storage.hpp"
#include "partition/grasp_storage.hpp"
#include "util/static_graph.hpp"

using namespace osrm;
using namespace osrm::partition;

namespace
{
struct MockEdge
{
    NodeID start;
    NodeID target;

    bool operator<(const MockEdge &other)
    {
        return std::tie(start, target) < std::tie(other.start, other.target);
    }
    bool operator==(const MockEdge &other)
    {
        return std::tie(start, target) == std::tie(other.start, other.target);
    }
    bool operator!=(const MockEdge &other) { return !(*this == other); }
};
std::ostream &operator<<(std::ostream &out, const MockEdge &m)
{
    out << m.start << "->" << m.target;
    return out;
}

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
}

BOOST_AUTO_TEST_SUITE(grasp_storage_tests)

BOOST_AUTO_TEST_CASE(grasp_storage)
{
    /*
        Example graph from paper:
        Note: The paper marks 5 as border node, without having a border edge on level 1.
              This is wrong hence I inserted an edge from 5 to 7 to make it true.
        10
          \
           11---5---0---3---8---13
           |   / \ /        |
           6------1----     |
           | /         \    |
           7--------2---4---9---14
                    |
                   12
    */
    // node:                0  1  2  3  4  5  6  7  8  9 10 11 12 13 14
    std::vector<CellID> l1{{0, 0, 1, 3, 2, 0, 0, 1, 3, 2, 0, 0, 1, 3, 2}};
    std::vector<CellID> l2{{0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1}};
    std::vector<CellID> l3{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    MultiLevelPartition mlp{{l1, l2, l3}, {4, 2, 1}};

    std::vector<MockEdge> edges = {
        {0, 1},
        {0, 3},
        {0, 5},
        {1, 4},
        {1, 5},
        {1, 6},
        {2, 4},
        {2, 7},
        {2, 12},
        {3, 8},
        {4, 9},
        {5, 7},
        {5, 11},
        {6, 7},
        {6, 11},
        {8, 9},
        {8, 13},
        {9, 14},
        {10, 11}
    };
    auto graph = makeGraph(edges);

    // test const storage
    const CellStorage cell_storage(mlp, graph);
    const GRASPStorage grasp_storage(mlp, graph, cell_storage);

    std::vector<MockEdge> down_edges;
    for (auto node : util::irange<NodeID>(0, graph.GetNumberOfNodes()))
    {
        for (auto edge : grasp_storage.GetDownwardEdgeRange(node))
        {
            auto source = grasp_storage.GetSource(edge);
            down_edges.push_back(MockEdge{source, node});
        }
    }
    std::sort(down_edges.begin(), down_edges.end());

    std::vector<MockEdge> down_edges_reference = {
        {0, 5},
        {0, 6},
        {0, 7},
        {1, 5},
        {1, 6},
        {1, 7},
        {2, 5},
        {2, 6},
        {2, 7},
        {3, 8},
        {3, 9},
        {4, 8},
        {4, 9},
        {5, 10},
        {5, 11},
        {6, 10},
        {6, 11},
        {7, 12},
        {8, 13},
        {9, 14}
    };
    std::sort(down_edges_reference.begin(), down_edges_reference.end());

    CHECK_EQUAL_COLLECTIONS(down_edges, down_edges_reference);
}

BOOST_AUTO_TEST_SUITE_END()
