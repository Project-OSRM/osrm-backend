#include "util/filtered_integer_range.hpp"

#include "../common/range_tools.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(bit_range_test)

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_CASE(filtered_irange_test)
{
    std::vector<bool> filter = {// 0  1      2      3     4     5
                                true,
                                false,
                                false,
                                true,
                                true,
                                false};

    CHECK_EQUAL_RANGE(filtered_irange<std::uint8_t>(0, 2, filter), 0);
    CHECK_EQUAL_RANGE(filtered_irange<std::uint8_t>(1, 4, filter), 3);
    CHECK_EQUAL_RANGE(filtered_irange<std::uint8_t>(1, 5, filter), 3, 4);
    CHECK_EQUAL_RANGE(filtered_irange<std::uint8_t>(0, 6, filter), 0, 3, 4);

    auto empty_1 = filtered_irange<std::uint8_t>(1, 3, filter);
    auto empty_2 = filtered_irange<std::uint8_t>(3, 3, filter);
    auto empty_3 = filtered_irange<std::uint8_t>(4, 4, filter);
    auto empty_4 = filtered_irange<std::uint8_t>(5, 5, filter);
    BOOST_CHECK(empty_1.begin() == empty_1.end());
    BOOST_CHECK(empty_2.begin() == empty_2.end());
    BOOST_CHECK(empty_3.begin() == empty_3.end());
    BOOST_CHECK(empty_4.begin() == empty_4.end());
}

BOOST_AUTO_TEST_SUITE_END()
