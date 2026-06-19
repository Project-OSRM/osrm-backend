#include "util/packed_vector.hpp"
#include "util/typedefs.hpp"

#include "extractor/packed_osm_ids.hpp"
#include "util/exception.hpp"

#include "../common/random_seed.hpp"
#include "common/range_tools.hpp"

#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <numeric>
#include <random>
#include <ranges>

BOOST_AUTO_TEST_SUITE(packed_vector_test)

using namespace osrm;
using namespace osrm::util;

// Verify that the packed vector behaves as expected
BOOST_AUTO_TEST_CASE(insert_and_retrieve_packed_test)
{
    PackedVector<OSMNodeID, 33> packed_ids;
    std::vector<OSMNodeID> original_ids;

    const constexpr std::size_t num_test_cases = 399;
    const constexpr std::uint64_t max_id = (1ULL << 33) - 1;

    std::mt19937 rng(osrm::test::getTestRandomSeed());
    std::uniform_int_distribution<std::uint64_t> dist(0, max_id);

    for (std::size_t i = 0; i < num_test_cases; i++)
    {
        OSMNodeID r{dist(rng)}; // max 33-bit uint

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
    PackedVector<OSMNodeID, 33> packed_vec;
    const std::size_t original_size = packed_vec.capacity();
    std::vector<OSMNodeID> dummy_vec;

    BOOST_CHECK_EQUAL(original_size, dummy_vec.capacity());

    packed_vec.reserve(100);

    BOOST_CHECK(packed_vec.capacity() >= 100);
}

BOOST_AUTO_TEST_CASE(packed_vector_resize_test)
{
    PackedVector<std::uint32_t, 33> packed_vec(100);

    BOOST_CHECK_EQUAL(packed_vec.size(), 100);
    packed_vec[99] = 1337;
    packed_vec[0] = 42;
    BOOST_CHECK_EQUAL(packed_vec[99], 1337u);
    BOOST_CHECK_EQUAL(packed_vec[0], 42u);
}

BOOST_AUTO_TEST_CASE(packed_vector_iterator_test)
{
    PackedVector<std::uint32_t, 33> packed_vec(100);

    std::iota(packed_vec.begin(), packed_vec.end(), 0);

    BOOST_CHECK(std::is_sorted(packed_vec.begin(), packed_vec.end()));

    auto vec_idx = 0;
    for (auto value : packed_vec)
    {
        BOOST_CHECK_EQUAL(packed_vec[vec_idx], value);
        vec_idx++;
    }
    BOOST_CHECK_EQUAL(vec_idx, packed_vec.size());

    auto range = std::ranges::subrange(packed_vec.cbegin(), packed_vec.cend());
    BOOST_CHECK_EQUAL(range.size(), packed_vec.size());
    for (auto idx : util::irange<std::size_t>(0, packed_vec.size()))
    {
        BOOST_CHECK_EQUAL(packed_vec[idx], range[idx]);
    }

    auto reverse_range =
        std::ranges::subrange(packed_vec.cbegin(), packed_vec.cend()) | std::views::reverse;
    BOOST_CHECK_EQUAL(reverse_range.size(), packed_vec.size());
    for (auto idx : util::irange<std::size_t>(0, packed_vec.size()))
    {
        BOOST_CHECK_EQUAL(packed_vec[packed_vec.size() - 1 - idx], reverse_range[idx]);
    }

    auto mut_range = std::ranges::subrange(packed_vec.begin(), packed_vec.end());
    BOOST_CHECK_EQUAL(range.size(), packed_vec.size());
    for (auto idx : util::irange<std::size_t>(0, packed_vec.size()))
    {
        BOOST_CHECK_EQUAL(packed_vec[idx], mut_range[idx]);
    }

    auto mut_reverse_range =
        std::ranges::subrange(packed_vec.begin(), packed_vec.end()) | std::views::reverse;
    BOOST_CHECK_EQUAL(reverse_range.size(), packed_vec.size());
    for (auto idx : util::irange<std::size_t>(0, packed_vec.size()))
    {
        BOOST_CHECK_EQUAL(packed_vec[packed_vec.size() - 1 - idx], mut_reverse_range[idx]);
    }
}

BOOST_AUTO_TEST_CASE(packed_vector_10bit_small_test)
{
    PackedVector<std::uint32_t, 10> vector = {10, 5, 8, 12, 254, 4, (1 << 10) - 1, 6};
    std::vector<std::uint32_t> reference = {10, 5, 8, 12, 254, 4, (1 << 10) - 1, 6};

    BOOST_CHECK_EQUAL(vector[0], reference[0]);
    BOOST_CHECK_EQUAL(vector[1], reference[1]);
    BOOST_CHECK_EQUAL(vector[2], reference[2]);
    BOOST_CHECK_EQUAL(vector[3], reference[3]);
    BOOST_CHECK_EQUAL(vector[4], reference[4]);
    BOOST_CHECK_EQUAL(vector[5], reference[5]);
    BOOST_CHECK_EQUAL(vector[6], reference[6]);
    BOOST_CHECK_EQUAL(vector[7], reference[7]);
}

BOOST_AUTO_TEST_CASE(packed_vector_33bit_small_test)
{
    std::vector<std::uint64_t> reference = {1597322404,
                                            1939964443,
                                            2112255763,
                                            1432114613,
                                            1067854538,
                                            352118606,
                                            1782436840,
                                            1909002904,
                                            165344818};

    PackedVector<std::uint64_t, 33> vector = {1597322404,
                                              1939964443,
                                              2112255763,
                                              1432114613,
                                              1067854538,
                                              352118606,
                                              1782436840,
                                              1909002904,
                                              165344818};

    BOOST_CHECK_EQUAL(vector[0], reference[0]);
    BOOST_CHECK_EQUAL(vector[1], reference[1]);
    BOOST_CHECK_EQUAL(vector[2], reference[2]);
    BOOST_CHECK_EQUAL(vector[3], reference[3]);
    BOOST_CHECK_EQUAL(vector[4], reference[4]);
    BOOST_CHECK_EQUAL(vector[5], reference[5]);
    BOOST_CHECK_EQUAL(vector[6], reference[6]);
    BOOST_CHECK_EQUAL(vector[7], reference[7]);
}

BOOST_AUTO_TEST_CASE(values_overflow)
{
    const std::uint64_t mask = (1ull << 42) - 1;
    PackedVector<std::uint64_t, 42> vector(52, 0);

    for (auto it = vector.begin(); it != vector.end(); ++it)
    {
        BOOST_CHECK_EQUAL(*it, 0);
    }

    std::uint64_t value = 1;
    for (auto it = vector.begin(); it != vector.end(); ++it)
    {
        BOOST_CHECK_EQUAL(*it, 0);
        *it = value;
        BOOST_CHECK_EQUAL(*it, value & mask);
        value <<= 1;
    }

    for (auto it = vector.rbegin(); it != vector.rend(); ++it)
    {
        value >>= 1;
        BOOST_CHECK_EQUAL(*it, value & mask);
    }

    for (auto it = vector.cbegin(); it != vector.cend(); ++it)
    {
        BOOST_CHECK_EQUAL(*it, value & mask);
        value <<= 1;
    }
}

BOOST_AUTO_TEST_CASE(packed_vector_33bit_continious)
{
    PackedVector<std::uint64_t, 33> vector;

    for (std::uint64_t i : osrm::util::irange(0, 400))
    {
        vector.push_back(i);
        BOOST_CHECK_EQUAL(vector.back(), i);
    }
}

// Regression test for https://github.com/Project-OSRM/osrm-backend/issues/7069
// OSM node IDs above 2^34 must round-trip through PackedOSMIDs without
// truncation. The previous 34-bit packing silently masked these values.
BOOST_AUTO_TEST_CASE(packed_osm_ids_above_2pow34_roundtrip)
{
    osrm::extractor::PackedOSMIDs ids;

    const std::vector<std::uint64_t> inputs = {
        0ull,
        1ull,
        (1ull << 33),                                // 2^33
        (1ull << 34),                                // 2^34, the old truncation boundary
        (1ull << 34) + 1,                            // just above 2^34
        20'000'000'000ull,                           // realistic projected OSM node ID
        (1ull << 35),                                // 2^35
        osrm::extractor::MAX_PACKED_OSM_NODE_ID - 1, // just below the new limit
        osrm::extractor::MAX_PACKED_OSM_NODE_ID,     // exactly at the new limit
    };

    for (auto raw : inputs)
    {
        ids.push_back(OSMNodeID{raw});
    }

    for (std::size_t i = 0; i < inputs.size(); ++i)
    {
        const OSMNodeID id_val = ids[i];
        BOOST_CHECK_EQUAL(static_cast<std::uint64_t>(id_val), inputs[i]);
    }
}

// Verify that the overflow guard fires for IDs above the packed-storage limit
// instead of silently masking them on push_back.
BOOST_AUTO_TEST_CASE(packed_osm_ids_overflow_guard_throws)
{
    const OSMNodeID just_above_limit{osrm::extractor::MAX_PACKED_OSM_NODE_ID + 1};
    BOOST_CHECK_THROW(osrm::extractor::checkPackedOSMNodeIdFits(just_above_limit),
                      osrm::util::exception);

    // Sanity: values at and below the limit must not throw.
    BOOST_CHECK_NO_THROW(osrm::extractor::checkPackedOSMNodeIdFits(
        OSMNodeID{osrm::extractor::MAX_PACKED_OSM_NODE_ID}));
    BOOST_CHECK_NO_THROW(osrm::extractor::checkPackedOSMNodeIdFits(OSMNodeID{1ull << 34}));
}

BOOST_AUTO_TEST_SUITE_END()
