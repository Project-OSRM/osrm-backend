#include "partition/reorder_first_last.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <functional>
#include <iterator>
#include <utility>
#include <vector>

using namespace osrm::partition;

BOOST_AUTO_TEST_SUITE(reorder_first_last)

BOOST_AUTO_TEST_CASE(reordering_one_is_equivalent_to_min_and_max)
{
    std::vector<int> range{9, 0, 8, 1, 7, 2, 6, 3, 5, 4};

    reorderFirstLast(begin(range), end(range), 1, std::less<>{});

    BOOST_CHECK_EQUAL(range.front(), 0);
    BOOST_CHECK_EQUAL(range.back(), 9);

    reorderFirstLast(begin(range), end(range), 1, std::greater<>{});

    BOOST_CHECK_EQUAL(range.front(), 9);
    BOOST_CHECK_EQUAL(range.back(), 0);
}

BOOST_AUTO_TEST_CASE(reordering_n_shuffles_n_smallest_to_front_n_largest_to_back)
{
    std::vector<int> range{9, 3, 8, 2};

    reorderFirstLast(begin(range), end(range), 2, std::less<>{});

    // Smallest at front, but: no ordering guarantee in that subrange!
    BOOST_CHECK((range[0] == 2 && range[1] == 3) || (range[0] == 3 && range[1] == 2));

    // Largest at back, but: no ordering guarantee in that subrange!
    BOOST_CHECK((range[2] == 8 && range[3] == 9) || (range[2] == 9 && range[3] == 8));
}

BOOST_AUTO_TEST_CASE(reordering_n_with_iterators)
{
    std::vector<int> range{9, 3, 8, 2};

    reorderFirstLast(begin(range), end(range), 1, std::less<>{});

    BOOST_CHECK_EQUAL(range.front(), 2);
    BOOST_CHECK_EQUAL(range.back(), 9);
}

BOOST_AUTO_TEST_SUITE_END()
