#include "util/pool_allocator.hpp"
#include "util/typedefs.hpp"
#include <boost/test/unit_test.hpp>

#include <unordered_map>

BOOST_AUTO_TEST_SUITE(pool_allocator)

using namespace osrm;
using namespace osrm::util;

// in many of these tests we hope on address sanitizer to alert in the case if we are doing something wrong
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
    std::unordered_map<int, int, std::hash<int>, std::equal_to<int>, PoolAllocator<std::pair<const int, int>>> map;
    map[1] = 42;
    BOOST_CHECK_EQUAL(map[1], 42);

    map.clear();

    map[2] = 43;

    BOOST_CHECK_EQUAL(map[2], 43);
}

BOOST_AUTO_TEST_SUITE_END()
