#include "util/block_delta_encoding.hpp"
#include "util/spatial_delta_view.hpp"

#include <boost/test/unit_test.hpp>

#include <array>
#include <limits>
#include <random>
#include <vector>

namespace
{
template <typename T> void checkRoundTrip(const std::vector<T> &values)
{
    auto encoded = osrm::util::encodeBlockDelta<T>(values);
    BOOST_REQUIRE(encoded);
    BOOST_CHECK_EQUAL(encoded->Headers.size(), (values.size() + 127) / 128);
    BOOST_REQUIRE(!encoded->Words.empty());
    BOOST_CHECK_EQUAL(encoded->Words.back(), 0);
    osrm::util::vector_view<osrm::util::BlockDeltaHeader> headers(encoded->Headers.data(),
                                                                  encoded->Headers.size());
    osrm::util::vector_view<std::uint64_t> words(encoded->Words.data(), encoded->Words.size());
    for (std::size_t i = 0; i < values.size(); ++i)
        BOOST_CHECK_EQUAL(osrm::util::decodeBlockDelta(headers, words, i), values[i]);
}
} // namespace

BOOST_AUTO_TEST_SUITE(block_delta)

BOOST_AUTO_TEST_CASE(all_widths_and_bit_offsets)
{
    std::mt19937_64 random(20261116);
    for (unsigned width = 0; width <= 64; ++width)
    {
        const auto mask = width == 64 ? std::numeric_limits<std::uint64_t>::max()
                                      : (std::uint64_t{1} << width) - 1;
        std::vector<std::uint64_t> values(257);
        for (auto &value : values)
            value = random() & mask;
        for (unsigned block = 0; block < 2; ++block)
        {
            values[block * 128] = 0;
            values[block * 128 + 1] = mask;
        }
        checkRoundTrip(values);
        const auto encoded = osrm::util::encodeBlockDelta<std::uint64_t>(values);
        BOOST_CHECK_EQUAL(encoded->Headers[0].width, width);
        BOOST_CHECK_EQUAL(encoded->Headers[1].width, width);
    }
}

BOOST_AUTO_TEST_CASE(partial_blocks_and_zero_width)
{
    for (const auto size : {0u, 1u, 2u, 63u, 64u, 127u, 128u, 129u, 255u, 256u})
    {
        checkRoundTrip(std::vector<std::uint64_t>(size, std::numeric_limits<std::uint64_t>::max()));
        checkRoundTrip(std::vector<std::uint64_t>(size, 0));
        std::vector<std::uint32_t> offsets(size);
        for (unsigned i = 0; i < size; ++i)
            offsets[i] = i * 13;
        checkRoundTrip(offsets);
    }
}

BOOST_AUTO_TEST_CASE(nonzero_base_and_nonmonotonic_values)
{
    std::vector<std::uint64_t> values;
    for (unsigned i = 0; i < 1000; ++i)
        values.push_back(std::numeric_limits<std::uint64_t>::max() - (i * 17) % 193);
    checkRoundTrip(values);
}

BOOST_AUTO_TEST_CASE(spatial_coordinate_view_handles_signed_coordinates)
{
    using osrm::util::Coordinate;
    using osrm::util::FixedLatitude;
    using osrm::util::FixedLongitude;
    using osrm::util::SpatialCoordinateView;
    using osrm::util::SpatialDeltaView;
    const std::vector<Coordinate> coordinates{
        {FixedLongitude{-179'000'000}, FixedLatitude{-80'000'000}},
        {FixedLongitude{-1}, FixedLatitude{0}},
        {FixedLongitude{179'000'000}, FixedLatitude{80'000'000}}};
    std::vector<unsigned> lon, lat;
    for (const auto &coordinate : coordinates)
    {
        lon.push_back(osrm::util::spatialUnsigned(static_cast<int>(coordinate.lon)));
        lat.push_back(osrm::util::spatialUnsigned(static_cast<int>(coordinate.lat)));
    }

    const auto encoded_lon = osrm::util::encodeBlockDelta<unsigned>(lon).value();
    const auto encoded_lat = osrm::util::encodeBlockDelta<unsigned>(lat).value();
    const SpatialDeltaView lon_view{encoded_lon.Headers, encoded_lon.Words};
    const SpatialDeltaView lat_view{encoded_lat.Headers, encoded_lat.Words};
    lon_view.validate(coordinates.size());
    lat_view.validate(coordinates.size());
    const SpatialCoordinateView view(lon_view, lat_view, coordinates.size());
    for (std::size_t i = 0; i < coordinates.size(); ++i)
        BOOST_CHECK(view[i] == coordinates[i]);
}

BOOST_AUTO_TEST_CASE(spatial_view_rejects_invalid_block_metadata)
{
    const std::vector<unsigned> values(129, 12);
    auto encoded = osrm::util::encodeBlockDelta<unsigned>(values).value();
    const osrm::util::SpatialDeltaView view{encoded.Headers, encoded.Words};
    BOOST_CHECK_NO_THROW(view.validate(values.size()));
    BOOST_CHECK_THROW(view.validate(128), osrm::util::exception);
    encoded.Headers[0].width = 33;
    BOOST_CHECK_THROW(view.validate(values.size()), osrm::util::exception);
    encoded.Headers[0].width = 0;
    encoded.Headers[1].word_offset = encoded.Words.size();
    BOOST_CHECK_THROW(view.validate(values.size()), osrm::util::exception);
}

BOOST_AUTO_TEST_SUITE_END()
