#include "util/compact_indexed_array.hpp"
#include "util/query_heap.hpp"

#include <boost/test/unit_test.hpp>

#include <limits>
#include <vector>

BOOST_AUTO_TEST_SUITE(compact_indexed_array)

BOOST_AUTO_TEST_CASE(unwritten_blocks_and_partial_tail_read_as_zero)
{
    osrm::util::CompactArrayStorage<unsigned, int> storage(1025);
    for (unsigned node = 0; node < 1025; ++node)
        BOOST_CHECK_EQUAL(storage.peek_index(node), 0);
    storage[511] = 17;
    storage[512] = 23;
    storage[1024] = 70000;
    for (unsigned node = 0; node < 1025; ++node)
    {
        const int expected = node == 511 ? 17 : node == 512 ? 23 : node == 1024 ? 70000 : 0;
        BOOST_CHECK_EQUAL(storage.peek_index(node), expected);
    }
    storage.Clear();
    for (unsigned node = 0; node < 1025; ++node)
        BOOST_CHECK_EQUAL(storage.peek_index(node), 0);
    storage[1024] = 11;
    BOOST_CHECK_EQUAL(storage.peek_index(1024), 11);
}

BOOST_AUTO_TEST_CASE(promotion_preserves_unwritten_blocks)
{
    osrm::util::CompactArrayStorage<unsigned, int> storage(1025);
    for (unsigned node = 0; node < 40; ++node)
        storage[node] = 70000 + node;
    for (unsigned node = 0; node < 1025; ++node)
        BOOST_CHECK_EQUAL(storage.peek_index(node), node < 40 ? 70000 + node : 0);
}

BOOST_AUTO_TEST_CASE(boundary_indices_round_trip)
{
    osrm::util::CompactArrayStorage<unsigned, int> storage(1024);
    const std::vector<int> expected = {
        0, 1, 65534, 65535, 65536, std::numeric_limits<int>::max(), -1};
    for (unsigned node = 0; node < expected.size(); ++node)
        storage[node] = expected[node];
    for (unsigned node = 0; node < expected.size(); ++node)
        BOOST_CHECK_EQUAL(storage.peek_index(node), expected[node]);
    storage[20] = storage[4];
    BOOST_CHECK_EQUAL(storage.peek_index(20), expected[4]);
}

BOOST_AUTO_TEST_CASE(full_width_fallback_preserves_all_slots)
{
    osrm::util::CompactArrayStorage<unsigned, int> storage(256);
    std::vector<int> expected(256);
    for (unsigned node = 0; node < expected.size(); ++node)
    {
        expected[node] = node * 7;
        storage[node] = expected[node];
    }
    for (unsigned node = 0; node < 64; ++node)
    {
        expected[node] = 100000 + node;
        storage[node] = expected[node];
    }
    const auto &view = storage;
    for (unsigned node = 0; node < expected.size(); ++node)
        BOOST_CHECK_EQUAL(view[node], expected[node]);
    storage.Clear();
    for (unsigned node = 0; node < expected.size(); ++node)
        BOOST_CHECK_EQUAL(view[node], 0);
    storage[200] = std::numeric_limits<int>::max();
    BOOST_CHECK_EQUAL(view[200], std::numeric_limits<int>::max());
    BOOST_CHECK_EQUAL(view[199], 0);
    BOOST_CHECK_EQUAL(view[0], 0);
}

BOOST_AUTO_TEST_CASE(sparse_overflow_is_cleared_and_reused)
{
    osrm::util::CompactArrayStorage<unsigned, int> storage(8192);
    storage[5] = 70000;
    storage.Clear();
    BOOST_CHECK_EQUAL(storage.peek_index(5), 0);
    storage[5] = 2;
    BOOST_CHECK_EQUAL(storage.peek_index(5), 2);
}

BOOST_AUTO_TEST_CASE(two_level_proxy_and_heap_reuse)
{
    using Index = osrm::util::TwoLevelStorage<unsigned,
                                              int,
                                              osrm::util::UnorderedMapStorage,
                                              osrm::util::CompactArrayStorage>;
    Index storage(1024, 128);
    const std::vector<unsigned> nodes = {0, 127, 128, 1023};
    for (const auto node : nodes)
        storage[node] = 70000 + node;
    const auto &view = storage;
    for (const auto node : nodes)
        BOOST_CHECK_EQUAL(view[node], 70000 + node);

    struct Payload
    {
        unsigned Value;
    };
    osrm::util::QueryHeap<unsigned, unsigned, int, Payload, Index> heap(1024, 128);
    heap.Insert(0, 4, {10});
    heap.Insert(129, 2, {13});
    BOOST_CHECK_EQUAL(heap.DeleteMin(), 129);
    heap.Clear();
    heap.Insert(1, 1, {17});
    BOOST_CHECK(!heap.WasInserted(0));
    BOOST_CHECK(!heap.WasInserted(129));
    BOOST_CHECK(heap.WasInserted(1));
    BOOST_CHECK_EQUAL(heap.GetData(1).Value, 17);
}

BOOST_AUTO_TEST_SUITE_END()
