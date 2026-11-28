#ifndef OSRM_CUSTOMIZER_SHARED_CELL_METRICS_HPP
#define OSRM_CUSTOMIZER_SHARED_CELL_METRICS_HPP

#include "customizer/cell_metric.hpp"
#include "partitioner/cell_storage.hpp"

#include <algorithm>
#include <span>
#include <vector>

namespace osrm::customizer
{

struct SharedCellMetrics
{
    CellMetric values;
    std::vector<std::vector<std::uint64_t>> offsets;
};

template <storage::Ownership Ownership = storage::Ownership::Container>
inline SharedCellMetrics
shareCellMetrics(std::span<const partitioner::CellStorage::CellData> cells,
                 const std::vector<detail::CellMetricImpl<Ownership>> &metrics)
{
    SharedCellMetrics shared;
    shared.offsets.resize(metrics.size(), std::vector<std::uint64_t>(cells.size()));

    struct UniqueCell
    {
        std::size_t metric;
        std::size_t cell;
    };
    std::vector<UniqueCell> unique_cells;
    std::vector<std::size_t> representatives;
    representatives.reserve(metrics.size());
    std::uint64_t total_entries = 0;

    for (std::size_t cell_index = 0; cell_index < cells.size(); ++cell_index)
    {
        const auto &cell = cells[cell_index];
        const auto count =
            static_cast<std::uint64_t>(cell.num_source_nodes) * cell.num_destination_nodes;
        representatives.clear();
        for (std::size_t metric_index = 0; metric_index < metrics.size(); ++metric_index)
        {
            const auto &metric = metrics[metric_index];
            BOOST_ASSERT(metric.cell_value_offsets.empty() ||
                         metric.cell_value_offsets.size() == cells.size());
            const auto input_offset = metric.cell_value_offsets.empty()
                                          ? cell.value_offset
                                          : metric.cell_value_offsets[cell_index];
            BOOST_ASSERT(input_offset + count <= metric.weights.size());
            BOOST_ASSERT(metric.weights.size() == metric.durations.size());
            BOOST_ASSERT(metric.weights.size() == metric.distances.size());
            const auto equal = [&](std::size_t other_index)
            {
                const auto &other = metrics[other_index];
                const auto other_offset = other.cell_value_offsets.empty()
                                              ? cell.value_offset
                                              : other.cell_value_offsets[cell_index];
                const auto equal_bytes = [&](const auto &left, const auto &right)
                {
                    const auto a = std::as_bytes(std::span{left.data() + input_offset, count});
                    const auto b = std::as_bytes(std::span{right.data() + other_offset, count});
                    return std::equal(a.begin(), a.end(), b.begin());
                };
                return count == 0 || (equal_bytes(metric.weights, other.weights) &&
                                      equal_bytes(metric.durations, other.durations) &&
                                      equal_bytes(metric.distances, other.distances));
            };
            const auto match = std::find_if(representatives.begin(), representatives.end(), equal);
            if (match != representatives.end())
                shared.offsets[metric_index][cell_index] = shared.offsets[*match][cell_index];
            else
            {
                shared.offsets[metric_index][cell_index] = total_entries;
                total_entries += count;
                representatives.push_back(metric_index);
                if (count != 0)
                    unique_cells.push_back({metric_index, cell_index});
            }
        }
    }

    shared.values.weights.resize(total_entries + 1, INVALID_EDGE_WEIGHT);
    shared.values.durations.resize(total_entries + 1, MAXIMAL_EDGE_DURATION);
    shared.values.distances.resize(total_entries + 1, INVALID_EDGE_DISTANCE);
    for (const auto &unique : unique_cells)
    {
        const auto &cell = cells[unique.cell];
        const auto count =
            static_cast<std::uint64_t>(cell.num_source_nodes) * cell.num_destination_nodes;
        const auto offset = shared.offsets[unique.metric][unique.cell];
        const auto &metric = metrics[unique.metric];
        const auto input_offset = metric.cell_value_offsets.empty()
                                      ? cell.value_offset
                                      : metric.cell_value_offsets[unique.cell];
        const auto copy_bytes = [&](const auto &input, auto &output)
        {
            const auto source = std::as_bytes(std::span{input.data() + input_offset, count});
            const auto target = std::as_writable_bytes(std::span{output.data() + offset, count});
            std::copy(source.begin(), source.end(), target.begin());
        };
        copy_bytes(metric.weights, shared.values.weights);
        copy_bytes(metric.durations, shared.values.durations);
        copy_bytes(metric.distances, shared.values.distances);
    }
    return shared;
}

} // namespace osrm::customizer

#endif
