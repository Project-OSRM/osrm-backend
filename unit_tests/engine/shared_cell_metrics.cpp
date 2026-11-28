#include "customizer/files.hpp"
#include "extractor/class_data.hpp"
#include "storage/storage.hpp"
#include "storage/view_factory.hpp"
#include "util/mmap_file.hpp"

#include "../common/temporary_file.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(cell_metric_views)

BOOST_AUTO_TEST_CASE(legacy_metrics_load_without_cell_offsets)
{
    TemporaryFile file;
    osrm::customizer::CellMetric first;
    first.weights = {{1}, {2}, {3}};
    first.durations = {{10}, {20}, {30}};
    first.distances = {{1.f}, {2.f}, {3.f}};
    auto second = first;
    second.weights[2] = EdgeWeight{9};
    const std::vector<osrm::customizer::CellMetric> original = {first, second};
    {
        osrm::storage::tar::FileWriter writer(file.path,
                                              osrm::storage::tar::FileWriter::GenerateFingerprint);
        writer.WriteElementCount64("/mld/metrics/test_weight/exclude", original.size());
        for (std::size_t i = 0; i < original.size(); ++i)
            osrm::customizer::serialization::write(
                writer, "/mld/metrics/test_weight/exclude/" + std::to_string(i), original[i]);
    }

    auto layout = std::make_unique<osrm::storage::TarDataLayout>();
    osrm::storage::populateLayoutFromFile(file.path, *layout);
    boost::iostreams::mapped_file_source mapped;
    const auto bytes = osrm::util::mmapFile<char>(file.path, mapped);
    std::vector<osrm::storage::SharedDataIndex::AllocatedRegion> regions;
    regions.push_back({const_cast<char *>(bytes.data()), std::move(layout)});
    const osrm::storage::SharedDataIndex index{std::move(regions)};
    for (std::size_t metric = 0; metric < original.size(); ++metric)
    {
        const auto view = osrm::storage::make_filtered_cell_metric_view(
            index, "/mld/metrics/test_weight", metric);
        BOOST_CHECK(view.cell_value_offsets.empty());
        const auto &expected = original[metric];
        BOOST_CHECK_EQUAL_COLLECTIONS(view.weights.begin(),
                                      view.weights.end(),
                                      expected.weights.begin(),
                                      expected.weights.end());
        BOOST_CHECK_EQUAL_COLLECTIONS(view.durations.begin(),
                                      view.durations.end(),
                                      expected.durations.begin(),
                                      expected.durations.end());
        BOOST_CHECK_EQUAL_COLLECTIONS(view.distances.begin(),
                                      view.distances.end(),
                                      expected.distances.begin(),
                                      expected.distances.end());
    }
}

BOOST_AUTO_TEST_CASE(prepared_metrics_share_matrices_and_keep_every_exclusion_slot)
{
    TemporaryFile file;
    const std::vector<osrm::partitioner::CellStorage::CellData> cells = {{0, 0, 0, 1, 2},
                                                                         {2, 1, 2, 1, 1}};
    std::unordered_map<std::string, std::vector<osrm::customizer::CellMetric>> original;
    auto &metrics = original["test_weight"];
    for (unsigned i = 0; i < osrm::extractor::MAX_EXCLUDABLE_CLASSES; ++i)
    {
        osrm::customizer::CellMetric metric;
        metric.weights = {{int(i + 1)}, {2}, {3}};
        metric.durations = {{int(i + 10)}, {20}, {30}};
        metric.distances = {{float(i + 1)}, {2.f}, {3.f}};
        metrics.push_back(std::move(metric));
    }
    osrm::customizer::files::writeCellMetrics(file.path, original, cells);
    std::unordered_map<std::string, std::vector<osrm::customizer::CellMetric>> restored;
    restored["test_weight"] = {};
    osrm::customizer::files::readCellMetrics(file.path, restored);
    BOOST_REQUIRE_EQUAL(restored["test_weight"].size(), metrics.size());

    auto layout = std::make_unique<osrm::storage::TarDataLayout>();
    osrm::storage::populateLayoutFromFile(file.path, *layout);
    boost::iostreams::mapped_file_source mapped;
    const auto bytes = osrm::util::mmapFile<char>(file.path, mapped);
    std::vector<osrm::storage::SharedDataIndex::AllocatedRegion> regions;
    regions.push_back({const_cast<char *>(bytes.data()), std::move(layout)});
    const osrm::storage::SharedDataIndex index{std::move(regions)};
    const auto first =
        osrm::storage::make_filtered_cell_metric_view(index, "/mld/metrics/test_weight", 0);
    BOOST_REQUIRE_EQUAL(first.cell_value_offsets.size(), cells.size());
    for (std::size_t metric = 0; metric < metrics.size(); ++metric)
    {
        const auto view = osrm::storage::make_filtered_cell_metric_view(
            index, "/mld/metrics/test_weight", metric);
        BOOST_REQUIRE_EQUAL(view.cell_value_offsets.size(), cells.size());
        BOOST_CHECK_EQUAL(view.cell_value_offsets[1], first.cell_value_offsets[1]);
        for (std::size_t cell = 0; cell < cells.size(); ++cell)
        {
            const auto offset = view.cell_value_offsets[cell];
            const auto owned_offset = restored["test_weight"][metric].cell_value_offsets[cell];
            for (std::size_t entry = 0; entry < cells[cell].num_destination_nodes; ++entry)
            {
                const auto source = cells[cell].value_offset + entry;
                BOOST_CHECK_EQUAL(view.weights[offset + entry], metrics[metric].weights[source]);
                BOOST_CHECK_EQUAL(view.durations[offset + entry],
                                  metrics[metric].durations[source]);
                BOOST_CHECK_EQUAL(view.distances[offset + entry],
                                  metrics[metric].distances[source]);
                BOOST_CHECK_EQUAL(restored["test_weight"][metric].weights[owned_offset + entry],
                                  metrics[metric].weights[source]);
                BOOST_CHECK_EQUAL(restored["test_weight"][metric].durations[owned_offset + entry],
                                  metrics[metric].durations[source]);
                BOOST_CHECK_EQUAL(restored["test_weight"][metric].distances[owned_offset + entry],
                                  metrics[metric].distances[source]);
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
