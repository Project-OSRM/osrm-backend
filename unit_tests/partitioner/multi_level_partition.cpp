#include <boost/numeric/conversion/cast.hpp>
#include <boost/test/unit_test.hpp>

#include "partitioner/multi_level_partition.hpp"

#define CHECK_SIZE_RANGE(range, ref) BOOST_CHECK_EQUAL(range.second - range.first, ref)
#define CHECK_EQUAL_RANGE(range, ref)                                                              \
    do                                                                                             \
    {                                                                                              \
        const auto &lhs = range;                                                                   \
        const auto &rhs = ref;                                                                     \
        BOOST_CHECK_EQUAL_COLLECTIONS(lhs.first, lhs.second, rhs.begin(), rhs.end());              \
    } while (0)

using namespace osrm;
using namespace osrm::partitioner;

BOOST_AUTO_TEST_SUITE(multi_level_partition_tests)

BOOST_AUTO_TEST_CASE(mlp_one)
{
    // node:                0  1  2  3  4  5  6  7  8  9 10 11
    std::vector<CellID> l1{{4, 4, 2, 2, 1, 1, 3, 3, 2, 2, 5, 5}};
    MultiLevelPartition mlp{{l1}, {6}};

    BOOST_CHECK_EQUAL(mlp.GetCell(1, 0), mlp.GetCell(1, 1));
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 2), mlp.GetCell(1, 3));
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 4), mlp.GetCell(1, 5));
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 6), mlp.GetCell(1, 7));
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 8), mlp.GetCell(1, 9));
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 10), mlp.GetCell(1, 11));
}

BOOST_AUTO_TEST_CASE(mlp_shuffled)
{
    // node:                0  1  2  3  4  5  6  7  8  9 10 11
    std::vector<CellID> l1{{4, 4, 2, 2, 1, 1, 3, 3, 2, 2, 5, 5}};
    std::vector<CellID> l2{{3, 3, 3, 3, 1, 1, 1, 1, 2, 2, 0, 0}};
    std::vector<CellID> l3{{0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}};
    std::vector<CellID> l4{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    MultiLevelPartition mlp{{l1, l2, l3, l4}, {6, 4, 2, 1}};

    BOOST_CHECK_EQUAL(mlp.GetNumberOfCells(1), 6);
    BOOST_CHECK_EQUAL(mlp.GetNumberOfCells(2), 4);
    BOOST_CHECK_EQUAL(mlp.GetNumberOfCells(3), 2);
    BOOST_CHECK_EQUAL(mlp.GetNumberOfCells(4), 1);

    BOOST_CHECK_EQUAL(mlp.GetCell(1, 0), mlp.GetCell(1, 1));
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 2), mlp.GetCell(1, 3));
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 4), mlp.GetCell(1, 5));
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 6), mlp.GetCell(1, 7));
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 8), mlp.GetCell(1, 9));
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 10), mlp.GetCell(1, 11));

    BOOST_CHECK_EQUAL(mlp.GetCell(2, 0), mlp.GetCell(2, 1));
    BOOST_CHECK_EQUAL(mlp.GetCell(2, 0), mlp.GetCell(2, 2));
    BOOST_CHECK_EQUAL(mlp.GetCell(2, 0), mlp.GetCell(2, 3));
    BOOST_CHECK_EQUAL(mlp.GetCell(2, 4), mlp.GetCell(2, 5));
    BOOST_CHECK_EQUAL(mlp.GetCell(2, 4), mlp.GetCell(2, 6));
    BOOST_CHECK_EQUAL(mlp.GetCell(2, 4), mlp.GetCell(2, 7));
    BOOST_CHECK_EQUAL(mlp.GetCell(2, 8), mlp.GetCell(2, 9));
    BOOST_CHECK_EQUAL(mlp.GetCell(2, 10), mlp.GetCell(2, 11));

    BOOST_CHECK_EQUAL(mlp.GetCell(3, 0), mlp.GetCell(3, 1));
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 0), mlp.GetCell(3, 2));
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 0), mlp.GetCell(3, 3));

    BOOST_CHECK_EQUAL(mlp.GetCell(3, 4), mlp.GetCell(3, 5));
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 4), mlp.GetCell(3, 6));
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 4), mlp.GetCell(3, 7));
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 4), mlp.GetCell(3, 8));
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 4), mlp.GetCell(3, 9));
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 4), mlp.GetCell(3, 10));
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 4), mlp.GetCell(3, 11));

    BOOST_CHECK_EQUAL(mlp.GetCell(4, 0), mlp.GetCell(4, 1));
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 0), mlp.GetCell(4, 2));
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 0), mlp.GetCell(4, 3));
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 0), mlp.GetCell(4, 4));
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 0), mlp.GetCell(4, 5));
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 0), mlp.GetCell(4, 6));
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 0), mlp.GetCell(4, 7));
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 0), mlp.GetCell(4, 8));
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 0), mlp.GetCell(4, 9));
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 0), mlp.GetCell(4, 10));
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 0), mlp.GetCell(4, 11));
}

