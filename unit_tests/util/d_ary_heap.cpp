
#include "util/d_ary_heap.hpp"
#include <boost/test/unit_test.hpp>

using namespace osrm::util;

BOOST_AUTO_TEST_SUITE(d_ary_heap_test)

BOOST_AUTO_TEST_CASE(test_empty_heap)
{
    DAryHeap<int, 2> heap;
    BOOST_CHECK(heap.empty());
    BOOST_CHECK_EQUAL(heap.size(), 0);
    heap.emplace(10, [](int &, size_t) {});
    BOOST_CHECK(!heap.empty());
    BOOST_CHECK_EQUAL(heap.size(), 1);
}

BOOST_AUTO_TEST_CASE(test_emplace_and_top)
{
    DAryHeap<int, 2> heap;
    heap.emplace(10, [](int &, size_t) {});
    heap.emplace(5, [](int &, size_t) {});
    heap.emplace(8, [](int &, size_t) {});

    BOOST_CHECK_EQUAL(heap.top(), 5);
    BOOST_CHECK_EQUAL(heap.size(), 3);
}

BOOST_AUTO_TEST_CASE(test_pop)
{
    DAryHeap<int, 2> heap;
    heap.emplace(10, [](int &, size_t) {});
    heap.emplace(5, [](int &, size_t) {});
    heap.emplace(8, [](int &, size_t) {});

    heap.pop([](int &, size_t) {});
    BOOST_CHECK_EQUAL(heap.top(), 8);
    BOOST_CHECK_EQUAL(heap.size(), 2);

    heap.pop([](int &, size_t) {});
    BOOST_CHECK_EQUAL(heap.top(), 10);
    BOOST_CHECK_EQUAL(heap.size(), 1);
}

BOOST_AUTO_TEST_CASE(test_decrease)
{
    struct HeapData
    {
        int key;
        int data;

        bool operator<(const HeapData &other) const { return key < other.key; }
    };
    DAryHeap<HeapData, 2> heap;
    size_t handle = DAryHeap<HeapData, 2>::INVALID_HANDLE;

    auto reorder_handler = [&](const HeapData &value, size_t new_handle)
    {
        if (value.data == 42)
        {
            handle = new_handle;
        }
    };

    heap.emplace({10, 42}, reorder_handler);
    heap.emplace({5, 73}, reorder_handler);
    heap.emplace({8, 37}, reorder_handler);

    heap.decrease(handle, {3, 42}, reorder_handler);
    BOOST_CHECK_EQUAL(heap.size(), 3);
    BOOST_CHECK_EQUAL(heap.top().key, 3);
    BOOST_CHECK_EQUAL(heap.top().data, 42);
    heap.pop(reorder_handler);
    BOOST_CHECK_EQUAL(heap.size(), 2);
    BOOST_CHECK_EQUAL(heap.top().key, 5);
    BOOST_CHECK_EQUAL(heap.top().data, 73);
    heap.pop(reorder_handler);
    BOOST_CHECK_EQUAL(heap.size(), 1);
    BOOST_CHECK_EQUAL(heap.top().key, 8);
    BOOST_CHECK_EQUAL(heap.top().data, 37);
    heap.pop(reorder_handler);
    BOOST_CHECK_EQUAL(heap.size(), 0);
    BOOST_CHECK(heap.empty());
}

BOOST_AUTO_TEST_CASE(test_reorder_handler)
{
    std::vector<int> reordered_values;
    std::vector<size_t> reordered_indices;
    auto reorder_handler = [&](int value, size_t index)
    {
        reordered_values.push_back(value);
        reordered_indices.push_back(index);
    };
    DAryHeap<int, 2> heap;
    std::vector<int> expected_reordered_values;
    std::vector<int> expected_reordered_indices;

    heap.emplace(10, reorder_handler);

    expected_reordered_values = {10};
    expected_reordered_indices = {0};
    BOOST_CHECK_EQUAL_COLLECTIONS(reordered_values.begin(),
                                  reordered_values.end(),
                                  expected_reordered_values.begin(),
                                  expected_reordered_values.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(reordered_indices.begin(),
                                  reordered_indices.end(),
                                  expected_reordered_indices.begin(),
                                  expected_reordered_indices.end());

    heap.emplace(5, reorder_handler);
    expected_reordered_values = {10, 10, 5};
    expected_reordered_indices = {0, 1, 0};
    BOOST_CHECK_EQUAL_COLLECTIONS(reordered_values.begin(),
                                  reordered_values.end(),
                                  expected_reordered_values.begin(),
                                  expected_reordered_values.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(reordered_indices.begin(),
                                  reordered_indices.end(),
                                  expected_reordered_indices.begin(),
                                  expected_reordered_indices.end());

    heap.emplace(8, reorder_handler);
    expected_reordered_values = {10, 10, 5, 8};
    expected_reordered_indices = {0, 1, 0, 2};
    BOOST_CHECK_EQUAL_COLLECTIONS(reordered_values.begin(),
                                  reordered_values.end(),
                                  expected_reordered_values.begin(),
                                  expected_reordered_values.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(reordered_indices.begin(),
                                  reordered_indices.end(),
                                  expected_reordered_indices.begin(),
                                  expected_reordered_indices.end());

    heap.pop(reorder_handler);
    expected_reordered_values = {10, 10, 5, 8, 8};
    expected_reordered_indices = {0, 1, 0, 2, 0};
    BOOST_CHECK_EQUAL_COLLECTIONS(reordered_values.begin(),
                                  reordered_values.end(),
                                  expected_reordered_values.begin(),
                                  expected_reordered_values.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(reordered_indices.begin(),
                                  reordered_indices.end(),
                                  expected_reordered_indices.begin(),
                                  expected_reordered_indices.end());

    heap.pop(reorder_handler);

    expected_reordered_values = {10, 10, 5, 8, 8, 10};
    expected_reordered_indices = {0, 1, 0, 2, 0, 0};
    BOOST_CHECK_EQUAL_COLLECTIONS(reordered_values.begin(),
                                  reordered_values.end(),
                                  expected_reordered_values.begin(),
                                  expected_reordered_values.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(reordered_indices.begin(),
                                  reordered_indices.end(),
                                  expected_reordered_indices.begin(),
                                  expected_reordered_indices.end());

    heap.pop(reorder_handler);
    expected_reordered_values = {10, 10, 5, 8, 8, 10};
    expected_reordered_indices = {0, 1, 0, 2, 0, 0};
    BOOST_CHECK_EQUAL_COLLECTIONS(reordered_values.begin(),
                                  reordered_values.end(),
                                  expected_reordered_values.begin(),
                                  expected_reordered_values.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(reordered_indices.begin(),
                                  reordered_indices.end(),
                                  expected_reordered_indices.begin(),
                                  expected_reordered_indices.end());
}

BOOST_AUTO_TEST_SUITE_END()
