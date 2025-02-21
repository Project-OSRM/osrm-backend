#include "util/pool_allocator.hpp"
#include "util/typedefs.hpp"
#include <boost/test/unit_test.hpp>

#include <unordered_map>

BOOST_AUTO_TEST_SUITE(pool_allocator)

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_CASE(test_align_up)
{
    BOOST_CHECK_EQUAL(align_up(5, 4), 8);
    BOOST_CHECK_EQUAL(align_up(9, 8), 16);
    BOOST_CHECK_EQUAL(align_up(17, 16), 32);
    BOOST_CHECK_EQUAL(align_up(4, 4), 4);
    BOOST_CHECK_EQUAL(align_up(8, 8), 8);
    BOOST_CHECK_EQUAL(align_up(16, 16), 16);
    BOOST_CHECK_EQUAL(align_up(32, 16), 32);
    BOOST_CHECK_EQUAL(align_up(0, 4), 0);
    BOOST_CHECK_EQUAL(align_up(0, 8), 0);
    BOOST_CHECK_EQUAL(align_up(0, 16), 0);
    BOOST_CHECK_EQUAL(align_up(1000000, 256), 1000192);
    BOOST_CHECK_EQUAL(align_up(999999, 512), 1000448);
    BOOST_CHECK_EQUAL(align_up(123456789, 1024), 123457536);
    BOOST_CHECK_EQUAL(align_up(0, 1), 0);
    BOOST_CHECK_EQUAL(align_up(5, 1), 5);
    BOOST_CHECK_EQUAL(align_up(123456, 1), 123456);
}

BOOST_AUTO_TEST_CASE(test_get_next_power_of_two_exponent)
{
    BOOST_CHECK_EQUAL(get_next_power_of_two_exponent(1), 0);
    BOOST_CHECK_EQUAL(get_next_power_of_two_exponent(2), 1);
    BOOST_CHECK_EQUAL(get_next_power_of_two_exponent(4), 2);
    BOOST_CHECK_EQUAL(get_next_power_of_two_exponent(8), 3);
    BOOST_CHECK_EQUAL(get_next_power_of_two_exponent(16), 4);
    BOOST_CHECK_EQUAL(get_next_power_of_two_exponent(3), 2);
    BOOST_CHECK_EQUAL(get_next_power_of_two_exponent(5), 3);
    BOOST_CHECK_EQUAL(get_next_power_of_two_exponent(9), 4);
    BOOST_CHECK_EQUAL(get_next_power_of_two_exponent(15), 4);
    BOOST_CHECK_EQUAL(get_next_power_of_two_exponent(17), 5);
    BOOST_CHECK_EQUAL(get_next_power_of_two_exponent(1), 0);
    BOOST_CHECK_EQUAL(get_next_power_of_two_exponent(SIZE_MAX), sizeof(size_t) * 8);
}

// in many of these tests we hope on address sanitizer to alert in the case if we are doing
// something wrong
BOOST_AUTO_TEST_CASE(smoke)
{
    PoolAllocator<int> pool;
    auto ptr = pool.allocate(1);
    *ptr = 42;
    BOOST_CHECK_NE(ptr, nullptr);
    pool.deallocate(ptr, 1);

    ptr = pool.allocate(2);
    *ptr = 42;
    *(ptr + 1) = 43;
    BOOST_CHECK_NE(ptr, nullptr);
    pool.deallocate(ptr, 2);
}

BOOST_AUTO_TEST_CASE(a_lot_of_items)
{
    PoolAllocator<int> pool;
    auto ptr = pool.allocate(2048);
    for (int i = 0; i < 2048; ++i)
    {
        ptr[i] = i;
    }

    for (int i = 0; i < 2048; ++i)
    {
        BOOST_CHECK_EQUAL(ptr[i], i);
    }

    pool.deallocate(ptr, 2048);
}

BOOST_AUTO_TEST_CASE(copy)
{
    PoolAllocator<int> pool;
    auto ptr = pool.allocate(1);
    *ptr = 42;
    BOOST_CHECK_NE(ptr, nullptr);
    pool.deallocate(ptr, 1);

    PoolAllocator<int> pool2(pool);
    ptr = pool2.allocate(1);
    *ptr = 42;
    BOOST_CHECK_NE(ptr, nullptr);
    pool2.deallocate(ptr, 1);
}

BOOST_AUTO_TEST_CASE(move)
{
    PoolAllocator<int> pool;
    auto ptr = pool.allocate(1);
    *ptr = 42;
    BOOST_CHECK_NE(ptr, nullptr);
    pool.deallocate(ptr, 1);

    PoolAllocator<int> pool2(std::move(pool));
    ptr = pool2.allocate(1);
    *ptr = 42;
    BOOST_CHECK_NE(ptr, nullptr);
    pool2.deallocate(ptr, 1);
}

BOOST_AUTO_TEST_CASE(unordered_map)
{
    std::unordered_map<int,
                       int,
                       std::hash<int>,
                       std::equal_to<int>,
                       PoolAllocator<std::pair<const int, int>>>
        map;
    map[1] = 42;
    BOOST_CHECK_EQUAL(map[1], 42);

    map.clear();

    map[2] = 43;

    BOOST_CHECK_EQUAL(map[2], 43);
}

BOOST_AUTO_TEST_CASE(alignment)
{
    PoolAllocator<char> pool_char;
    PoolAllocator<double> pool_double;

    auto ptr_char = pool_char.allocate(1);
    auto ptr_double = pool_double.allocate(1);

    BOOST_CHECK_NE(ptr_double, nullptr);
    BOOST_CHECK_EQUAL(reinterpret_cast<uintptr_t>(ptr_double) % alignof(double), 0);
    BOOST_CHECK_NE(ptr_char, nullptr);
    BOOST_CHECK_EQUAL(reinterpret_cast<uintptr_t>(ptr_char) % alignof(char), 0);

    pool_char.deallocate(ptr_char, 1);
    pool_double.deallocate(ptr_double, 1);

    ptr_char = pool_char.allocate(2);
    ptr_double = pool_double.allocate(1);

    BOOST_CHECK_NE(ptr_double, nullptr);
    BOOST_CHECK_EQUAL(reinterpret_cast<uintptr_t>(ptr_double) % alignof(double), 0);
    BOOST_CHECK_NE(ptr_char, nullptr);
    BOOST_CHECK_EQUAL(reinterpret_cast<uintptr_t>(ptr_char) % alignof(char), 0);

    pool_char.deallocate(ptr_char, 2);
    pool_double.deallocate(ptr_double, 1);
}

BOOST_AUTO_TEST_SUITE_END()
