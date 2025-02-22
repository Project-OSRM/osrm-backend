#include "util/binary_heap.hpp"

#include <boost/test/unit_test.hpp>
#include <string>

BOOST_AUTO_TEST_SUITE(binary_heap_test)

BOOST_AUTO_TEST_CASE(empty_heap)
{
    osrm::util::BinaryHeap<int> heap;
    BOOST_CHECK(heap.empty());
}

BOOST_AUTO_TEST_CASE(push_and_top)
{
    osrm::util::BinaryHeap<int> heap;
    heap.emplace(5);
    BOOST_CHECK_EQUAL(heap.top(), 5);
    BOOST_CHECK(!heap.empty());
}

BOOST_AUTO_TEST_CASE(push_multiple_and_order)
{
    osrm::util::BinaryHeap<int> heap;
    heap.emplace(5);
    heap.emplace(10);
    heap.emplace(3);
    heap.emplace(8);
    BOOST_CHECK_EQUAL(heap.top(), 10);
}

BOOST_AUTO_TEST_CASE(pop_and_order)
{
    osrm::util::BinaryHeap<int> heap;
    heap.emplace(5);
    heap.emplace(10);
    heap.emplace(3);
    heap.emplace(8);

    BOOST_CHECK_EQUAL(heap.top(), 10);
    heap.pop();
    BOOST_CHECK_EQUAL(heap.top(), 8);
    heap.pop();
    BOOST_CHECK_EQUAL(heap.top(), 5);
    heap.pop();
    BOOST_CHECK_EQUAL(heap.top(), 3);
    heap.pop();
    BOOST_CHECK(heap.empty());
}

BOOST_AUTO_TEST_CASE(clear_heap)
{
    osrm::util::BinaryHeap<int> heap;
    heap.emplace(5);
    heap.emplace(10);
    heap.clear();
    BOOST_CHECK(heap.empty());
}

BOOST_AUTO_TEST_CASE(emplace_with_custom_type)
{
    struct CustomType
    {
        int value;
        CustomType(int v) : value(v) {}
        bool operator<(const CustomType &other) const { return value < other.value; }
    };

    osrm::util::BinaryHeap<CustomType> heap;
    heap.emplace(5);
    heap.emplace(10);
    heap.emplace(3);
    BOOST_CHECK_EQUAL(heap.top().value, 10);
}

BOOST_AUTO_TEST_CASE(large_number_of_elements)
{
    osrm::util::BinaryHeap<int> heap;
    for (int i = 0; i < 1000; ++i)
    {
        heap.emplace(i);
    }
    BOOST_CHECK_EQUAL(heap.top(), 999);

    for (int i = 999; i >= 0; --i)
    {
        BOOST_CHECK_EQUAL(heap.top(), i);
        heap.pop();
    }
    BOOST_CHECK(heap.empty());
}

BOOST_AUTO_TEST_CASE(duplicate_values)
{
    osrm::util::BinaryHeap<int> heap;
    heap.emplace(5);
    heap.emplace(5);
    heap.emplace(5);

    BOOST_CHECK_EQUAL(heap.top(), 5);
    heap.pop();
    BOOST_CHECK_EQUAL(heap.top(), 5);
    heap.pop();
    BOOST_CHECK_EQUAL(heap.top(), 5);
    heap.pop();
    BOOST_CHECK(heap.empty());
}

BOOST_AUTO_TEST_CASE(string_type)
{
    osrm::util::BinaryHeap<std::string> heap;
    heap.emplace("apple");
    heap.emplace("banana");
    heap.emplace("cherry");

    BOOST_CHECK_EQUAL(heap.top(), "cherry");
    heap.pop();
    BOOST_CHECK_EQUAL(heap.top(), "banana");
    heap.pop();
    BOOST_CHECK_EQUAL(heap.top(), "apple");
}

BOOST_AUTO_TEST_CASE(emplace_after_clear)
{
    osrm::util::BinaryHeap<int> heap;
    heap.emplace(5);
    heap.clear();
    heap.emplace(10);
    BOOST_CHECK_EQUAL(heap.top(), 10);
}

BOOST_AUTO_TEST_SUITE_END()