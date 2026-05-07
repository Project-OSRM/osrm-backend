#include "util/browse_resistant_cache.hpp"

#include <boost/test/unit_test.hpp>
#include <string>

using namespace osrm::util;

namespace
{
auto identity_cost = [](const int &) -> size_t { return 1; };
using TestCache = BrowseResistantCache<int, int, decltype(identity_cost)>;
} // namespace

BOOST_AUTO_TEST_SUITE(browse_resistant_cache_test)

BOOST_AUTO_TEST_CASE(lookup_miss)
{
    TestCache cache(10, 40, identity_cost);
    BOOST_CHECK(cache.get(42) == nullptr);
    BOOST_CHECK_EQUAL(cache.size(), 0);
}

BOOST_AUTO_TEST_CASE(insert_and_first_lookup_in_l1)
{
    TestCache cache(10, 40, identity_cost);
    cache.insert(1, 100);
    BOOST_CHECK_EQUAL(cache.l1_size(), 1);
    BOOST_CHECK_EQUAL(cache.l2_size(), 0);

    // First access promotes to L2
    auto *val = cache.get(1);
    BOOST_REQUIRE(val != nullptr);
    BOOST_CHECK_EQUAL(*val, 100);
    BOOST_CHECK_EQUAL(cache.l1_size(), 0);
    BOOST_CHECK_EQUAL(cache.l2_size(), 1);
}

BOOST_AUTO_TEST_CASE(l2_lru_refresh)
{
    TestCache cache(10, 40, identity_cost);
    cache.insert(1, 100);
    cache.get(1); // promote to L2

    cache.insert(2, 200);
    cache.get(2); // promote to L2

    // Both in L2, access key 1 again to refresh it
    auto *val = cache.get(1);
    BOOST_REQUIRE(val != nullptr);
    BOOST_CHECK_EQUAL(*val, 100);
    BOOST_CHECK_EQUAL(cache.l2_size(), 2);
}

BOOST_AUTO_TEST_CASE(l1_eviction_removes)
{
    // L1 budget = 2 entries, L2 budget = 10
    TestCache cache(2, 10, identity_cost);
    cache.insert(1, 100);
    cache.insert(2, 200);
    // L1 is at capacity (2)
    BOOST_CHECK_EQUAL(cache.l1_size(), 2);

    // Insert a third, should evict oldest (key 1)
    cache.insert(3, 300);
    BOOST_CHECK_EQUAL(cache.l1_size(), 2);
    BOOST_CHECK(cache.get(1) == nullptr); // key 1 was evicted and gone

    // keys 2 and 3: key 2 was just promoted by get(1) failing... no.
    // Actually get(1) returned nullptr so no promotion. Let's check key 2 is still findable.
    // key 2 is in L1, get promotes to L2
    auto *val = cache.get(2);
    BOOST_REQUIRE(val != nullptr);
    BOOST_CHECK_EQUAL(*val, 200);
}

BOOST_AUTO_TEST_CASE(l2_eviction_demotes_to_l1)
{
    // L1 budget = 5, L2 budget = 2 entries
    TestCache cache(5, 2, identity_cost);

    // Fill L2 with two entries
    cache.insert(1, 100);
    cache.get(1); // promote to L2
    cache.insert(2, 200);
    cache.get(2); // promote to L2
    BOOST_CHECK_EQUAL(cache.l2_size(), 2);
    BOOST_CHECK_EQUAL(cache.l1_size(), 0);

    // Add a third and promote — should evict L2 LRU (key 1) to L1
    cache.insert(3, 300);
    cache.get(3); // promote to L2, L2 now over budget
    BOOST_CHECK_EQUAL(cache.l2_size(), 2);
    BOOST_CHECK_EQUAL(cache.l1_size(), 1);

    // key 1 should still be findable (demoted to L1, then re-promoted on access)
    auto *val = cache.get(1);
    BOOST_REQUIRE(val != nullptr);
    BOOST_CHECK_EQUAL(*val, 100);
}

BOOST_AUTO_TEST_CASE(memory_budget_enforcement)
{
    auto string_cost = [](const std::string &s) -> size_t { return s.size(); };
    using StrCache = BrowseResistantCache<int, std::string, decltype(string_cost)>;

    // L1 budget = 10 bytes, L2 budget = 20 bytes
    StrCache cache(10, 20, string_cost);
    cache.insert(1, "hello"); // 5 bytes
    cache.insert(2, "world"); // 5 bytes
    BOOST_CHECK_EQUAL(cache.l1_memory_used(), 10);

    // This should evict key 1 from L1
    cache.insert(3, "foobar"); // 6 bytes, total would be 16 > 10
    BOOST_CHECK(cache.l1_memory_used() <= 10);
    BOOST_CHECK(cache.get(1) == nullptr);
}

BOOST_AUTO_TEST_CASE(clear_resets_everything)
{
    TestCache cache(10, 40, identity_cost);
    cache.insert(1, 100);
    cache.get(1);
    cache.insert(2, 200);
    BOOST_CHECK(cache.size() > 0);

    cache.clear();
    BOOST_CHECK_EQUAL(cache.size(), 0);
    BOOST_CHECK_EQUAL(cache.l1_memory_used(), 0);
    BOOST_CHECK_EQUAL(cache.l2_memory_used(), 0);
    BOOST_CHECK(cache.get(1) == nullptr);
}

BOOST_AUTO_TEST_CASE(browse_resistance)
{
    // L1 budget = 3, L2 budget = 10
    TestCache cache(3, 10, identity_cost);

    // Simulate a sequential scan of 20 keys — none accessed twice
    for (int i = 0; i < 20; ++i)
    {
        cache.insert(i, i * 10);
    }

    // None of the scan keys should have made it to L2
    BOOST_CHECK_EQUAL(cache.l2_size(), 0);
    // Only last few survive in L1
    BOOST_CHECK(cache.l1_size() <= 3);

    // Now insert a "hot" key and access it twice
    cache.insert(100, 999);
    auto *val = cache.get(100); // promotes to L2
    BOOST_REQUIRE(val != nullptr);
    BOOST_CHECK_EQUAL(*val, 999);
    BOOST_CHECK_EQUAL(cache.l2_size(), 1);

    // Continue scanning — hot key should survive in L2
    for (int i = 20; i < 40; ++i)
    {
        cache.insert(i, i * 10);
    }
    val = cache.get(100);
    BOOST_REQUIRE(val != nullptr);
    BOOST_CHECK_EQUAL(*val, 999);
}

BOOST_AUTO_TEST_SUITE_END()
