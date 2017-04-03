#include "util/range_table.hpp"
#include "util/typedefs.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <numeric>
#include <stxxl/vector>

BOOST_AUTO_TEST_SUITE(range_table)

using namespace osrm;
using namespace osrm::util;

constexpr unsigned BLOCK_SIZE = 16;
typedef RangeTable<BLOCK_SIZE, osrm::storage::Ownership::Container> TestRangeTable;

void ConstructionTest(stxxl::vector<unsigned> lengths, std::vector<unsigned> offsets)
{
    BOOST_ASSERT(lengths.size() == offsets.size() - 1);

    TestRangeTable table(lengths);

    for (unsigned i = 0; i < lengths.size(); i++)
    {
        auto range = table.GetRange(i);
        BOOST_CHECK_EQUAL(range.front(), offsets[i]);
        BOOST_CHECK_EQUAL(range.back() + 1, offsets[i + 1]);
    }
}

void ComputeLengthsOffsets(stxxl::vector<unsigned> &lengths,
                           std::vector<unsigned> &offsets,
                           unsigned num)
{
    lengths.resize(num);
    offsets.resize(num + 1);
    std::iota(lengths.begin(), lengths.end(), 1);
    offsets[0] = 0;
    std::partial_sum(lengths.begin(), lengths.end(), offsets.begin() + 1);

    std::stringstream l_ss;
    l_ss << "Lengths: ";
    for (auto l : lengths)
        l_ss << l << ", ";
    BOOST_TEST_MESSAGE(l_ss.str());
    std::stringstream o_ss;
    o_ss << "Offsets: ";
    for (auto o : offsets)
        o_ss << o << ", ";
    BOOST_TEST_MESSAGE(o_ss.str());
}

BOOST_AUTO_TEST_CASE(serialization_test)
{
    stxxl::vector<unsigned> lengths;
    std::vector<unsigned> offsets;
    ComputeLengthsOffsets(lengths, offsets, (BLOCK_SIZE + 1) * 10);

    TestRangeTable in_table(lengths);
    TestRangeTable out_table;

    std::stringstream ss;
    ss << in_table;
    ss >> out_table;

    for (unsigned i = 0; i < lengths.size(); i++)
    {
        auto range = out_table.GetRange(i);
        BOOST_CHECK_EQUAL(range.front(), offsets[i]);
        BOOST_CHECK_EQUAL(range.back() + 1, offsets[i + 1]);
    }
}

BOOST_AUTO_TEST_CASE(construction_test)
{
    // only offset empty block
    stxxl::vector<unsigned> empty_lengths;
    empty_lengths.push_back(1);
    ConstructionTest(empty_lengths, {0, 1});
    // first block almost full => sentinel is last element of block
    // [0] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, (16)}
    stxxl::vector<unsigned> almost_full_lengths;
    std::vector<unsigned> almost_full_offsets;
    ComputeLengthsOffsets(almost_full_lengths, almost_full_offsets, BLOCK_SIZE);
    ConstructionTest(almost_full_lengths, almost_full_offsets);

    // first block full => sentinel is offset of new block, next block empty
    // [0]     {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}
    // [(153)] {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    stxxl::vector<unsigned> full_lengths;
    std::vector<unsigned> full_offsets;
    ComputeLengthsOffsets(full_lengths, full_offsets, BLOCK_SIZE + 1);
    ConstructionTest(full_lengths, full_offsets);

    // first block full and offset of next block not sentinel, but the first differential value
    // [0]   {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}
    // [153] {(17), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    stxxl::vector<unsigned> over_full_lengths;
    std::vector<unsigned> over_full_offsets;
    ComputeLengthsOffsets(over_full_lengths, over_full_offsets, BLOCK_SIZE + 2);
    ConstructionTest(over_full_lengths, over_full_offsets);

    // test multiple blocks
    stxxl::vector<unsigned> multiple_lengths;
    std::vector<unsigned> multiple_offsets;
    ComputeLengthsOffsets(multiple_lengths, multiple_offsets, (BLOCK_SIZE + 1) * 10);
    ConstructionTest(multiple_lengths, multiple_offsets);
}

BOOST_AUTO_TEST_SUITE_END()
