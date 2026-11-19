#ifndef OSRM_CUSTOMIZER_SERIALIZATION_HPP
#define OSRM_CUSTOMIZER_SERIALIZATION_HPP

#include "customizer/edge_based_graph.hpp"

#include "partitioner/cell_storage.hpp"

#include "storage/serialization.hpp"
#include "storage/shared_memory_ownership.hpp"
#include "storage/tar.hpp"
#include "util/block_delta_encoding.hpp"
#include "util/serialization.hpp"

namespace osrm::customizer::serialization
{

template <storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::CellMetricImpl<Ownership> &metric)
{
    storage::serialization::read(reader, name + "/weights", metric.weights);
    storage::serialization::read(reader, name + "/durations", metric.durations);
    storage::serialization::read(reader, name + "/distances", metric.distances);
}

template <storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::CellMetricImpl<Ownership> &metric)
{
    storage::serialization::write(writer, name + "/weights", metric.weights);
    storage::serialization::write(writer, name + "/durations", metric.durations);
    storage::serialization::write(writer, name + "/distances", metric.distances);
}

template <typename EdgeDataT, storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 MultiLevelGraph<EdgeDataT, Ownership> &graph)
{
    util::serialization::read(
        reader, name, static_cast<util::StaticGraph<EdgeDataT, Ownership> &>(graph));
    storage::serialization::read(reader, name + "/node_weights", graph.node_weights);
    storage::serialization::read(reader, name + "/node_durations", graph.node_durations);
    storage::serialization::read(reader, name + "/node_distances", graph.node_distances);
    storage::serialization::read(reader, name + "/is_forward_edge", graph.is_forward_edge);
    storage::serialization::read(reader, name + "/is_backward_edge", graph.is_backward_edge);
    storage::serialization::read(reader, name + "/node_to_edge_offset", graph.node_to_edge_offset);
}

template <typename EdgeDataT, storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const MultiLevelGraph<EdgeDataT, Ownership> &graph)
{
    std::vector<std::uint32_t> offsets(graph.GetNumberOfNodes() + 1);
    for (std::size_t i = 0; i < offsets.size(); ++i)
        offsets[i] = graph.BeginEdges(i);
    // CSR block names are shared with StaticGraph's query view.
    const auto blocks = util::encodeBlockDelta<std::uint32_t>(offsets);
    if (blocks && blocks->Headers.size() * sizeof(util::BlockDeltaHeader) +
                          blocks->Words.size() * sizeof(std::uint64_t) <
                      offsets.size() * sizeof(std::uint32_t))
    {
        storage::serialization::write(writer, name + "/node_blocks/headers", blocks->Headers);
        storage::serialization::write(writer, name + "/node_blocks/words", blocks->Words);
        writer.WriteElementCount64(name + "/node_blocks/count", 1);
        writer.WriteFrom(name + "/node_blocks/count", std::uint64_t(offsets.size()));
    }
    else
    {
        static_assert(sizeof(typename MultiLevelGraph<EdgeDataT, Ownership>::NodeArrayEntry) ==
                      sizeof(std::uint32_t));
        storage::serialization::write(writer, name + "/node_array", offsets);
    }
    storage::serialization::write(writer, name + "/node_weights", graph.node_weights);
    storage::serialization::write(writer, name + "/node_durations", graph.node_durations);
    storage::serialization::write(writer, name + "/node_distances", graph.node_distances);
    storage::serialization::write(writer, name + "/edge_array", graph.edge_array);
    storage::serialization::write(writer, name + "/is_forward_edge", graph.is_forward_edge);
    storage::serialization::write(writer, name + "/is_backward_edge", graph.is_backward_edge);
    storage::serialization::write(writer, name + "/node_to_edge_offset", graph.node_to_edge_offset);
}
} // namespace osrm::customizer::serialization

#endif
