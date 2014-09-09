#include <boost/test/unit_test.hpp>
#include <boost/assert.hpp>
#include <boost/test/test_case_template.hpp>

#include <iostream>

#include "../../Algorithms/MinCostSchematization.h"
#include "TestUtils.h"

BOOST_AUTO_TEST_SUITE(min_cost_schematization)

// Check the merging of edges of the same direction
BOOST_AUTO_TEST_CASE(edge_simplification_test)
{
    SchematizedPlane plane(2, 0.2);
    MinCostSchematization mcs(plane);

    TestRaster raster(0, 50, 0, 50, 10, 10);
    std::vector<SymbolicCoordinate> path = {
        raster.symbolic(0, 0),
        raster.symbolic(1, 1),
        raster.symbolic(2, 2),
        raster.symbolic(3, 1),
        raster.symbolic(4, 0),
        raster.symbolic(4, 1),
        raster.symbolic(4, 2),
        raster.symbolic(5, 3),
        raster.symbolic(6, 2),
        raster.symbolic(7, 2),
        raster.symbolic(8, 2)
    };

    std::vector<unsigned> directions;
    std::vector<unsigned> alternative_directions;
    std::vector<SubPath::EdgeType> types;
    mcs.computeEdges(path, directions, alternative_directions, types);

    std::vector<SymbolicCoordinate> correct_edges =
    {
        raster.symbolic(0, 0),
        raster.symbolic(2, 2),
        raster.symbolic(4, 0),
        raster.symbolic(4, 2),
        raster.symbolic(5, 3),
        raster.symbolic(6, 2),
        raster.symbolic(8, 2)
    };

    BOOST_CHECK_EQUAL(correct_edges.size(), path.size());
    for (unsigned i = 0; i < correct_edges.size(); i++)
    {
        BOOST_CHECK_EQUAL(correct_edges[i], path[i]);
    }
}

void test_stripe_expansion(const MinCostSchematization& mcs,
                           const std::vector<unsigned>& node_to_y_order,
                           const std::vector<unsigned>& y_order_to_node,
                           const std::vector<unsigned>& y_order_to_stripe,
                           const std::vector<SubPath::Stripe>& stripes,
                           const std::vector<SubPath::EdgeType>& edge_types,
                           const std::vector<bool>& correct_result)
{
    /*
     * Check if input data valid.
     */
    BOOST_ASSERT(node_to_y_order.size() == edge_types.size() + 1);
    BOOST_ASSERT(node_to_y_order.size() == y_order_to_node.size());
    BOOST_ASSERT(node_to_y_order.size() == y_order_to_stripe.size());

    for (unsigned node_idx = 0; node_idx < node_to_y_order.size(); node_idx++)
    {
        BOOST_ASSERT(y_order_to_node[node_to_y_order[node_idx]] == node_idx);
    }

    unsigned last_stripe = 0;
    for (auto s : y_order_to_stripe)
    {
        BOOST_ASSERT(s >= last_stripe);
        last_stripe = s;
    }

    unsigned last_end = 0;
    for (auto s: stripes)
    {
        BOOST_ASSERT(last_end == s.front());
        last_end = s.back()+1;
    }
    BOOST_ASSERT(last_end == node_to_y_order.size());

    /* End of sanity check, real testing starts here. */

    std::vector<bool> expand_stripe;
    mcs.computeMinSchematizationCost(y_order_to_node,
                                     node_to_y_order,
                                     stripes,
                                     y_order_to_stripe,
                                     edge_types,
                                     expand_stripe);

    BOOST_CHECK_EQUAL(expand_stripe.size(), stripes.size());

    for (unsigned i = 0; i < expand_stripe.size(); i++)
    {
        BOOST_CHECK_EQUAL(expand_stripe[i], correct_result[i]);
    }
}

BOOST_AUTO_TEST_CASE(cost_computation_regressions)
{
    SchematizedPlane plane(2, 0.2);
    MinCostSchematization mcs(plane);

    /*
     * 2 ---x------------------v
     *     / \
     * 1 -x---\----------------^
     *         \
     * 0 -------x--------------v
     */
    test_stripe_expansion(mcs,
                          {1, 2, 0},
                          {2, 0, 1},
                          {0, 1, 2},
                          {boost::irange(0u, 1u), boost::irange(1u, 2u),  boost::irange(2u, 3u)},
                          {SubPath::EDGE_HORIZONTAL, SubPath::EDGE_VERTICAL},
                          {true, true, false});

    /*
     * 1 ---x----------x----^
     *       \        /
     * 0 -----x______x------v
     */
    test_stripe_expansion(mcs,
                          {2, 0, 1, 3},
                          {1, 2, 0, 3},
                          {0, 0, 1, 1},
                          {boost::irange(0u, 2u), boost::irange(2u, 4u)},
                          {SubPath::EDGE_VERTICAL, SubPath::EDGE_HORIZONTAL, SubPath::EDGE_VERTICAL},
                          {true, true});
    /*
     * 2 ----------------x-^
     *                  /
     * 1 -------x------x---v
     *       /     \  /
     * 0 --x---------x-----^
     */
    test_stripe_expansion(mcs,
                          {0, 2, 1, 3, 4},
                          {0, 2, 1, 3, 4},
                          {0, 0, 1, 1, 2},
                          {boost::irange(0u, 2u), boost::irange(2u, 4u), boost::irange(4u, 5u)},
                          {SubPath::EDGE_HORIZONTAL, SubPath::EDGE_HORIZONTAL, SubPath::EDGE_VERTICAL, SubPath::EDGE_VERTICAL},
                          {true, false, true});
    /*
     * 2 --------------x---^
     *                /
     * 1 -------x----x-----v
     *       /     \ |
     * 0 --x---------x-----v
     */
    test_stripe_expansion(mcs,
                          {0, 2, 1, 3, 4},
                          {0, 2, 1, 3, 4},
                          {0, 0, 1, 1, 2},
                          {boost::irange(0u, 2u), boost::irange(2u, 4u), boost::irange(4u, 5u)},
                          {SubPath::EDGE_HORIZONTAL, SubPath::EDGE_HORIZONTAL, SubPath::EDGE_STRICTLY_VERTICAL, SubPath::EDGE_VERTICAL},
                          {true, true, true});
}

BOOST_AUTO_TEST_SUITE_END()
