#ifndef OSRM_CUSTOMIZER_SERIALIZATION_HPP
#define OSRM_CUSTOMIZER_SERIALIZATION_HPP

#include "customizer/edge_based_graph.hpp"

#include "partitioner/cell_storage.hpp"

#include "storage/serialization.hpp"
#include "storage/shared_memory_ownership.hpp"
#include "storage/tar.hpp"

namespace osrm
{
namespace customizer
{
namespace serialization
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
    storage::serialization::read(reader, name + "/node_array", graph.node_array);
    storage::serialization::read(reader, name + "/node_weights", graph.node_weights);
    storage::serialization::read(reader, name + "/node_durations", graph.node_durations);
    storage::serialization::read(reader, name + "/node_distances", graph.node_distances);
    storage::serialization::read(reader, name + "/edge_array", graph.edge_array);
    storage::serialization::read(reader, name + "/is_forward_edge", graph.is_forward_edge);
    storage::serialization::read(reader, name + "/is_backward_edge", graph.is_backward_edge);
    storage::serialization::read(reader, name + "/node_to_edge_offset", graph.node_to_edge_offset);
}

template <typename EdgeDataT, storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const MultiLevelGraph<EdgeDataT, Ownership> &graph)
{
    storage::serialization::write(writer, name + "/node_array", graph.node_array);
    storage::serialization::write(writer, name + "/node_weights", graph.node_weights);
    storage::serialization::write(writer, name + "/node_durations", graph.node_durations);
    storage::serialization::write(writer, name + "/node_distances", graph.node_distances);
    storage::serialization::write(writer, name + "/edge_array", graph.edge_array);
    storage::serialization::write(writer, name + "/is_forward_edge", graph.is_forward_edge);
    storage::serialization::write(writer, name + "/is_backward_edge", graph.is_backward_edge);
    storage::serialization::write(writer, name + "/node_to_edge_offset", graph.node_to_edge_offset);
}
}
}
}

#endif
