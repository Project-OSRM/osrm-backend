#include <boost/numeric/conversion/cast.hpp>
#include <boost/test/unit_test.hpp>

#include "partitioner/bisection_to_partition.hpp"

#define CHECK_SIZE_RANGE(range, ref) BOOST_CHECK_EQUAL((range).end() - (range).begin(), ref)
#define CHECK_EQUAL_RANGE(range, ref)                                                              \
    do                                                                                             \
    {                                                                                              \
        const auto &lhs = range;                                                                   \
        const auto &rhs = ref;                                                                     \
        BOOST_CHECK_EQUAL_COLLECTIONS(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());             \
    } while (0)

using namespace osrm;
using namespace osrm::partitioner;

BOOST_AUTO_TEST_SUITE(bisection_to_partition_tests)

BOOST_AUTO_TEST_CASE(unsplitable_case)
{
    std::vector<Partition> partitions;
    std::vector<std::uint32_t> num_cells;

    /*
                    0          |          1
                   /                              \
            0      |      1                          \
           /               \                          |
       0   |   1       0   |   1                      |
       /       \       /       \             /                   \
       |       |       |       |     /                              \
     0   1   2   3   4   5   6   7   8   9   10   11   12   13  14  15
    */
    const std::vector<BisectionID> ids_1 = {
        0b000,
        0b000,
        0b001,
        0b001,
        0b010,
        0b010,
        0b011,
        0b011,
        0b100,
        0b100,
        0b100,
        0b100,
        0b100,
        0b100,
        0b100,
        0b100,
    };

    // If cell sizes are not a factor of two we will see sub-optimal results like below
    std::tie(partitions, num_cells) = bisectionToPartition(ids_1, {2, 4, 8, 16});
    BOOST_CHECK_EQUAL(partitions.size(), 4);

    std::vector<std::uint32_t> reference_num_cells = {5, 3, 2, 1};
    CHECK_EQUAL_RANGE(reference_num_cells, num_cells);

    // Four cells of size 2 and one of size 4 (could not be split)
    const std::vector<CellID> reference_l1{partitions[0][0],  // 0
                                           partitions[0][0],  // 1
                                           partitions[0][2],  // 2
                                           partitions[0][2],  // 3
                                           partitions[0][4],  // 4
                                           partitions[0][4],  // 5
                                           partitions[0][6],  // 6
                                           partitions[0][6],  // 7
                                           partitions[0][8],  // 8
                                           partitions[0][8],  // 9
                                           partitions[0][8],  // 10
                                           partitions[0][8],  // 11
                                           partitions[0][8],  // 12
                                           partitions[0][8],  // 13
                                           partitions[0][8],  // 14
                                           partitions[0][8]}; // 15
    // Two cells of size 4 and one of size 8
    const std::vector<CellID> reference_l2{partitions[1][0],  // 0
                                           partitions[1][0],  // 1
                                           partitions[1][0],  // 2
                                           partitions[1][0],  // 3
                                           partitions[1][4],  // 4
                                           partitions[1][4],  // 5
                                           partitions[1][4],  // 6
                                           partitions[1][4],  // 7
                                           partitions[1][8],  // 8
                                           partitions[1][8],  // 9
                                           partitions[1][8],  // 10
                                           partitions[1][8],  // 11
                                           partitions[1][8],  // 12
                                           partitions[1][8],  // 13
                                           partitions[1][8],  // 14
                                           partitions[1][8]}; // 15
    // Two cells of size 8
    const std::vector<CellID> reference_l3{partitions[2][0],  // 0
                                           partitions[2][0],  // 1
                                           partitions[2][0],  // 2
                                           partitions[2][0],  // 3
                                           partitions[2][0],  // 4
                                           partitions[2][0],  // 5
                                           partitions[2][0],  // 6
                                           partitions[2][0],  // 7
                                           partitions[2][8],  // 8
                                           partitions[2][8],  // 9
                                           partitions[2][8],  // 10
                                           partitions[2][8],  // 11
                                           partitions[2][8],  // 12
                                           partitions[2][8],  // 13
                                           partitions[2][8],  // 14
                                           partitions[2][8]}; // 15
    // All in one cell
    const std::vector<CellID> reference_l4{partitions[3][0],  // 0
                                           partitions[3][0],  // 1
                                           partitions[3][0],  // 2
                                           partitions[3][0],  // 3
                                           partitions[3][0],  // 4
                                           partitions[3][0],  // 5
                                           partitions[3][0],  // 6
                                           partitions[3][0],  // 7
                                           partitions[3][0],  // 8
                                           partitions[3][0],  // 9
                                           partitions[3][0],  // 10
                                           partitions[3][0],  // 11
                                           partitions[3][0],  // 12
                                           partitions[3][0],  // 13
                                           partitions[3][0],  // 14
                                           partitions[3][0]}; // 15

    CHECK_EQUAL_RANGE(reference_l1, partitions[0]);
    CHECK_EQUAL_RANGE(reference_l2, partitions[1]);
    CHECK_EQUAL_RANGE(reference_l3, partitions[2]);
    CHECK_EQUAL_RANGE(reference_l4, partitions[3]);
}

