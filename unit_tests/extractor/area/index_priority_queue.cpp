#include "extractor/area/index_priority_queue.hpp"

#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <vector>

BOOST_AUTO_TEST_SUITE(area_util_index_priority_queue)

using namespace osrm;
using namespace osrm::extractor::area;

BOOST_AUTO_TEST_CASE(area_util_index_priority_queue_min_test)
{
    // osrm::util::LogPolicy::GetInstance().SetLevel(logDEBUG);
    // osrm::util::LogPolicy::GetInstance().Unmute();

    std::vector<int> distances{1, 3, 5, 7, 9, 8, 6, 4, 2, 0};

    /******************/
    /* Test MIN Queue */
    /******************/

    IndexPriorityQueue pqmin(
        distances.size(), [&](size_t u, size_t v) -> bool { return distances[u] < distances[v]; });

    for (size_t i = 0; i < distances.size(); ++i)
    {
        pqmin.insert(i);
    }
    for (size_t i = 0; i < distances.size(); ++i)
    {
        BOOST_CHECK_EQUAL(distances[pqmin.pop()], i);
    }
    for (size_t i = 0; i < distances.size(); ++i)
    {
        pqmin.insert(i);
    }

    distances[2] = -1;
    pqmin.decrease(2);
    BOOST_CHECK_EQUAL(pqmin.top(), 2);
    distances[4] = -2;
    pqmin.decrease(4);
    BOOST_CHECK_EQUAL(pqmin.top(), 4);
}

BOOST_AUTO_TEST_CASE(area_util_index_priority_queue_max_test)
{
    /******************/
    /* Test MAX Queue */
    /******************/

    std::vector<int> distances{1, 3, 5, 7, 9, 8, 6, 4, 2, 0};

    IndexPriorityQueue pqmax(
        distances.size(), [&](size_t u, size_t v) -> bool { return distances[u] > distances[v]; });

    for (size_t i = 0; i < distances.size(); ++i)
    {
        pqmax.insert(i);
    }
    for (size_t i = 0; i < distances.size(); ++i)
    {
        BOOST_CHECK_EQUAL(distances[pqmax.pop()], distances.size() - i - 1);
    }
}

BOOST_AUTO_TEST_SUITE_END()
