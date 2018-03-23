#include "storage/shared_datatype.hpp"

#include "../common/range_tools.hpp"
#include "../common/temporary_file.hpp"

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

#include <cmath>

BOOST_AUTO_TEST_SUITE(data_layout)

using namespace osrm;
using namespace osrm::storage;

BOOST_AUTO_TEST_CASE(layout_write_test)
{
    DataLayout layout;

    Block block_1{20, 8 * 20};
    Block block_2{1, 4 * 1};
    Block block_3{100, static_cast<std::uint64_t>(std::ceil(100 / 64.))};

    layout.SetBlock("block1", block_1);
    layout.SetBlock("block2", block_2);
    layout.SetBlock("block3", block_3);

    // Canary and alignment change layout size
    BOOST_CHECK_GT(layout.GetSizeOfLayout(),
                   block_1.byte_size + block_2.byte_size + block_3.byte_size);

    BOOST_CHECK_EQUAL(layout.GetBlockSize("block1"), block_1.byte_size);
    BOOST_CHECK_EQUAL(layout.GetBlockSize("block2"), block_2.byte_size);
    BOOST_CHECK_EQUAL(layout.GetBlockSize("block3"), block_3.byte_size);

    std::vector<char> buffer(layout.GetSizeOfLayout());
    auto smallest_addr = buffer.data();
    auto biggest_addr = buffer.data() + buffer.size();

    {
        auto block_1_ptr = layout.GetBlockPtr<std::uint64_t, true>(buffer.data(), "block1");
        auto block_2_ptr = layout.GetBlockPtr<std::uint32_t, true>(buffer.data(), "block2");
        auto block_3_ptr = layout.GetBlockPtr<std::uint64_t, true>(buffer.data(), "block3");

        BOOST_CHECK_LT(reinterpret_cast<std::size_t>(smallest_addr),
                       reinterpret_cast<std::size_t>(block_1_ptr));
        BOOST_CHECK_GT(
            reinterpret_cast<std::size_t>(biggest_addr),
            reinterpret_cast<std::size_t>(block_1_ptr + layout.GetBlockEntries("block1")));

        BOOST_CHECK_LT(reinterpret_cast<std::size_t>(smallest_addr),
                       reinterpret_cast<std::size_t>(block_2_ptr));
        BOOST_CHECK_GT(
            reinterpret_cast<std::size_t>(biggest_addr),
            reinterpret_cast<std::size_t>(block_2_ptr + layout.GetBlockEntries("block2")));

        BOOST_CHECK_LT(reinterpret_cast<std::size_t>(smallest_addr),
                       reinterpret_cast<std::size_t>(block_3_ptr));
        BOOST_CHECK_GT(reinterpret_cast<std::size_t>(biggest_addr),
                       reinterpret_cast<std::size_t>(
                           block_3_ptr + static_cast<std::size_t>(
                                             std::ceil(layout.GetBlockEntries("block3") / 64))));
    }
}

BOOST_AUTO_TEST_SUITE_END()
