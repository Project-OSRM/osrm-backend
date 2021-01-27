#include "util/hilbert_value.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(hilbert_values_test)

using namespace osrm::util;

BOOST_AUTO_TEST_CASE(hilbert_linearization_2bit_test)
{
    const std::uint8_t expected[4][4] =
        // 0 1  2   3  x  // y
        {{5, 6, 9, 10},   // 3  ┌┐┌┐
         {4, 7, 8, 11},   // 2  │└┘│
         {3, 2, 13, 12},  // 1  └┐┌┘
         {0, 1, 14, 15}}; // 0  ─┘└─

    auto bit2_8 = HilbertToLinear<2, std::uint8_t, std::uint16_t>;
    auto bit2_16 = HilbertToLinear<2, std::uint16_t, std::uint32_t>;
    auto bit2_32 = HilbertToLinear<2, std::uint32_t, std::uint64_t>;
    auto bit32_32 = HilbertToLinear<32, std::uint32_t, std::uint64_t>;
    for (auto x = 0; x < 4; ++x)
        for (auto y = 0; y < 4; ++y)
        {
            BOOST_CHECK_EQUAL(bit2_8(x << 6, y << 6), expected[3 - y][x]);
            BOOST_CHECK_EQUAL(bit2_16(x << 14, y << 14), expected[3 - y][x]);
            BOOST_CHECK_EQUAL(bit2_32(x << 30, y << 30), expected[3 - y][x]);
            BOOST_CHECK_EQUAL(bit32_32(x << 30, y << 30) >> 60, expected[3 - y][x]);
        }
}

BOOST_AUTO_TEST_CASE(hilbert_linearization_3bit_test)
{
    const std::uint8_t expected[8][8] =
        // 0   1   2   3   4   5   6   7 x // y
        {{21, 22, 25, 26, 37, 38, 41, 42}, // 7  ┌┐┌┐┌┐┌┐
         {20, 23, 24, 27, 36, 39, 40, 43}, // 6  │└┘││└┘│
         {19, 18, 29, 28, 35, 34, 45, 44}, // 5  └┐┌┘└┐┌┘
         {16, 17, 30, 31, 32, 33, 46, 47}, // 4  ┌┘└──┘└┐
         {15, 12, 11, 10, 53, 52, 51, 48}, // 3  │┌─┐┌─┐│
         {14, 13, 8, 9, 54, 55, 50, 49},   // 2  └┘ ││ └┘
         {1, 2, 7, 6, 57, 56, 61, 62},     // 1  ┌┐ ││ ┌┐
         {0, 3, 4, 5, 58, 59, 60, 63}};    // 0  │└─┘└─┘│

    auto bit3_8 = HilbertToLinear<3, std::uint8_t, std::uint16_t>;
    auto bit3_16 = HilbertToLinear<3, std::uint16_t, std::uint32_t>;
    auto bit3_32 = HilbertToLinear<3, std::uint32_t, std::uint64_t>;
    auto bit32_32 = HilbertToLinear<32, std::uint32_t, std::uint64_t>;
    for (auto x = 0; x < 8; ++x)
        for (auto y = 0; y < 8; ++y)
        {
            BOOST_CHECK_EQUAL(bit3_8(x << 5, y << 5), expected[7 - y][x]);
            BOOST_CHECK_EQUAL(bit3_16(x << 13, y << 13), expected[7 - y][x]);
            BOOST_CHECK_EQUAL(bit3_32(x << 29, y << 29), expected[7 - y][x]);
            BOOST_CHECK_EQUAL(bit32_32(x << 29, y << 29) >> 58, expected[7 - y][x]);
        }
}

BOOST_AUTO_TEST_CASE(hilbert_linearization_32bit_test)
{
    auto bit32 = HilbertToLinear<32, std::uint32_t, std::uint64_t>;

    BOOST_CHECK_EQUAL(bit32(0x00000000, 0x00000000), 0x0000000000000000);
    BOOST_CHECK_EQUAL(bit32(0x7fffffff, 0x00000000), 0x1555555555555555);
    BOOST_CHECK_EQUAL(bit32(0x80000000, 0x00000000), 0xeaaaaaaaaaaaaaaa);
    BOOST_CHECK_EQUAL(bit32(0xffffffff, 0x00000000), 0xffffffffffffffff);

    BOOST_CHECK_EQUAL(bit32(0x00000000, 0x7fffffff), 0x3fffffffffffffff);
    BOOST_CHECK_EQUAL(bit32(0x7fffffff, 0x7fffffff), 0x2aaaaaaaaaaaaaaa);
    BOOST_CHECK_EQUAL(bit32(0x80000000, 0x7fffffff), 0xd555555555555555);
    BOOST_CHECK_EQUAL(bit32(0xffffffff, 0x7fffffff), 0xc000000000000000);

    BOOST_CHECK_EQUAL(bit32(0x00000000, 0x80000000), 0x4000000000000000);
    BOOST_CHECK_EQUAL(bit32(0x7fffffff, 0x80000000), 0x7fffffffffffffff);
    BOOST_CHECK_EQUAL(bit32(0x80000000, 0x80000000), 0x8000000000000000);
    BOOST_CHECK_EQUAL(bit32(0xffffffff, 0x80000000), 0xbfffffffffffffff);

    BOOST_CHECK_EQUAL(bit32(0x00000000, 0xffffffff), 0x5555555555555555);
    BOOST_CHECK_EQUAL(bit32(0x7fffffff, 0xffffffff), 0x6aaaaaaaaaaaaaaa);
    BOOST_CHECK_EQUAL(bit32(0x80000000, 0xffffffff), 0x9555555555555555);
    BOOST_CHECK_EQUAL(bit32(0xffffffff, 0xffffffff), 0xaaaaaaaaaaaaaaaa);
}

BOOST_AUTO_TEST_SUITE_END()