BOOST_AUTO_TEST_CASE(power_of_two_case)
{
    std::vector<Partition> partitions;
    std::vector<std::uint32_t> num_cells;

    /*
                    0          |          1
                   /                       \
            0      |      1                |
           /               \               |
       0   |   1       0   |   1        0  |   1
       /       \       /       \       /        \
       |       |       |       |       |        |
     0   1   2   3   4   5   6   7   8   9   10   11
    */
    const std::vector<BisectionID> ids_1 = {
        0b000,
        0b000,
        0b001,
        0b001,
        0b010,
        0b010,
        0b011,
        0b011,
        0b100,
        0b100,
        0b101,
        0b101,
    };

    // If cell sizes are not a factor of two we will see sub-optimal results like below
    std::tie(partitions, num_cells) = bisectionToPartition(ids_1, {2, 4, 8, 16});
    BOOST_CHECK_EQUAL(partitions.size(), 4);

    std::vector<std::uint32_t> reference_num_cells = {6, 3, 2, 1};
    CHECK_EQUAL_RANGE(reference_num_cells, num_cells);

    // Six cells of size 2
    const std::vector<CellID> reference_l1{partitions[0][0],   // 0
                                           partitions[0][0],   // 1
                                           partitions[0][2],   // 2
                                           partitions[0][2],   // 3
                                           partitions[0][4],   // 4
                                           partitions[0][4],   // 5
                                           partitions[0][6],   // 6
                                           partitions[0][6],   // 7
                                           partitions[0][8],   // 8
                                           partitions[0][8],   // 9
                                           partitions[0][10],  // 10
                                           partitions[0][10]}; // 11
    // Three cells of size 4
    const std::vector<CellID> reference_l2{partitions[1][0],  // 0
                                           partitions[1][0],  // 1
                                           partitions[1][0],  // 2
                                           partitions[1][0],  // 3
                                           partitions[1][4],  // 4
                                           partitions[1][4],  // 5
                                           partitions[1][4],  // 6
                                           partitions[1][4],  // 7
                                           partitions[1][8],  // 8
                                           partitions[1][8],  // 9
                                           partitions[1][8],  // 10
                                           partitions[1][8]}; // 11
    // Two cells of size 8 and 4
    const std::vector<CellID> reference_l3{partitions[2][0],  // 0
                                           partitions[2][0],  // 1
                                           partitions[2][0],  // 2
                                           partitions[2][0],  // 3
                                           partitions[2][0],  // 4
                                           partitions[2][0],  // 5
                                           partitions[2][0],  // 6
                                           partitions[2][0],  // 7
                                           partitions[2][8],  // 8
                                           partitions[2][8],  // 9
                                           partitions[2][8],  // 10
                                           partitions[2][8]}; // 11
    // All in one cell
    const std::vector<CellID> reference_l4{partitions[3][0],  // 0
                                           partitions[3][0],  // 1
                                           partitions[3][0],  // 2
                                           partitions[3][0],  // 3
                                           partitions[3][0],  // 4
                                           partitions[3][0],  // 5
                                           partitions[3][0],  // 6
                                           partitions[3][0],  // 7
                                           partitions[3][0],  // 8
                                           partitions[3][0],  // 9
                                           partitions[3][0],  // 10
                                           partitions[3][0]}; // 11

    CHECK_EQUAL_RANGE(reference_l1, partitions[0]);
    CHECK_EQUAL_RANGE(reference_l2, partitions[1]);
    CHECK_EQUAL_RANGE(reference_l3, partitions[2]);
    CHECK_EQUAL_RANGE(reference_l4, partitions[3]);

    // Inserting zeros at bit position 0, and 2 should not change the result
    const std::vector<BisectionID> ids_2 = {
        0b00000,
        0b00000,
        0b00010,
        0b00010,
        0b00100,
        0b00100,
        0b00110,
        0b00110,
        0b10000,
        0b10000,
        0b10010,
        0b10010,
    };
    std::tie(partitions, num_cells) = bisectionToPartition(ids_2, {2, 4, 8, 16});
    CHECK_EQUAL_RANGE(reference_l1, partitions[0]);
    CHECK_EQUAL_RANGE(reference_l2, partitions[1]);
    CHECK_EQUAL_RANGE(reference_l3, partitions[2]);
    CHECK_EQUAL_RANGE(reference_l4, partitions[3]);

    // Inserting a prefix should not change anything
    const std::vector<BisectionID> ids_3 = {
        0b101000,
        0b101000,
        0b101001,
        0b101001,
        0b101010,
        0b101010,
        0b101011,
        0b101011,
        0b101100,
        0b101100,
        0b101101,
        0b101101,
    };
    std::tie(partitions, num_cells) = bisectionToPartition(ids_3, {2, 4, 8, 16});
    CHECK_EQUAL_RANGE(reference_l1, partitions[0]);
    CHECK_EQUAL_RANGE(reference_l2, partitions[1]);
    CHECK_EQUAL_RANGE(reference_l3, partitions[2]);
    CHECK_EQUAL_RANGE(reference_l4, partitions[3]);
}

