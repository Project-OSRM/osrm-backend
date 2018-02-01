#include "util/connectivity_checksum.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(connectivity_checksum)

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_CASE(connectivity_checksum)
{
    ConnectivityChecksum checksum;

    BOOST_CHECK_EQUAL(checksum.update_checksum(42), 42);

    checksum.process_bit(1);
    BOOST_CHECK_EQUAL(checksum.update_checksum(0), 0x7ebe16cd);

    for (std::size_t i = 0; i < CHAR_BIT; ++i)
        checksum.process_bit(1);
    BOOST_CHECK_EQUAL(checksum.update_checksum(0), 0x1aab001b);

    checksum.process_byte(1);
    BOOST_CHECK_EQUAL(checksum.update_checksum(0), 0x2f7abdf7);
}

BOOST_AUTO_TEST_CASE(connectivity_checksum_bytes_bits)
{
    ConnectivityChecksum checksum1, checksum2;

    checksum1.process_bit(1); // A
    checksum1.process_bit(0);
    checksum1.process_bit(1);
    checksum1.process_bit(0);

    checksum1.process_bit(1); // 9
    checksum1.process_bit(0);
    checksum1.process_bit(0);
    checksum1.process_bit(1);

    checksum1.process_bit(1); // 5
    checksum1.process_bit(0);
    checksum1.process_bit(1);

    checksum2.process_byte(0xA9);
    checksum2.process_byte(0x05);
    BOOST_CHECK_EQUAL(checksum1.update_checksum(0), checksum2.update_checksum(0));
}

BOOST_AUTO_TEST_CASE(connectivity_checksum_diff_size)
{
    ConnectivityChecksum checksum1, checksum2;

    checksum1.process_byte(1);
    checksum1.process_byte(2);
    checksum1.process_byte(3);
    checksum1.process_byte(4);
    checksum1.process_byte(5);
    checksum1.process_bit(1);

    checksum2.process_byte(1);
    checksum2.process_byte(2);
    checksum2.process_byte(3);
    checksum2.update_checksum(0);
    checksum2.process_byte(4);
    checksum2.process_byte(5);
    checksum2.process_bit(1);

    BOOST_CHECK_EQUAL(checksum1.update_checksum(0), checksum2.update_checksum(0));
}

BOOST_AUTO_TEST_CASE(connectivity_checksum_parallel)
{
    ConnectivityChecksum expected, checksum1, checksum2;

    expected.process_byte(1);
    expected.process_byte(2);
    expected.process_byte(3);
    expected.process_byte(4);
    expected.process_byte(5);
    expected.process_bit(1);

    checksum1.process_byte(1);
    checksum1.process_byte(2);
    checksum1.process_byte(3);

    checksum2.process_byte(4);
    checksum2.process_byte(5);
    checksum2.process_bit(1);

    BOOST_CHECK_EQUAL(expected.update_checksum(0),
                      checksum2.update_checksum(checksum1.update_checksum(0)));
}

BOOST_AUTO_TEST_SUITE_END()
