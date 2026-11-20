#include "customizer/shared_cell_metrics.hpp"

#include <boost/test/unit_test.hpp>

#include <bit>

BOOST_AUTO_TEST_SUITE(shared_cell_metrics_tests)

BOOST_AUTO_TEST_CASE(shares_only_bit_identical_payloads)
{
    std::vector<osrm::partitioner::CellStorage::CellData> cells = {
        {0, 0, 0, 2, 2}, {4, 0, 0, 1, 1}, {5, 0, 0, 0, 0}};
    osrm::customizer::CellMetric base;
    base.weights = {{1}, {2}, {3}, INVALID_EDGE_WEIGHT, {5}, INVALID_EDGE_WEIGHT};
    base.durations = {{10}, {20}, {30}, MAXIMAL_EDGE_DURATION, {50}, {0}};
    base.distances = {{0.f}, {2.f}, {3.f}, INVALID_EDGE_DISTANCE, {5.f}, {0.f}};
    std::vector<osrm::customizer::CellMetric> metrics(5, base);
    metrics[1].durations[4] = EdgeDuration{51};
    metrics[2].weights[0] = EdgeWeight{9};
    metrics[3].distances[0] = EdgeDistance{-0.f};
    metrics[4] = metrics[1];

    const auto shared = osrm::customizer::shareCellMetrics(cells, metrics);
    BOOST_CHECK_EQUAL(shared.offsets[0][0], shared.offsets[1][0]);
    BOOST_CHECK_EQUAL(shared.offsets[1][1], shared.offsets[4][1]);
    BOOST_CHECK_NE(shared.offsets[0][1], shared.offsets[1][1]);
    BOOST_CHECK_NE(shared.offsets[0][0], shared.offsets[2][0]);
    BOOST_CHECK_NE(shared.offsets[0][0], shared.offsets[3][0]);
    BOOST_CHECK_EQUAL(shared.values.weights.size(), 15);
    for (std::size_t metric = 0; metric < metrics.size(); ++metric)
    {
        for (std::size_t cell = 0; cell < cells.size(); ++cell)
        {
            const auto count = cells[cell].num_source_nodes * cells[cell].num_destination_nodes;
            const auto offset = shared.offsets[metric][cell];
            const auto original = cells[cell].value_offset;
            for (std::size_t entry = 0; entry < count; ++entry)
            {
                BOOST_CHECK_EQUAL(
                    std::bit_cast<std::uint32_t>(metrics[metric].weights[original + entry]),
                    std::bit_cast<std::uint32_t>(shared.values.weights[offset + entry]));
                BOOST_CHECK_EQUAL(
                    std::bit_cast<std::uint32_t>(metrics[metric].durations[original + entry]),
                    std::bit_cast<std::uint32_t>(shared.values.durations[offset + entry]));
                BOOST_CHECK_EQUAL(
                    std::bit_cast<std::uint32_t>(metrics[metric].distances[original + entry]),
                    std::bit_cast<std::uint32_t>(shared.values.distances[offset + entry]));
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(preserves_nan_payloads)
{
    std::vector<osrm::partitioner::CellStorage::CellData> cells = {{0, 0, 0, 1, 1}};
    osrm::customizer::CellMetric metric;
    metric.weights = {{1}};
    metric.durations = {{1}};
    metric.distances = {{std::bit_cast<float>(0x7fc00001u)}};
    std::vector<osrm::customizer::CellMetric> metrics(3, metric);
    metrics[2].distances[0] = EdgeDistance{std::bit_cast<float>(0x7fc00002u)};
    const auto shared = osrm::customizer::shareCellMetrics(cells, metrics);
    BOOST_CHECK_EQUAL(shared.offsets[0][0], shared.offsets[1][0]);
    BOOST_CHECK_NE(shared.offsets[0][0], shared.offsets[2][0]);
    BOOST_CHECK_EQUAL(std::bit_cast<std::uint32_t>(shared.values.distances[shared.offsets[0][0]]),
                      std::bit_cast<std::uint32_t>(metrics[0].distances[0]));
    BOOST_CHECK_EQUAL(std::bit_cast<std::uint32_t>(shared.values.distances[shared.offsets[2][0]]),
                      std::bit_cast<std::uint32_t>(metrics[2].distances[0]));
}

BOOST_AUTO_TEST_CASE(mapped_cells_preserve_rows_and_columns)
{
    std::vector<osrm::partitioner::CellStorageView::CellData> cells = {{0, 0, 0, 1, 1},
                                                                       {1, 1, 1, 2, 3}};
    std::vector<NodeID> sources = {9, 10, 11};
    std::vector<NodeID> destinations = {19, 20, 21, 22};
    std::vector<std::uint64_t> levels = {0, 2};
    osrm::partitioner::CellStorageView storage{{sources.data(), sources.size()},
                                               {destinations.data(), destinations.size()},
                                               {cells.data(), cells.size()},
                                               {levels.data(), levels.size()}};
    osrm::customizer::CellMetric metric;
    metric.weights = {{1}, {2}, {3}, {4}, {5}, {6}, {7}};
    metric.durations = {{10}, {20}, {30}, {40}, {50}, {60}, {70}};
    metric.distances = {{1.f}, {2.f}, {3.f}, {4.f}, {5.f}, {6.f}, {7.f}};
    auto other = metric;
    other.weights[0] = EdgeWeight{99};
    std::vector<osrm::partitioner::CellStorage::CellData> original_cells;
    for (const auto &cell : cells)
        original_cells.push_back({cell.value_offset,
                                  cell.source_boundary_offset,
                                  cell.destination_boundary_offset,
                                  cell.num_source_nodes,
                                  cell.num_destination_nodes});
    auto shared = osrm::customizer::shareCellMetrics(original_cells, {metric, other});
    for (std::size_t index = 0; index < 2; ++index)
    {
        const osrm::customizer::CellMetricView view{
            {shared.values.weights.data(), shared.values.weights.size()},
            {shared.values.durations.data(), shared.values.durations.size()},
            {shared.values.distances.data(), shared.values.distances.size()},
            {shared.offsets[index].data(), shared.offsets[index].size()}};
        const auto cell = storage.GetCell(view, 1, 1);
        const auto row = cell.GetOutWeight(11);
        BOOST_REQUIRE_EQUAL(row.size(), 3);
        BOOST_CHECK_EQUAL(row.begin()[0], EdgeWeight{5});
        BOOST_CHECK_EQUAL(row.begin()[2], EdgeWeight{7});
        const auto column = cell.GetInDuration(21);
        BOOST_CHECK_EQUAL(*column.begin(), EdgeDuration{30});
        BOOST_CHECK_EQUAL(*std::next(column.begin()), EdgeDuration{60});
        BOOST_CHECK_EQUAL(*cell.GetOutDistance(10).begin(), EdgeDistance{2.f});
    }
}

BOOST_AUTO_TEST_CASE(sharing_an_already_shared_metric_is_exact)
{
    std::vector<osrm::partitioner::CellStorage::CellData> cells = {{0, 0, 0, 1, 2},
                                                                   {2, 0, 0, 1, 1}};
    osrm::customizer::CellMetric first;
    first.weights = {{1}, {2}, {3}};
    first.durations = {{10}, {20}, {30}};
    first.distances = {{1.f}, {2.f}, {3.f}};
    auto second = first;
    second.weights[0] = EdgeWeight{99};
    const auto original = osrm::customizer::shareCellMetrics(cells, {first, second});
    std::vector<osrm::customizer::CellMetric> restored(2, original.values);
    for (std::size_t i = 0; i < restored.size(); ++i)
        restored[i].cell_value_offsets = original.offsets[i];
    const auto repeated = osrm::customizer::shareCellMetrics(cells, restored);
    BOOST_CHECK(original.offsets == repeated.offsets);
    BOOST_CHECK(original.values.weights == repeated.values.weights);
    BOOST_CHECK(original.values.durations == repeated.values.durations);
    BOOST_CHECK(original.values.distances == repeated.values.distances);
}

BOOST_AUTO_TEST_CASE(empty_input_has_a_valid_sentinel)
{
    const auto shared = osrm::customizer::shareCellMetrics({}, {});
    BOOST_CHECK(shared.offsets.empty());
    BOOST_CHECK_EQUAL(shared.values.weights.size(), 1);
}

BOOST_AUTO_TEST_SUITE_END()