BOOST_AUTO_TEST_CASE(non_factor_two_case)
{
    std::vector<Partition> partitions;
    std::vector<std::uint32_t> num_cells;

    /*
                    0          |          1
                   /                       \
            0      |      1                |
           /               \               |
       0   |   1       0   |   1        0  |   1
       /       \       /       \       /        \
       |       |       |       |       |        |
     0   1   2   3   4   5   6   7   8   9   10   11
    */
    const std::vector<BisectionID> ids_1 = {
        0b000,
        0b000,
        0b001,
        0b001,
        0b010,
        0b010,
        0b011,
        0b011,
        0b100,
        0b100,
        0b101,
        0b101,
    };

    // If cell sizes are not a factor of two we will see sub-optimal results like below
    std::tie(partitions, num_cells) = bisectionToPartition(ids_1, {2, 4, 6, 12});
    BOOST_CHECK_EQUAL(partitions.size(), 4);

    std::vector<std::uint32_t> reference_num_cells = {6, 3, 3, 1};
    CHECK_EQUAL_RANGE(reference_num_cells, num_cells);

    // Six cells of size 2
    const std::vector<CellID> reference_l1{partitions[0][0],   // 0
                                           partitions[0][0],   // 1
                                           partitions[0][2],   // 2
                                           partitions[0][2],   // 3
                                           partitions[0][4],   // 4
                                           partitions[0][4],   // 5
                                           partitions[0][6],   // 6
                                           partitions[0][6],   // 7
                                           partitions[0][8],   // 8
                                           partitions[0][8],   // 9
                                           partitions[0][10],  // 10
                                           partitions[0][10]}; // 11
    // Three cells of size 4
    const std::vector<CellID> reference_l2{partitions[1][0],  // 0
                                           partitions[1][0],  // 1
                                           partitions[1][0],  // 2
                                           partitions[1][0],  // 3
                                           partitions[1][4],  // 4
                                           partitions[1][4],  // 5
                                           partitions[1][4],  // 6
                                           partitions[1][4],  // 7
                                           partitions[1][8],  // 8
                                           partitions[1][8],  // 9
                                           partitions[1][8],  // 10
                                           partitions[1][8]}; // 11
    // Again three cells of size 4 (bad)
    const std::vector<CellID> reference_l3{partitions[2][0],  // 0
                                           partitions[2][0],  // 1
                                           partitions[2][0],  // 2
                                           partitions[2][0],  // 3
                                           partitions[2][4],  // 4
                                           partitions[2][4],  // 5
                                           partitions[2][4],  // 6
                                           partitions[2][4],  // 7
                                           partitions[2][8],  // 8
                                           partitions[2][8],  // 9
                                           partitions[2][8],  // 10
                                           partitions[2][8]}; // 11
    // All in one cell
    const std::vector<CellID> reference_l4{partitions[3][0],  // 0
                                           partitions[3][0],  // 1
                                           partitions[3][0],  // 2
                                           partitions[3][0],  // 3
                                           partitions[3][0],  // 4
                                           partitions[3][0],  // 5
                                           partitions[3][0],  // 6
                                           partitions[3][0],  // 7
                                           partitions[3][0],  // 8
                                           partitions[3][0],  // 9
                                           partitions[3][0],  // 10
                                           partitions[3][0]}; // 11

    CHECK_EQUAL_RANGE(reference_l1, partitions[0]);
    CHECK_EQUAL_RANGE(reference_l2, partitions[1]);
    CHECK_EQUAL_RANGE(reference_l3, partitions[2]);
    CHECK_EQUAL_RANGE(reference_l4, partitions[3]);

    // Inserting zeros at bit position 0, and 2 should not change the result
    const std::vector<BisectionID> ids_2 = {
        0b00000,
        0b00000,
        0b00010,
        0b00010,
        0b00100,
        0b00100,
        0b00110,
        0b00110,
        0b10000,
        0b10000,
        0b10010,
        0b10010,
    };
    std::tie(partitions, num_cells) = bisectionToPartition(ids_2, {2, 4, 6, 12});
    CHECK_EQUAL_RANGE(reference_l1, partitions[0]);
    CHECK_EQUAL_RANGE(reference_l2, partitions[1]);
    CHECK_EQUAL_RANGE(reference_l3, partitions[2]);
    CHECK_EQUAL_RANGE(reference_l4, partitions[3]);

    // Inserting a prefix should not change anything
    const std::vector<BisectionID> ids_3 = {
        0b101000,
        0b101000,
        0b101001,
        0b101001,
        0b101010,
        0b101010,
        0b101011,
        0b101011,
        0b101100,
        0b101100,
        0b101101,
        0b101101,
    };
    std::tie(partitions, num_cells) = bisectionToPartition(ids_3, {2, 4, 6, 12});
    CHECK_EQUAL_RANGE(reference_l1, partitions[0]);
    CHECK_EQUAL_RANGE(reference_l2, partitions[1]);
    CHECK_EQUAL_RANGE(reference_l3, partitions[2]);
    CHECK_EQUAL_RANGE(reference_l4, partitions[3]);
}

BOOST_AUTO_TEST_SUITE_END()
