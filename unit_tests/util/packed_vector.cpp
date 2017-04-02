#include "util/packed_vector.hpp"
#include "util/typedefs.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(packed_vector_test)

using namespace osrm;
using namespace osrm::util;

// Verify that the packed vector behaves as expected
BOOST_AUTO_TEST_CASE(insert_and_retrieve_packed_test)
{
    PackedVector<OSMNodeID> packed_ids;
    std::vector<OSMNodeID> original_ids;

    const constexpr std::size_t num_test_cases = 399;

    for (std::size_t i = 0; i < num_test_cases; i++)
    {
        OSMNodeID r{static_cast<std::uint64_t>(rand() % 2147483647)}; // max 33-bit uint

        packed_ids.push_back(r);
        original_ids.push_back(r);
    }

    for (std::size_t i = 0; i < num_test_cases; i++)
    {
        BOOST_CHECK_EQUAL(original_ids.at(i), packed_ids.at(i));
    }
}

BOOST_AUTO_TEST_CASE(packed_vector_capacity_test)
{
    PackedVector<OSMNodeID> packed_vec;
    const std::size_t original_size = packed_vec.capacity();
    std::vector<OSMNodeID> dummy_vec;

    BOOST_CHECK_EQUAL(original_size, dummy_vec.capacity());

    packed_vec.reserve(100);

    BOOST_CHECK(packed_vec.capacity() >= 100);
}

BOOST_AUTO_TEST_SUITE_END()
