#include <boost/test/unit_test.hpp>

#include "engine/trip/trip_brute_force.hpp"
#include "engine/trip/trip_dynamic_programming.hpp"
#include "engine/trip/trip_farthest_insertion.hpp"
#include "util/dist_table_wrapper.hpp"
#include "util/typedefs.hpp"

#include <vector>

using osrm::engine::trip::BruteForceTrip;
using osrm::engine::trip::DynamicProgrammingTrip;
using osrm::util::DistTableWrapper;

static EdgeDuration route_cost(const DistTableWrapper<EdgeDuration> &dist,
                               const std::vector<NodeID> &route)
{
    if (route.empty())
        return INVALID_EDGE_DURATION;
    EdgeDuration sum{0};
    const std::size_t n = route.size();
    for (std::size_t i = 0; i < n; ++i)
    {
        const auto a = route[i];
        const auto b = route[(i + 1) % n];
        const auto w = dist(a, b);
        if (w == INVALID_EDGE_DURATION)
            return INVALID_EDGE_DURATION;
        sum += w;
    }
    return sum;
}

BOOST_AUTO_TEST_SUITE(trip_algorithm_tests)

BOOST_AUTO_TEST_CASE(dp_equals_bruteforce_small)
{
    const std::size_t n = 4;
    std::vector<EdgeDuration> table;
    table.reserve(n * n);
    const int raw[n][n] = {{0, 1, 3, 4}, {1, 0, 2, 5}, {3, 2, 0, 1}, {4, 5, 1, 0}};

    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j)
            table.push_back(EdgeDuration{raw[i][j]});

    DistTableWrapper<EdgeDuration> dist(table, n);

    const auto bf = BruteForceTrip(n, dist);
    const auto dp = DynamicProgrammingTrip(n, dist);

    const auto cost_bf = route_cost(dist, bf);
    const auto cost_dp = route_cost(dist, dp);

    BOOST_CHECK(cost_bf != INVALID_EDGE_DURATION);
    BOOST_CHECK(cost_dp != INVALID_EDGE_DURATION);
    BOOST_CHECK_EQUAL(cost_bf, cost_dp);
}

BOOST_AUTO_TEST_CASE(dp_single_node)
{
    const std::size_t n = 1;
    std::vector<EdgeDuration> table = {EdgeDuration{0}};
    DistTableWrapper<EdgeDuration> dist(table, n);

    const auto dp = DynamicProgrammingTrip(n, dist);
    BOOST_CHECK_EQUAL(dp.size(), 1u);
    BOOST_CHECK_EQUAL(dp[0], static_cast<NodeID>(0));
    const auto cost = route_cost(dist, dp);
    BOOST_CHECK_EQUAL(cost, EdgeDuration{0});
}

BOOST_AUTO_TEST_CASE(dp_handles_unreachable_edges)
{
    const std::size_t n = 3;
    std::vector<EdgeDuration> table;
    table.reserve(n * n);

    // 0->1,1->2,2->0 valid; other directions invalid
    for (std::size_t i = 0; i < n; ++i)
    {
        for (std::size_t j = 0; j < n; ++j)
        {
            if (i == j)
                table.push_back(EdgeDuration{0});
            else if (i == 0 && j == 1)
                table.push_back(EdgeDuration{1});
            else if (i == 1 && j == 2)
                table.push_back(EdgeDuration{1});
            else if (i == 2 && j == 0)
                table.push_back(EdgeDuration{1});
            else
                table.push_back(INVALID_EDGE_DURATION);
        }
    }

    DistTableWrapper<EdgeDuration> dist(table, n);
    const auto dp = DynamicProgrammingTrip(n, dist);
    const auto cost_dp = route_cost(dist, dp);
    BOOST_CHECK(cost_dp != INVALID_EDGE_DURATION);
    BOOST_CHECK_EQUAL(cost_dp, EdgeDuration{3});
}

BOOST_AUTO_TEST_CASE(two_opt_improves_suboptimal_route)
{
    const std::size_t n = 4;
    std::vector<EdgeDuration> table;
    table.reserve(n * n);
    const int raw[n][n] = {{0, 10, 1, 10}, {10, 0, 10, 1}, {1, 10, 0, 10}, {10, 1, 10, 0}};

    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j)
            table.push_back(EdgeDuration{raw[i][j]});

    DistTableWrapper<EdgeDuration> dist(table, n);
    const std::vector<NodeID> route = {0, 1, 2, 3};

    const auto optimized = osrm::engine::trip::TwoOptTrip(route, dist);

    BOOST_CHECK_EQUAL(route_cost(dist, route), EdgeDuration{40});
    BOOST_CHECK_EQUAL(route_cost(dist, optimized), EdgeDuration{22});
    BOOST_CHECK(route_cost(dist, optimized) < route_cost(dist, route));
}

BOOST_AUTO_TEST_CASE(two_opt_does_not_worsen_asymmetric_route)
{
    const std::size_t n = 5;
    std::vector<EdgeDuration> table;
    table.reserve(n * n);
    const int raw[n][n] = {
        {0, 7, 31, 38, 40},
        {37, 0, 6, 13, 24},
        {39, 15, 0, 26, 22},
        {8, 6, 5, 0, 15},
        {17, 37, 21, 32, 0},
    };

    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j)
            table.push_back(EdgeDuration{raw[i][j]});

    DistTableWrapper<EdgeDuration> dist(table, n);
    const std::vector<NodeID> route = {0, 1, 2, 3, 4};

    const auto optimized = osrm::engine::trip::TwoOptTrip(route, dist);

    BOOST_CHECK(route_cost(dist, optimized) != INVALID_EDGE_DURATION);
    BOOST_CHECK(route_cost(dist, optimized) <= route_cost(dist, route));
}

BOOST_AUTO_TEST_SUITE_END()
