#include "util/bit_range.hpp"

#include "../common/range_tools.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(bit_range_test)

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_CASE(bit_range_8bit_test)
{
    std::uint8_t value_1 = (1UL << 0) | (1UL << 1) | (1UL << 5) | (1UL << 7);
    std::uint8_t value_2 = (1UL << 0);
    std::uint8_t value_3 =
        (1UL << 0) | (1UL << 1) | (1UL << 2) | (1UL << 3) | (1UL << 4) | (1UL << 5);

    CHECK_EQUAL_RANGE(makeBitRange<std::uint8_t>(value_1), 7, 5, 1, 0);
    CHECK_EQUAL_RANGE(makeBitRange<std::uint8_t>(value_2), 0);
    CHECK_EQUAL_RANGE(makeBitRange<std::uint8_t>(value_3), 5, 4, 3, 2, 1, 0);

    BOOST_CHECK_EQUAL(makeBitRange<std::uint8_t>(value_3).size(), 6);
    BOOST_CHECK_EQUAL(makeBitRange<std::uint8_t>(0).size(), 0);
}

BOOST_AUTO_TEST_CASE(bit_range_16bit_test)
{
    std::uint16_t value_1 = (1UL << 0) | (1UL << 1) | (1UL << 9) | (1UL << 15);
    std::uint16_t value_2 = (1UL << 0);
    std::uint16_t value_3 =
        (1UL << 0) | (1UL << 1) | (1UL << 2) | (1UL << 3) | (1UL << 4) | (1UL << 5);

    CHECK_EQUAL_RANGE(makeBitRange<std::uint16_t>(value_1), 15, 9, 1, 0);
    CHECK_EQUAL_RANGE(makeBitRange<std::uint16_t>(value_2), 0);
    CHECK_EQUAL_RANGE(makeBitRange<std::uint16_t>(value_3), 5, 4, 3, 2, 1, 0);

    BOOST_CHECK_EQUAL(makeBitRange<std::uint16_t>(value_3).size(), 6);
    BOOST_CHECK_EQUAL(makeBitRange<std::uint16_t>(0).size(), 0);
}

BOOST_AUTO_TEST_CASE(bit_range_32bit_test)
{
    std::uint32_t value_1 = (1UL << 0) | (1UL << 1) | (1UL << 17) | (1UL << 31);
    std::uint32_t value_2 = (1UL << 0);
    std::uint32_t value_3 =
        (1UL << 0) | (1UL << 1) | (1UL << 2) | (1UL << 3) | (1UL << 4) | (1UL << 5);

    CHECK_EQUAL_RANGE(makeBitRange<std::uint32_t>(value_1), 31, 17, 1, 0);
    CHECK_EQUAL_RANGE(makeBitRange<std::uint32_t>(value_2), 0);
    CHECK_EQUAL_RANGE(makeBitRange<std::uint32_t>(value_3), 5, 4, 3, 2, 1, 0);

    BOOST_CHECK_EQUAL(makeBitRange<std::uint32_t>(value_3).size(), 6);
    BOOST_CHECK_EQUAL(makeBitRange<std::uint32_t>(0).size(), 0);
}

BOOST_AUTO_TEST_CASE(bit_range_64bit_test)
{
    std::uint64_t value_1 = (1ULL << 0) | (1ULL << 1) | (1ULL << 33) | (1ULL << 63);
    std::uint64_t value_2 = (1ULL << 0);
    std::uint64_t value_3 =
        (1ULL << 0) | (1ULL << 1) | (1ULL << 2) | (1ULL << 3) | (1ULL << 4) | (1ULL << 5);

    CHECK_EQUAL_RANGE(makeBitRange<std::uint64_t>(value_1), 63, 33, 1, 0);
    CHECK_EQUAL_RANGE(makeBitRange<std::uint64_t>(value_2), 0);
    CHECK_EQUAL_RANGE(makeBitRange<std::uint64_t>(value_3), 5, 4, 3, 2, 1, 0);

    BOOST_CHECK_EQUAL(makeBitRange<std::uint64_t>(value_3).size(), 6);
    BOOST_CHECK_EQUAL(makeBitRange<std::uint64_t>(0).size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
