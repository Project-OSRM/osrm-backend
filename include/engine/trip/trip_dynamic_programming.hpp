#ifndef TRIP_DYNAMIC_PROGRAMMING_HPP
#define TRIP_DYNAMIC_PROGRAMMING_HPP

#include "util/dist_table_wrapper.hpp"
#include "util/typedefs.hpp"

#include "osrm/json_container.hpp"
#include <boost/assert.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

namespace osrm::engine::trip
{

inline std::vector<NodeID>
DynamicProgrammingTrip(const std::size_t number_of_locations,
                       const util::DistTableWrapper<EdgeDuration> &dist_table)
{
    BOOST_ASSERT_MSG(number_of_locations * number_of_locations == dist_table.size(),
                     "number_of_locations and dist_table size do not match");
    if (number_of_locations == 0)
        return {};

    // Guard against shift overflow when building masks
    BOOST_ASSERT_MSG(number_of_locations < (sizeof(unsigned long long) * 8),
                     "number_of_locations too large for DP solver");

    const std::size_t n = number_of_locations;
    const std::size_t one = std::size_t{1};
    const std::size_t mask_count = one << n;

    // dp[mask * n + i] == minimum cost to visit set "mask" and end at i
    const EdgeDuration INF = INVALID_EDGE_DURATION;
    std::vector<EdgeDuration> dp(mask_count * n, INF);

    auto idx = [n](std::size_t mask, std::size_t i) { return mask * n + i; };

    // start at node 0
    dp[idx(one, 0u)] = EdgeDuration{0};

    for (std::size_t mask = 1; mask < mask_count; ++mask)
    {
        // only consider masks that include the starting node (0)
        if ((mask & one) == 0u)
            continue;

        for (std::size_t i = 0; i < n; ++i)
        {
            if ((mask & (one << i)) == 0u)
                continue;

            const auto current = dp[idx(mask, i)];
            if (current == INF)
                continue;

            for (std::size_t j = 0; j < n; ++j)
            {
                if (mask & (one << j))
                    continue; // j already visited

                const auto edge = dist_table(static_cast<NodeID>(i), static_cast<NodeID>(j));
                if (edge == INVALID_EDGE_DURATION)
                    continue;

                const auto next_mask = mask | (one << j);
                const auto cand = current + edge;
                auto &dst = dp[idx(next_mask, j)];
                if (cand < dst)
                {
                    dst = cand;
                }
            }
        }
    }

    const std::size_t full_mask = mask_count - 1;

    // find best end node (returning to 0)
    EdgeDuration best_cost = INF;
    int best_end = -1;
    for (std::size_t i = 0; i < n; ++i)
    {
        const auto val = dp[idx(full_mask, i)];
        if (val == INF)
            continue;
        const auto back = dist_table(static_cast<NodeID>(i), static_cast<NodeID>(0));
        if (back == INVALID_EDGE_DURATION)
            continue;
        const auto total = val + back;
        if (total < best_cost)
        {
            best_cost = total;
            best_end = static_cast<int>(i);
        }
    }

    // If no valid tour was found, return a default order
    if (best_end < 0)
    {
        std::vector<NodeID> fallback(n);
        for (std::size_t i = 0; i < n; ++i)
            fallback[i] = static_cast<NodeID>(i);
        return fallback;
    }

    // Reconstruct path
    std::vector<NodeID> route(n);
    std::size_t mask = full_mask;
    int curr = best_end;
    route[n - 1] = static_cast<NodeID>(curr);

    for (std::size_t pos = n - 1; pos > 0; --pos)
    {
        const std::size_t prev_mask = mask ^ (one << curr);
        int found_prev = -1;
        for (std::size_t p = 0; p < n; ++p)
        {
            if ((prev_mask & (one << p)) == 0u)
                continue;
            const auto prev_val = dp[idx(prev_mask, p)];
            if (prev_val == INF)
                continue;
            const auto edge = dist_table(static_cast<NodeID>(p), static_cast<NodeID>(curr));
            if (edge == INVALID_EDGE_DURATION)
                continue;
            if (prev_val + edge == dp[idx(mask, curr)])
            {
                found_prev = static_cast<int>(p);
                break;
            }
        }

        BOOST_ASSERT_MSG(found_prev >= 0, "Failed to reconstruct DP path");
        route[pos - 1] = static_cast<NodeID>(found_prev);
        curr = found_prev;
        mask = prev_mask;
    }

    return route;
}

} // namespace osrm::engine::trip

#endif // TRIP_DYNAMIC_PROGRAMMING_HPP
