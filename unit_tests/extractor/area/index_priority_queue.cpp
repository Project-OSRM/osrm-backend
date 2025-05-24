#include "extractor/area/index_priority_queue.hpp"

#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <vector>

BOOST_AUTO_TEST_SUITE(area_util_index_priority_queue)

using namespace osrm;
using namespace osrm::extractor::area;

namespace
{
const std::vector<int> distances{1, 3, 5, 7, 9, 8, 6, 4, 2, 0};
auto min_comp = [](size_t u, size_t v) -> bool { return distances[u] < distances[v]; };
auto max_comp = [](size_t u, size_t v) -> bool { return distances[u] > distances[v]; };
} // namespace

BOOST_AUTO_TEST_CASE(size_test)
{
    IndexPriorityQueue pq(distances.size(), min_comp);
    BOOST_CHECK(pq.empty());
    BOOST_CHECK_EQUAL(pq.size(), 0);
    pq.insert(0);
    BOOST_CHECK(!pq.empty());
    BOOST_CHECK_EQUAL(pq.size(), 1);
    pq.top();
    BOOST_CHECK(!pq.empty());
    BOOST_CHECK_EQUAL(pq.size(), 1);
    pq.pop();
    BOOST_CHECK(pq.empty());
    BOOST_CHECK_EQUAL(pq.size(), 0);
}

BOOST_AUTO_TEST_CASE(min_comp_test)
{
    // osrm::util::LogPolicy::GetInstance().SetLevel(logDEBUG);
    // osrm::util::LogPolicy::GetInstance().Unmute();

    IndexPriorityQueue pq(distances.size(), min_comp);

    for (size_t i = 0; i < distances.size(); ++i)
    {
        pq.insert(i);
    }
    for (size_t i = 0; i < distances.size(); ++i)
    {
        BOOST_CHECK_EQUAL(distances[pq.pop()], i);
    }
}

BOOST_AUTO_TEST_CASE(max_comp_test)
{
    IndexPriorityQueue pq(distances.size(), max_comp);

    for (size_t i = 0; i < distances.size(); ++i)
    {
        pq.insert(i);
    }
    for (size_t i = 0; i < distances.size(); ++i)
    {
        BOOST_CHECK_EQUAL(distances[pq.pop()], distances.size() - i - 1);
    }
}

BOOST_AUTO_TEST_CASE(decrease_test)
{
    // osrm::util::LogPolicy::GetInstance().SetLevel(logDEBUG);
    // osrm::util::LogPolicy::GetInstance().Unmute();

    auto copy = distances;

    IndexPriorityQueue pq(copy.size(),
                          [&](size_t u, size_t v) -> bool { return copy[u] < copy[v]; });
    for (size_t i = 0; i < distances.size(); ++i)
    {
        pq.insert(i);
    }
    copy[2] = -1;
    pq.decrease(2);
    BOOST_CHECK_EQUAL(copy[pq.top()], -1);

    copy[4] = -2;
    pq.decrease(4);
    BOOST_CHECK_EQUAL(copy[pq.top()], -2);

    BOOST_CHECK_EQUAL(copy[pq.pop()], -2);
    BOOST_CHECK_EQUAL(copy[pq.pop()], -1);
    BOOST_CHECK_EQUAL(copy[pq.pop()], 0);
}

BOOST_AUTO_TEST_SUITE_END()
