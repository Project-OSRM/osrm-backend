#include "extractor/record_spool.hpp"

#include "../common/temporary_file.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <numeric>
#include <span>
#include <vector>

using osrm::extractor::RecordSpool;

BOOST_AUTO_TEST_SUITE(record_spool)

BOOST_AUTO_TEST_CASE(keeps_small_inputs_in_memory)
{
    const TemporaryFile parent;
    RecordSpool<std::uint32_t> spool(parent.path);
    BOOST_CHECK(spool.empty());
    const std::array<std::uint32_t, 3> expected{3, 1, 2};
    for (const auto value : expected)
        spool.append(value);

    spool.finish();
    BOOST_CHECK(!spool.spilled());
    BOOST_CHECK(!std::filesystem::exists(parent.path));
    spool.forEachBatch(
        [&](std::span<const std::uint32_t> batch)
        {
            BOOST_CHECK_EQUAL_COLLECTIONS(
                batch.begin(), batch.end(), expected.begin(), expected.end());
        });

    const auto result = spool.takeVector();
    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    BOOST_CHECK(spool.empty());
}

BOOST_AUTO_TEST_CASE(replays_spilled_records_and_reuses_the_spool)
{
    const TemporaryFile parent;
    BOOST_REQUIRE(std::filesystem::create_directory(parent.path));
    RecordSpool<std::uint32_t> spool(parent.path);

    std::vector<std::uint32_t> expected(16 * 1024 * 1024 / sizeof(std::uint32_t) + 3);
    std::iota(expected.begin(), expected.end(), 0u);
    for (const auto value : expected)
        spool.append(value);

    BOOST_CHECK(spool.spilled());
    BOOST_CHECK_EQUAL(spool.size(), expected.size());
    BOOST_CHECK(!std::filesystem::is_empty(parent.path));

    spool.finish();
    std::size_t offset = 0;
    spool.forEachBatch(
        [&](std::span<const std::uint32_t> batch)
        {
            BOOST_REQUIRE_LE(offset + batch.size(), expected.size());
            const auto expected_batch = std::span(expected).subspan(offset, batch.size());
            BOOST_CHECK(std::ranges::equal(batch, expected_batch));
            offset += batch.size();
        });
    BOOST_CHECK_EQUAL(offset, expected.size());
    BOOST_CHECK(spool.takeVector() == expected);
    BOOST_CHECK(spool.empty());
    BOOST_CHECK(!spool.spilled());
    BOOST_CHECK(std::filesystem::is_empty(parent.path));

    spool.append(42u);
    const auto reused = spool.takeVector();
    BOOST_REQUIRE_EQUAL(reused.size(), 1u);
    BOOST_CHECK_EQUAL(reused.front(), 42u);
}

BOOST_AUTO_TEST_CASE(detects_truncated_spill_and_removes_it)
{
    const TemporaryFile parent;
    BOOST_REQUIRE(std::filesystem::create_directory(parent.path));
    {
        RecordSpool<std::uint32_t> spool(parent.path);
        for (std::size_t i = 0; i <= 16 * 1024 * 1024 / sizeof(std::uint32_t); ++i)
            spool.append(42u);
        spool.finish();
        BOOST_REQUIRE(spool.spilled());

        const auto directory = std::filesystem::directory_iterator(parent.path)->path();
        std::filesystem::resize_file(directory / "records", 0);
        BOOST_CHECK_THROW(spool.forEachBatch([](auto) {}), osrm::util::exception);
    }
    BOOST_CHECK(std::filesystem::is_empty(parent.path));
}

BOOST_AUTO_TEST_SUITE_END()
