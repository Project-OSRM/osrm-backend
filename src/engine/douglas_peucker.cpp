#include "engine/douglas_peucker.hpp"
#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/integer_range.hpp"
#include "util/web_mercator.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <stack>
#include <utility>

namespace osrm
{
namespace engine
{

// Normed to the thresholds table
std::uint64_t fastPerpendicularDistance(const util::FloatCoordinate &projected_start,
                                        const util::FloatCoordinate &projected_target,
                                        const util::FloatCoordinate &projected)
{
    util::FloatCoordinate projected_point_on_segment;
    std::tie(std::ignore, projected_point_on_segment) =
        util::coordinate_calculation::projectPointOnSegment(
            projected_start, projected_target, projected);
    auto squared_distance = util::coordinate_calculation::squaredEuclideanDistance(
        projected, projected_point_on_segment);
    return squared_distance;
}

std::vector<util::Coordinate> douglasPeucker(std::vector<util::Coordinate>::const_iterator begin,
                                             std::vector<util::Coordinate>::const_iterator end,
                                             const unsigned zoom_level)
{
    BOOST_ASSERT_MSG(zoom_level < detail::DOUGLAS_PEUCKER_THRESHOLDS_SIZE,
                     "unsupported zoom level");

    const std::size_t size = std::distance(begin, end);
    if (size < 2)
    {
        return {};
    }

    std::vector<util::FloatCoordinate> projected_coordinates(size);
    std::transform(begin, end, projected_coordinates.begin(), [](const util::Coordinate coord) {
        return util::web_mercator::fromWGS84(coord);
    });

    std::vector<bool> is_necessary(size, false);
    BOOST_ASSERT(is_necessary.size() >= 2);
    is_necessary.front() = true;
    is_necessary.back() = true;
    using GeometryRange = std::pair<std::size_t, std::size_t>;

    std::stack<GeometryRange> recursion_stack;

    recursion_stack.emplace(0UL, size - 1);

    // mark locations as 'necessary' by divide-and-conquer
    while (!recursion_stack.empty())
    {
        // pop next element
        const GeometryRange pair = recursion_stack.top();
        recursion_stack.pop();
        // sanity checks
        BOOST_ASSERT_MSG(is_necessary[pair.first], "left border must be necessary");
        BOOST_ASSERT_MSG(is_necessary[pair.second], "right border must be necessary");
        BOOST_ASSERT_MSG(pair.second < size, "right border outside of geometry");
        BOOST_ASSERT_MSG(pair.first <= pair.second, "left border on the wrong side");

        std::uint64_t max_distance = 0;
        auto farthest_entry_index = pair.second;

        // sweep over range to find the maximum
        for (auto idx = pair.first + 1; idx != pair.second; ++idx)
        {
            using namespace util::coordinate_calculation;
            const auto distance = fastPerpendicularDistance(projected_coordinates[pair.first],
                                                            projected_coordinates[pair.second],
                                                            projected_coordinates[idx]);
            // found new feasible maximum?
            if (distance > max_distance &&
                distance > detail::DOUGLAS_PEUCKER_THRESHOLDS[zoom_level])
            {
                farthest_entry_index = idx;
                max_distance = distance;
            }
        }

        // check if maximum violates a zoom level dependent threshold
        if (max_distance > detail::DOUGLAS_PEUCKER_THRESHOLDS[zoom_level])
        {
            //  mark idx as necessary
            is_necessary[farthest_entry_index] = true;
            if (pair.first < farthest_entry_index)
            {
                recursion_stack.emplace(pair.first, farthest_entry_index);
            }
            if (farthest_entry_index < pair.second)
            {
                recursion_stack.emplace(farthest_entry_index, pair.second);
            }
        }
    }

    auto simplified_size = std::count(is_necessary.begin(), is_necessary.end(), true);
    std::vector<util::Coordinate> simplified_geometry;
    simplified_geometry.reserve(simplified_size);
    for (auto idx : util::irange<std::size_t>(0UL, size))
    {
        if (is_necessary[idx])
        {
            simplified_geometry.push_back(begin[idx]);
        }
    }

    return simplified_geometry;
}
} // namespace engine
} // namespace osrm
