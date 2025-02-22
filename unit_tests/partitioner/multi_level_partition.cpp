#include <boost/numeric/conversion/cast.hpp>
#include <boost/test/unit_test.hpp>

#include "util/exception.hpp"
#include "util/for_each_indexed.hpp"
#include <util/integer_range.hpp>
#include <util/msb.hpp>

#include "partitioner/multi_level_partition.hpp"

#define CHECK_SIZE_RANGE(range, ref) BOOST_CHECK_EQUAL((range).second - (range).first, ref)
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

BOOST_AUTO_TEST_CASE(large_cell_number)
{
    size_t num_nodes = 256;
    size_t num_levels = 9;
    std::vector<std::vector<CellID>> levels(num_levels, std::vector<CellID>(num_nodes));
    std::vector<uint32_t> levels_to_num_cells(num_levels);

    std::iota(levels[0].begin(), levels[0].end(), 0);
    levels_to_num_cells[0] = num_nodes;

    for (auto l : util::irange<size_t>(1UL, num_levels))
    {
        std::transform(levels[l - 1].begin(),
                       levels[l - 1].end(),
                       levels[l].begin(),
                       [](auto val) { return val / 2; });
        levels_to_num_cells[l] = levels_to_num_cells[l - 1] / 2;
    }

    // level 1:             0  1  2  3 ... 252 253 254 255
    // level 2:             0  0  1  1 ... 126 126 127 127
    // level 3:             0  0  0  0 ... 63  63  63  63
    // ...
    // level 9:             0  0  0  0 ... 0   0   0   0
    MultiLevelPartition mlp{levels, levels_to_num_cells};

    for (const auto l : util::irange<size_t>(1UL, num_levels + 1))
    {
        BOOST_REQUIRE_EQUAL(mlp.GetNumberOfCells(l), num_nodes >> (l - 1));
        for (const auto n : util::irange<size_t>(0UL, num_nodes))
        {
            BOOST_REQUIRE_EQUAL(mlp.GetCell(l, n), levels[l - 1][n]);
        }
    }

    for (const auto m : util::irange<size_t>(0UL, num_nodes))
    {
        for (const auto n : util::irange(m + 1, num_nodes))
        {
            BOOST_REQUIRE_EQUAL(mlp.GetHighestDifferentLevel(m, n), 1 + util::msb(m ^ n));
        }
    }

    for (const auto l : util::irange<size_t>(2UL, num_levels + 1))
    {
        for (const auto c : util::irange<size_t>(0UL, levels_to_num_cells[l - 1]))
        {
            BOOST_REQUIRE_EQUAL(mlp.BeginChildren(l, c), 2 * c);
            BOOST_REQUIRE_EQUAL(mlp.EndChildren(l, c), 2 * (c + 1));
        }
    }
}

BOOST_AUTO_TEST_CASE(cell_64_bits)
{
    // bits = ceil(log2(2458529 + 1)) + ceil(log2(258451 + 1)) +  ceil(log2(16310 + 1)) +
    // ceil(log2(534 + 1))
    //      = 22 + 18 + 14 + 10
    //      = 64
    const size_t NUM_PARTITIONS = 2458529;
    const std::vector<size_t> level_cells = {NUM_PARTITIONS, 258451, 16310, 534};
    std::vector<std::vector<CellID>> levels(level_cells.size(),
                                            std::vector<CellID>(level_cells[0]));
    std::vector<uint32_t> levels_to_num_cells(level_cells.size());

    const auto set_level_cells = [&](size_t level, auto const num_cells)
    {
        for (auto val : util::irange<size_t>(0ULL, NUM_PARTITIONS))
        {
            levels[level][val] = std::min(val, num_cells - 1);
        }
        levels_to_num_cells[level] = num_cells;
    };
    util::for_each_indexed(level_cells, set_level_cells);

    MultiLevelPartition mlp{levels, levels_to_num_cells};

    BOOST_REQUIRE_EQUAL(mlp.GetNumberOfCells(1), level_cells[0]);
    BOOST_REQUIRE_EQUAL(mlp.GetNumberOfCells(2), level_cells[1]);
    BOOST_REQUIRE_EQUAL(mlp.GetNumberOfCells(3), level_cells[2]);
    BOOST_REQUIRE_EQUAL(mlp.GetNumberOfCells(4), level_cells[3]);
}

BOOST_AUTO_TEST_CASE(cell_overflow_bits)
{
    // bits = ceil(log2(4194304 + 1)) + ceil(log2(262144 + 1)) +  ceil(log2(16384 + 1)) +
    // ceil(log2(1024 + 1))
    //      = 23 + 19 + 15 + 11
    //      = 68
    const size_t NUM_PARTITIONS = 4194304;
    const std::vector<size_t> level_cells = {NUM_PARTITIONS, 262144, 16384, 1024};
    std::vector<std::vector<CellID>> levels(level_cells.size(),
                                            std::vector<CellID>(level_cells[0]));
    std::vector<uint32_t> levels_to_num_cells(level_cells.size());

    const auto set_level_cells = [&](size_t level, auto const num_cells)
    {
        for (auto val : util::irange<size_t>(0ULL, NUM_PARTITIONS))
        {
            levels[level][val] = std::min(val, num_cells - 1);
        }
        levels_to_num_cells[level] = num_cells;
    };
    util::for_each_indexed(level_cells, set_level_cells);

    BOOST_REQUIRE_EXCEPTION(MultiLevelPartition(levels, levels_to_num_cells),
                            util::exception,
                            [](auto) { return true; });
}

BOOST_AUTO_TEST_SUITE_END()