BOOST_AUTO_TEST_CASE(mlp_sorted)
{
    // node:                0  1  2  3  4  5  6  7  8  9 10 11
    std::vector<CellID> l1{{0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5}};
    std::vector<CellID> l2{{0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3}};
    std::vector<CellID> l3{{0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}};
    std::vector<CellID> l4{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    MultiLevelPartition mlp{{l1, l2, l3, l4}, {6, 4, 2, 1}};

    BOOST_CHECK_EQUAL(mlp.GetNumberOfCells(1), 6);
    BOOST_CHECK_EQUAL(mlp.GetNumberOfCells(2), 4);
    BOOST_CHECK_EQUAL(mlp.GetNumberOfCells(3), 2);
    BOOST_CHECK_EQUAL(mlp.GetNumberOfCells(4), 1);

    BOOST_CHECK_EQUAL(mlp.GetCell(1, 0), l1[0]);
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 1), l1[1]);
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 2), l1[2]);
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 3), l1[3]);
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 4), l1[4]);
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 5), l1[5]);
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 6), l1[6]);
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 7), l1[7]);
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 8), l1[8]);
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 9), l1[9]);
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 10), l1[10]);
    BOOST_CHECK_EQUAL(mlp.GetCell(1, 11), l1[11]);

    BOOST_CHECK_EQUAL(mlp.GetCell(2, 0), l2[0]);
    BOOST_CHECK_EQUAL(mlp.GetCell(2, 1), l2[1]);
    BOOST_CHECK_EQUAL(mlp.GetCell(2, 2), l2[2]);
    BOOST_CHECK_EQUAL(mlp.GetCell(2, 3), l2[3]);
    BOOST_CHECK_EQUAL(mlp.GetCell(2, 4), l2[4]);
    BOOST_CHECK_EQUAL(mlp.GetCell(2, 5), l2[5]);
    BOOST_CHECK_EQUAL(mlp.GetCell(2, 6), l2[6]);
    BOOST_CHECK_EQUAL(mlp.GetCell(2, 7), l2[7]);
    BOOST_CHECK_EQUAL(mlp.GetCell(2, 8), l2[8]);
    BOOST_CHECK_EQUAL(mlp.GetCell(2, 9), l2[9]);
    BOOST_CHECK_EQUAL(mlp.GetCell(2, 10), l2[10]);
    BOOST_CHECK_EQUAL(mlp.GetCell(2, 11), l2[11]);

    BOOST_CHECK_EQUAL(mlp.GetCell(3, 0), l3[0]);
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 1), l3[1]);
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 2), l3[2]);
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 3), l3[3]);
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 4), l3[4]);
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 5), l3[5]);
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 6), l3[6]);
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 7), l3[7]);
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 8), l3[8]);
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 9), l3[9]);
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 10), l3[10]);
    BOOST_CHECK_EQUAL(mlp.GetCell(3, 11), l3[11]);

    BOOST_CHECK_EQUAL(mlp.GetCell(4, 0), l4[0]);
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 1), l4[1]);
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 2), l4[2]);
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 3), l4[3]);
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 4), l4[4]);
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 5), l4[5]);
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 6), l4[6]);
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 7), l4[7]);
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 8), l4[8]);
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 9), l4[9]);
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 10), l4[10]);
    BOOST_CHECK_EQUAL(mlp.GetCell(4, 11), l4[11]);

    BOOST_CHECK_EQUAL(mlp.GetHighestDifferentLevel(0, 1), 0);
    BOOST_CHECK_EQUAL(mlp.GetHighestDifferentLevel(0, 2), 1);
    BOOST_CHECK_EQUAL(mlp.GetHighestDifferentLevel(0, 4), 3);
    BOOST_CHECK_EQUAL(mlp.GetHighestDifferentLevel(7, 8), 2);

    BOOST_CHECK_EQUAL(mlp.BeginChildren(2, 0), 0);
    BOOST_CHECK_EQUAL(mlp.EndChildren(2, 0), 2);
    BOOST_CHECK_EQUAL(mlp.BeginChildren(2, 1), 2);
    BOOST_CHECK_EQUAL(mlp.EndChildren(2, 1), 4);
    BOOST_CHECK_EQUAL(mlp.BeginChildren(2, 2), 4);
    BOOST_CHECK_EQUAL(mlp.EndChildren(2, 2), 5);
    BOOST_CHECK_EQUAL(mlp.BeginChildren(2, 3), 5);
    BOOST_CHECK_EQUAL(mlp.EndChildren(2, 3), 6);

    BOOST_CHECK_EQUAL(mlp.BeginChildren(3, 0), 0);
    BOOST_CHECK_EQUAL(mlp.EndChildren(3, 0), 1);
    BOOST_CHECK_EQUAL(mlp.BeginChildren(3, 1), 1);
    BOOST_CHECK_EQUAL(mlp.EndChildren(3, 1), 4);

    BOOST_CHECK_EQUAL(mlp.BeginChildren(4, 0), 0);
    BOOST_CHECK_EQUAL(mlp.EndChildren(4, 0), 2);
}

BOOST_AUTO_TEST_SUITE_END()
