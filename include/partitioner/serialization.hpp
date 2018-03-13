#ifndef OSRM_PARTITIONER_SERIALIZATION_HPP
#define OSRM_PARTITIONER_SERIALIZATION_HPP

#include "partitioner/cell_storage.hpp"
#include "partitioner/edge_based_graph.hpp"
#include "partitioner/multi_level_graph.hpp"
#include "partitioner/multi_level_partition.hpp"

#include "storage/block.hpp"
#include "storage/io.hpp"
#include "storage/serialization.hpp"
#include "storage/shared_memory_ownership.hpp"

namespace osrm
{
namespace partitioner
{
namespace serialization
{

template <typename EdgeDataT, storage::Ownership Ownership>
inline void read(storage::io::FileReader &reader,
                 MultiLevelGraph<EdgeDataT, Ownership> &graph,
                 std::uint32_t &connectivity_checksum)
{
    storage::serialization::read(reader, graph.node_array);
    storage::serialization::read(reader, graph.edge_array);
    storage::serialization::read(reader, graph.node_to_edge_offset);
    reader.ReadInto(connectivity_checksum);
}

template <typename EdgeDataT, storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer,
                  const MultiLevelGraph<EdgeDataT, Ownership> &graph,
                  const std::uint32_t connectivity_checksum)
{
    storage::serialization::write(writer, graph.node_array);
    storage::serialization::write(writer, graph.edge_array);
    storage::serialization::write(writer, graph.node_to_edge_offset);
    writer.WriteOne(connectivity_checksum);
}

template <storage::Ownership Ownership>
inline void read(storage::io::FileReader &reader, detail::MultiLevelPartitionImpl<Ownership> &mlp)
{
    reader.ReadInto(*mlp.level_data);
    storage::serialization::read(reader, mlp.partition);
    storage::serialization::read(reader, mlp.cell_to_children);
}

template <storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer,
                  const detail::MultiLevelPartitionImpl<Ownership> &mlp)
{
    writer.WriteOne(*mlp.level_data);
    storage::serialization::write(writer, mlp.partition);
    storage::serialization::write(writer, mlp.cell_to_children);
}

template <storage::Ownership Ownership>
inline void read(storage::io::FileReader &reader, detail::CellStorageImpl<Ownership> &storage)
{
    storage::serialization::read(reader, storage.source_boundary);
    storage::serialization::read(reader, storage.destination_boundary);
    storage::serialization::read(reader, storage.cells);
    storage::serialization::read(reader, storage.level_to_cell_offset);
}

template <storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer,
                  const detail::CellStorageImpl<Ownership> &storage)
{
    storage::serialization::write(writer, storage.source_boundary);
    storage::serialization::write(writer, storage.destination_boundary);
    storage::serialization::write(writer, storage.cells);
    storage::serialization::write(writer, storage.level_to_cell_offset);
}
}
}
}

#endif
