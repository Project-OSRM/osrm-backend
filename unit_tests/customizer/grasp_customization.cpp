#include "common/range_tools.hpp"
#include <boost/test/unit_test.hpp>

#include "customizer/grasp_cell_customizer.hpp"
#include "partition/cell_storage.hpp"
#include "partition/grasp_storage.hpp"
#include "partition/multi_level_graph.hpp"
#include "partition/multi_level_partition.hpp"
#include "util/static_graph.hpp"

using namespace osrm;
using namespace osrm::customizer;
using namespace osrm::partition;
using namespace osrm::util;

namespace
{
struct MockEdge
{
    NodeID start;
    NodeID target;
    EdgeWeight weight;
    bool operator<(const MockEdge &other)
    {
        return std::tie(start, target, weight) < std::tie(other.start, other.target, other.weight);
    }
    bool operator==(const MockEdge &other)
    {
        return std::tie(start, target, weight) == std::tie(other.start, other.target, other.weight);
    }
    bool operator!=(const MockEdge &other) { return !(*this == other); }
};
std::ostream &operator<<(std::ostream &out, const MockEdge &m)
{
    out << m.start << "->" << m.target << "(" << m.weight << ")";
    return out;
}

auto makeGraph(const MultiLevelPartition &mlp, const std::vector<MockEdge> &mock_edges)
{
    struct EdgeData
    {
        EdgeWeight weight;
        bool forward;
        bool backward;
    };
    using Edge = static_graph_details::SortableEdgeWithData<EdgeData>;
    std::vector<Edge> edges;
    std::size_t max_id = 0;
    for (const auto &m : mock_edges)
    {
        max_id = std::max<std::size_t>(max_id, std::max(m.start, m.target));
        edges.push_back(Edge{m.start, m.target, m.weight, true, false});
        edges.push_back(Edge{m.target, m.start, m.weight, false, true});
    }
    std::sort(edges.begin(), edges.end());
    return partition::MultiLevelGraph<EdgeData, osrm::storage::Ownership::Container>(
        mlp, max_id + 1, edges);
}
}

BOOST_AUTO_TEST_SUITE(grasp_customization_tests)

BOOST_AUTO_TEST_CASE(paper_test)
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

    std::vector<MockEdge> edges = {{0, 1, 1},
                                   {0, 3, 1},
                                   {0, 5, 1},
                                   {1, 4, 1},
                                   {1, 5, 1},
                                   {1, 6, 1},
                                   {2, 4, 1},
                                   {2, 7, 1},
                                   {2, 12, 1},
                                   {3, 8, 1},
                                   {4, 9, 1},
                                   {5, 7, 1},
                                   {5, 11, 1},
                                   {6, 7, 1},
                                   {6, 11, 1},
                                   {8, 9, 1},
                                   {8, 13, 1},
                                   {9, 14, 1},
                                   {10, 11, 1}};
    auto graph = makeGraph(mlp, edges);

    CellStorage cell_storage(mlp, graph);
    GRASPStorage grasp_storage(mlp, graph, cell_storage);
    GRASPCellCustomizer customizer(mlp);

    customizer.Customize(graph, cell_storage, grasp_storage);

    std::vector<MockEdge> down_edges;
    for (auto node : util::irange<NodeID>(0, graph.GetNumberOfNodes()))
    {
        for (auto edge : grasp_storage.GetDownwardEdgeRange(node))
        {
            auto source = grasp_storage.GetSource(edge);
            auto weight = grasp_storage.GetEdgeData(edge).weight;
            down_edges.push_back(MockEdge{source, node, weight});
        }
    }
    std::sort(down_edges.begin(), down_edges.end());

    std::vector<MockEdge> down_edges_reference = {{0, 5, 1},
                                                  {0, 6, 2},
                                                  {0, 7, 2},
                                                  {1, 5, 1},
                                                  {1, 6, 1},
                                                  {1, 7, 2},
                                                  {2, 5, INVALID_EDGE_WEIGHT},
                                                  {2, 6, INVALID_EDGE_WEIGHT},
                                                  {2, 7, 1},
                                                  {3, 8, 1},
                                                  {3, 9, 2},
                                                  {4, 8, INVALID_EDGE_WEIGHT},
                                                  {4, 9, 1},
                                                  {5, 10, INVALID_EDGE_WEIGHT},
                                                  {5, 11, 1},
                                                  {6, 10, INVALID_EDGE_WEIGHT},
                                                  {6, 11, 1},
                                                  {7, 12, INVALID_EDGE_WEIGHT},
                                                  {8, 13, 1},
                                                  {9, 14, 1}};
    std::sort(down_edges_reference.begin(), down_edges_reference.end());

    CHECK_EQUAL_COLLECTIONS(down_edges, down_edges_reference);
}

