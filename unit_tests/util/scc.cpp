#include "extractor/pearce_scc.hpp"
#include "extractor/tarjan_scc.hpp"
#include "util/integer_range.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <unordered_map>
#include <vector>

BOOST_AUTO_TEST_SUITE(strongly_connected_component)

struct Graph
{
    using NodeIterator = NodeID;
    using EdgeIterator = NodeID;
    using Edge = std::pair<NodeID, NodeID>;
    using EdgeRange = osrm::util::range<EdgeIterator>;

    std::size_t GetNumberOfNodes() const { return m_nodes; }
    EdgeRange GetAdjacentEdgeRange(NodeID v) const
    {
        return EdgeRange(
            std::distance(m_edges.begin(),
                          std::lower_bound(m_edges.begin(), m_edges.end(), Edge{v + 0, 0})),
            std::distance(m_edges.begin(),
                          std::lower_bound(m_edges.begin(), m_edges.end(), Edge{v + 1, 0})));
    }
    NodeID GetSource(EdgeID e) const { return m_edges[e].first; }
    NodeID GetTarget(EdgeID e) const { return m_edges[e].second; }

    NodeID m_nodes;
    std::vector<Edge> m_edges;
};

BOOST_AUTO_TEST_CASE(find_test)
{
    // https://en.wikipedia.org/wiki/Strongly_connected_component#/media/File:Scc.png
    // a → b → c ⇆ d
    // ↑↙ ↓   ↓   ⇵
    // e → f ⇆ g ← h
    // SCCs: {a, b, e}, {c, d, h}, {f, g}
    const Graph graph{8,
                      {{0, 1},
                       {1, 2},
                       {1, 4},
                       {1, 5},
                       {2, 3},
                       {2, 6},
                       {3, 2},
                       {3, 7},
                       {4, 0},
                       {4, 5},
                       {5, 6},
                       {6, 5},
                       {7, 3},
                       {7, 6}}};
    osrm::extractor::PearceSCC<Graph> scc(graph); // TarjanSCC
    scc.Run();
    BOOST_CHECK_EQUAL(scc.GetNumberOfComponents(), 3);
    BOOST_CHECK_EQUAL(scc.GetComponentID(0), scc.GetComponentID(1));
    BOOST_CHECK_EQUAL(scc.GetComponentID(0), scc.GetComponentID(4));
    BOOST_CHECK_EQUAL(scc.GetComponentID(2), scc.GetComponentID(3));
    BOOST_CHECK_EQUAL(scc.GetComponentID(2), scc.GetComponentID(7));
    BOOST_CHECK_EQUAL(scc.GetComponentID(5), scc.GetComponentID(6));
    BOOST_CHECK_NE(scc.GetComponentID(0), scc.GetComponentID(2));
    BOOST_CHECK_NE(scc.GetComponentID(0), scc.GetComponentID(5));
    BOOST_CHECK_NE(scc.GetComponentID(2), scc.GetComponentID(5));
}

BOOST_AUTO_TEST_SUITE_END()
