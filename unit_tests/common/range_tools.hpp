#ifndef UNIT_TESTS_RANGE_TOOLS_HPP
#define UNIT_TESTS_RANGE_TOOLS_HPP

#include <boost/test/unit_test.hpp>

#define REQUIRE_SIZE_RANGE(range, ref) BOOST_REQUIRE_EQUAL(range.size(), ref)
#define CHECK_EQUAL_RANGE(range, ...)                                                              \
    do                                                                                             \
    {                                                                                              \
        const auto &lhs = range;                                                                   \
        const auto &rhs = {__VA_ARGS__};                                                           \
        BOOST_CHECK_EQUAL_COLLECTIONS(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());             \
    } while (0)
#define CHECK_EQUAL_COLLECTIONS(lhs, rhs)                                                          \
    do                                                                                             \
    {                                                                                              \
        BOOST_CHECK_EQUAL_COLLECTIONS(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());             \
    } while (0)

#endif // UNIT_TESTS_RANGE_TOOLS_HPP