BOOST_AUTO_TEST_CASE(paper_test_undirected)
{
    /*
        Example graph from paper:
        Note: The paper marks 5 as border node, without having a border edge on level 1.
              Here 5 is not a border node.
        10
          \
           11---5---0---3---8---13
           |     \ /        |
           6------1----     |
           |           \    |
           7--------2---4---9---14
                    |
                   12
    */
    // node:                0  1  2  3  4  5  6  7  8  9 10 11 12 13 14
    std::vector<CellID> l1{{0, 0, 1, 3, 2, 0, 0, 1, 3, 2, 0, 0, 1, 3, 2}};
    std::vector<CellID> l2{{0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1}};
    std::vector<CellID> l3{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    MultiLevelPartition mlp{{l1, l2, l3}, {4, 2, 1}};

    std::vector<MockEdge> edges = {{0, 1, 1},
                                   {1, 0, 1},
                                   {0, 3, 1},
                                   {3, 0, 1},
                                   {0, 5, 1},
                                   {5, 0, 1},
                                   {1, 4, 1},
                                   {4, 1, 1},
                                   {1, 5, 1},
                                   {5, 1, 1},
                                   {1, 6, 1},
                                   {6, 1, 1},
                                   {2, 4, 1},
                                   {4, 2, 1},
                                   {2, 7, 1},
                                   {7, 2, 1},
                                   {2, 12, 1},
                                   {12, 2, 1},
                                   {3, 8, 1},
                                   {8, 3, 1},
                                   {4, 9, 1},
                                   {9, 4, 1},
                                   {5, 11, 1},
                                   {11, 5, 1},
                                   {6, 7, 1},
                                   {7, 6, 1},
                                   {6, 11, 1},
                                   {11, 6, 1},
                                   {8, 9, 1},
                                   {9, 8, 1},
                                   {8, 13, 1},
                                   {13, 8, 1},
                                   {9, 14, 1},
                                   {14, 9, 1},
                                   {10, 11, 1},
                                   {11, 10, 1}};
    auto graph = makeGraph(mlp, edges);

    CellStorage cell_storage(mlp, graph);
    GRASPStorage grasp_storage(mlp, graph, cell_storage);
    GRASPCellCustomizer customizer(mlp);

    customizer.Customize(graph, cell_storage, grasp_storage);

    std::vector<MockEdge> down_edges;
    for (auto node : util::irange<NodeID>(0, graph.GetNumberOfNodes()))
    {
        for (auto edge : grasp_storage.GetDownwardEdgeRange(node))
        {
            auto source = grasp_storage.GetSource(edge);
            auto weight = grasp_storage.GetEdgeData(edge).weight;
            down_edges.push_back(MockEdge{source, node, weight});
        }
    }
    std::sort(down_edges.begin(), down_edges.end());

    std::vector<MockEdge> down_edges_reference = {{0, 6, 2},
                                                  {0, 7, 3},
                                                  {1, 6, 1},
                                                  {1, 7, 2},
                                                  {2, 6, 2},
                                                  {2, 7, 1},
                                                  {3, 8, 1},
                                                  {3, 9, 2},
                                                  {4, 8, 2},
                                                  {4, 9, 1},
                                                  {6, 5, 2},
                                                  {6, 10, 2},
                                                  {6, 11, 1},
                                                  {7, 12, 2},
                                                  {8, 13, 1},
                                                  {9, 14, 1}};
    std::sort(down_edges_reference.begin(), down_edges_reference.end());

    CHECK_EQUAL_COLLECTIONS(down_edges, down_edges_reference);
}

BOOST_AUTO_TEST_SUITE_END()
