#include "extractor/files.hpp"
#include "storage/storage.hpp"
#include "storage/view_factory.hpp"
#include "util/mmap_file.hpp"

#include "../common/temporary_file.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace
{
using Segments = osrm::extractor::SegmentDataContainer;

template <typename Actual> void checkSegments(const Segments &expected, const Actual &actual)
{
    BOOST_REQUIRE_EQUAL(actual.GetNumberOfGeometries(), expected.GetNumberOfGeometries());
    for (std::uint32_t g = 0; g < expected.GetNumberOfGeometries(); ++g)
    {
        BOOST_CHECK(
            std::ranges::equal(actual.GetForwardGeometry(g), expected.GetForwardGeometry(g)));
        BOOST_CHECK(std::ranges::equal(actual.GetForwardWeights(g), expected.GetForwardWeights(g)));
        BOOST_CHECK(std::ranges::equal(actual.GetReverseWeights(g), expected.GetReverseWeights(g)));
        BOOST_CHECK(
            std::ranges::equal(actual.GetForwardDurations(g), expected.GetForwardDurations(g)));
        BOOST_CHECK(
            std::ranges::equal(actual.GetReverseDurations(g), expected.GetReverseDurations(g)));
        BOOST_CHECK(
            std::ranges::equal(actual.GetForwardDatasources(g), expected.GetForwardDatasources(g)));
        BOOST_CHECK(
            std::ranges::equal(actual.GetReverseDatasources(g), expected.GetReverseDatasources(g)));
    }
}

void checkArchiveRoundTrip(const Segments &input, const Segments &expected, bool shared)
{
    TemporaryFile archive;
    osrm::extractor::files::writeSegmentData(archive.path, input);
    const std::string prefix = "/common/segment_data";
    {
        const osrm::storage::tar::FileReader reader(
            archive.path, osrm::storage::tar::FileReader::VerifyFingerprint);
        BOOST_REQUIRE_EQUAL(reader.HasEntry(prefix + "/adaptive_costs/descriptors"), shared);
        BOOST_CHECK_EQUAL(reader.HasEntry(prefix + "/forward_weights/packed"), !shared);
    }

    auto layout = std::make_unique<osrm::storage::TarDataLayout>();
    osrm::storage::populateLayoutFromFile(archive.path, *layout);
    boost::iostreams::mapped_file_source mapped;
    const auto bytes = osrm::util::mmapFile<char>(archive.path, mapped);
    std::vector<osrm::storage::SharedDataIndex::AllocatedRegion> regions;
    regions.push_back({const_cast<char *>(bytes.data()), std::move(layout)});
    const osrm::storage::SharedDataIndex index{std::move(regions)};
    auto view = osrm::storage::make_segment_data_view(index, prefix);
    checkSegments(expected, std::as_const(view));
    BOOST_CHECK(
        std::ranges::equal(view.GetForwardDatasources(0), expected.GetForwardDatasources(0)));
    BOOST_CHECK(
        std::ranges::equal(view.GetReverseDatasources(0), expected.GetReverseDatasources(0)));

    TemporaryFile rewrite;
    osrm::extractor::files::writeSegmentData(rewrite.path, view);
    Segments restored;
    osrm::extractor::files::readSegmentData(rewrite.path, restored);
    checkSegments(expected, std::as_const(restored));
}
} // namespace

BOOST_AUTO_TEST_SUITE(segment_data_archive)

BOOST_AUTO_TEST_CASE(round_trip_fixed_width_shared_costs)
{
    const std::vector<std::uint32_t> indices{0, 4};
    const std::vector<NodeID> nodes{10, 20, 30, 40};
    const std::vector<DatasourceID> sources(nodes.size(), 0);
    constexpr std::uint64_t descriptor = (3ULL << 32) | (1ULL << 48) | (2ULL << 50) | (3ULL << 52);

    Segments::SegmentWeightVector weights;
    Segments::SegmentDurationVector durations;
    for (const unsigned cost : {3u, 5u, 7u, 11u, 13u, 17u, 19u, 23u, 29u, 31u, 37u, 41u})
    {
        weights.push_back(SegmentWeight{cost});
        durations.push_back(SegmentDuration{cost});
    }
    const auto count = static_cast<std::uint64_t>(weights.size());
    const Segments shared(indices,
                          nodes,
                          sources,
                          sources,
                          {descriptor, count},
                          std::move(weights),
                          std::move(durations));
    const Segments expected(indices,
                            nodes,
                            Segments::SegmentWeightVector{{0}, {3}, {5}, {7}},
                            Segments::SegmentWeightVector{{19}, {23}, {29}, {0}},
                            Segments::SegmentDurationVector{{0}, {11}, {13}, {17}},
                            Segments::SegmentDurationVector{{31}, {37}, {41}, {0}},
                            sources,
                            sources);

    checkArchiveRoundTrip(shared, expected, true);
}

BOOST_AUTO_TEST_CASE(round_trip_geometry_beyond_adaptive_length)
{
    constexpr std::uint32_t num_nodes = std::numeric_limits<std::uint16_t>::max() + 2u;
    std::vector<NodeID> nodes(num_nodes);
    std::iota(nodes.begin(), nodes.end(), NodeID{0});
    const Segments original({0, num_nodes},
                            std::move(nodes),
                            Segments::SegmentWeightVector(num_nodes, SegmentWeight{7}),
                            Segments::SegmentWeightVector(num_nodes, SegmentWeight{11}),
                            Segments::SegmentDurationVector(num_nodes, SegmentDuration{13}),
                            Segments::SegmentDurationVector(num_nodes, SegmentDuration{17}),
                            std::vector<DatasourceID>(num_nodes, 0),
                            std::vector<DatasourceID>(num_nodes, 2));

    checkArchiveRoundTrip(original, original, false);
}

BOOST_AUTO_TEST_SUITE_END()
