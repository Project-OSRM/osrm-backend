#include "util/deallocating_vector.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <array>

BOOST_AUTO_TEST_SUITE(deallocating_vector)

BOOST_AUTO_TEST_CASE(sort_uses_random_access_iterator_ordering)
{
    osrm::util::DeallocatingVector<unsigned> values;
    values.push_back(3);
    values.push_back(1);
    values.push_back(2);

    BOOST_CHECK(values.begin() < values.end());
    std::ranges::sort(values);
    const std::array<unsigned, 3> expected{1, 2, 3};
    BOOST_CHECK(std::ranges::equal(values, expected));
}

BOOST_AUTO_TEST_SUITE_END()
