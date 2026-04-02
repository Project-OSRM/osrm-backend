#include "contractor/graph_contractor.hpp"

#include "contractor/contractor_graph.hpp"
#include "helper.hpp"

#include <boost/test/unit_test.hpp>

#include <tbb/global_control.h>
#include <tuple>

using namespace osrm;
using namespace osrm::contractor;
using namespace osrm::unit_test;

#define HAS(a, b) BOOST_CHECK(query_graph.FindEdge(a, b) != SPECIAL_EDGEID);
#define NOT(a, b) BOOST_CHECK(query_graph.FindEdge(a, b) == SPECIAL_EDGEID);

BOOST_AUTO_TEST_SUITE(graph_contractor)

BOOST_AUTO_TEST_CASE(contract_graph)
{
    {
        /*
         *  0 - 1
         *
         *  1
         *  |
         *  0
         */
        const ContractorGraph g = makeGraph({{0, 1, 1}}); // start, target, weight

        auto query_graph = g;
        contractGraph(query_graph, {{1}, {1}});

        HAS(0, 1)
        NOT(1, 0)
    }

    {
        /*
         *  0 - 1 - 2
         *
         *    1
         *   / \
         *  0   2
         */

        const ContractorGraph g = makeGraph({{0, 1, 1}, // start, target, weight
                                             {1, 2, 1}});

        auto query_graph = g;
        contractGraph(query_graph, {{1}, {1}, {1}});

        HAS(0, 1)
        HAS(2, 1)

        NOT(1, 0)
        NOT(1, 2)
        NOT(2, 0)
        NOT(0, 2)
    }

    {
        /*
         *  0 - 1
         *   \ /
         *    2
         *
         *    2
         *   /|
         *  1 |
         *   \|
         *    0
         */

        const ContractorGraph g = makeGraph({{0, 1, 1}, // start, target, weight
                                             {1, 2, 1},
                                             {0, 2, 1}});

        auto query_graph = g;
        contractGraph(query_graph, {{1}, {1}, {1}});

        HAS(0, 1)
        HAS(0, 2)
        HAS(1, 2)

        NOT(1, 0)
        NOT(2, 0)
        NOT(2, 1)
    }

    {
        /*
         *  0 - 1
         *  |   |
         *  3 - 2
         *
         *      3
         *    / |
         *  1   |
         *  | X |
         *  0   2
         */

        const ContractorGraph g = makeGraph({{0, 1, 1}, // start, target, weight
                                             {1, 2, 1},
                                             {2, 3, 1},
                                             {3, 0, 1}});

        auto query_graph = g;
        contractGraph(query_graph, {{1}, {1}, {1}, {1}});

        HAS(0, 1)
        HAS(0, 3)
        HAS(2, 1)
        HAS(2, 3)
        HAS(1, 3)

        NOT(1, 0)
        NOT(3, 0)
        NOT(1, 2)
        NOT(3, 2)
        NOT(3, 1)

        NOT(0, 2)
        NOT(2, 0)
    }
}

BOOST_AUTO_TEST_CASE(contract_excludable_graph)
{
    {
        /*
         *  Same as above but 0 is uncontractible
         *
         *  0 - 1
         *  |   |
         *  3 - 2
         *
         *  0
         *  | \
         *  |   2
         *  | X |
         *  1   3
         */

        const ContractorGraph g = makeGraph({{0, 1, 1}, // start, target, weight
                                             {1, 2, 1},
                                             {2, 3, 1},
                                             {3, 0, 1}});

        auto [query_graph, ignore] = contractExcludableGraph(
            g, {{1}, {1}, {1}, {1}}, {{true, true, true, true}, {false, true, true, true}});

        HAS(1, 0)
        HAS(1, 2)
        HAS(3, 0)
        HAS(3, 2)
        HAS(2, 0)

        NOT(0, 1)
        NOT(2, 1)
        NOT(0, 3)
        NOT(2, 3)
        NOT(0, 2)

        NOT(1, 3)
        NOT(3, 1)
    }
}

BOOST_AUTO_TEST_SUITE_END()
