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
    std::unique_ptr<BaseDataLayout> layout = std::make_unique<ContiguousDataLayout>();

    Block block_1{20, 8 * 20};
    Block block_2{1, 4 * 1};
    Block block_3{100, static_cast<std::uint64_t>(std::ceil(100 / 64.))};

    layout->SetBlock("block1", block_1);
    layout->SetBlock("block2", block_2);
    layout->SetBlock("block3", block_3);

    // Canary and alignment change layout size
    BOOST_CHECK_GT(layout->GetSizeOfLayout(),
                   block_1.byte_size + block_2.byte_size + block_3.byte_size);

    BOOST_CHECK_EQUAL(layout->GetBlockSize("block1"), block_1.byte_size);
    BOOST_CHECK_EQUAL(layout->GetBlockSize("block2"), block_2.byte_size);
    BOOST_CHECK_EQUAL(layout->GetBlockSize("block3"), block_3.byte_size);

    std::vector<char> buffer(layout->GetSizeOfLayout());
    auto smallest_addr = buffer.data();
    auto biggest_addr = buffer.data() + buffer.size();

    {
        auto block_1_ptr =
            reinterpret_cast<std::uint64_t *>(layout->GetBlockPtr(buffer.data(), "block1"));
        auto block_2_ptr =
            reinterpret_cast<std::uint32_t *>(layout->GetBlockPtr(buffer.data(), "block2"));
        auto block_3_ptr =
            reinterpret_cast<std::uint64_t *>(layout->GetBlockPtr(buffer.data(), "block3"));

        BOOST_CHECK_LE(reinterpret_cast<std::size_t>(smallest_addr),
                       reinterpret_cast<std::size_t>(block_1_ptr));
        BOOST_CHECK_GT(
            reinterpret_cast<std::size_t>(biggest_addr),
            reinterpret_cast<std::size_t>(block_1_ptr + layout->GetBlockEntries("block1")));

        BOOST_CHECK_LT(reinterpret_cast<std::size_t>(smallest_addr),
                       reinterpret_cast<std::size_t>(block_2_ptr));
        BOOST_CHECK_GT(
            reinterpret_cast<std::size_t>(biggest_addr),
            reinterpret_cast<std::size_t>(block_2_ptr + layout->GetBlockEntries("block2")));

        BOOST_CHECK_LT(reinterpret_cast<std::size_t>(smallest_addr),
                       reinterpret_cast<std::size_t>(block_3_ptr));
        BOOST_CHECK_GT(reinterpret_cast<std::size_t>(biggest_addr),
                       reinterpret_cast<std::size_t>(
                           block_3_ptr + static_cast<std::size_t>(
                                             std::ceil(layout->GetBlockEntries("block3") / 64))));
    }
}

BOOST_AUTO_TEST_CASE(layout_list_test)
{
    std::unique_ptr<BaseDataLayout> layout = std::make_unique<ContiguousDataLayout>();

    Block block_1{20, 8 * 20};
    Block block_2{1, 4 * 1};
    Block block_3{100, static_cast<std::uint64_t>(std::ceil(100 / 64.))};

    layout->SetBlock("/ch/edge_filter/block1", block_1);
    layout->SetBlock("/ch/edge_filter/block2", block_2);
    layout->SetBlock("/ch/edge_filter/block3", block_3);
    layout->SetBlock("/mld/metrics/0/durations", block_2);
    layout->SetBlock("/mld/metrics/0/weights", block_3);
    layout->SetBlock("/mld/metrics/1/durations", block_2);
    layout->SetBlock("/mld/metrics/1/weights", block_3);

    std::vector<std::string> results_1;
    std::vector<std::string> results_2;
    std::vector<std::string> results_3;
    layout->List("/ch/edge_filter", std::back_inserter(results_1));
    layout->List("/ch/edge_filter/", std::back_inserter(results_2));
    layout->List("/ch/", std::back_inserter(results_3));

    std::vector<std::string> results_4;
    std::vector<std::string> results_5;
    std::vector<std::string> results_6;
    layout->List("/mld/metrics", std::back_inserter(results_4));
    layout->List("/mld/metrics/", std::back_inserter(results_5));
    layout->List("/mld/", std::back_inserter(results_6));

    std::vector<std::string> results_7;
    layout->List("", std::back_inserter(results_7));
    BOOST_CHECK_EQUAL(results_7.size(), 7);

    CHECK_EQUAL_RANGE(
        results_1, "/ch/edge_filter/block1", "/ch/edge_filter/block2", "/ch/edge_filter/block3");
    CHECK_EQUAL_RANGE(
        results_2, "/ch/edge_filter/block1", "/ch/edge_filter/block2", "/ch/edge_filter/block3");
    CHECK_EQUAL_RANGE(results_3, "/ch/edge_filter");
    CHECK_EQUAL_RANGE(results_4,
                      "/mld/metrics/0/durations",
                      "/mld/metrics/0/weights",
                      "/mld/metrics/1/durations",
                      "/mld/metrics/1/weights");
    CHECK_EQUAL_RANGE(results_5, "/mld/metrics/0", "/mld/metrics/1");
    CHECK_EQUAL_RANGE(results_6, "/mld/metrics");
}

BOOST_AUTO_TEST_SUITE_END()
