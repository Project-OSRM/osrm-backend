#ifndef OSRM_PARTITION_SERIALIZATION_HPP
#define OSRM_PARTITION_SERIALIZATION_HPP

#include "partition/cell_storage.hpp"
#include "partition/edge_based_graph.hpp"
#include "partition/multi_level_graph.hpp"
#include "partition/multi_level_partition.hpp"

#include "storage/io.hpp"
#include "storage/shared_memory_ownership.hpp"

namespace osrm
{
namespace partition
{
namespace serialization
{

template <typename EdgeDataT, storage::Ownership Ownership>
inline void read(storage::io::FileReader &reader,
                 MultiLevelGraph<EdgeDataT, Ownership> &graph)
{
    reader.DeserializeVector(graph.node_array);
    reader.DeserializeVector(graph.edge_array);
    reader.DeserializeVector(graph.edge_to_level);
}

template <typename EdgeDataT, storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer,
                  const MultiLevelGraph<EdgeDataT, Ownership> &graph)
{
    writer.SerializeVector(graph.node_array);
    writer.SerializeVector(graph.edge_array);
    writer.SerializeVector(graph.node_to_edge_offset);
}

template <> inline void read(storage::io::FileReader &reader, MultiLevelPartition &mlp)
{
    reader.ReadInto<MultiLevelPartition::LevelData>(mlp.level_data);
    reader.DeserializeVector(mlp.partition);
    reader.DeserializeVector(mlp.cell_to_children);
}

template <> inline void write(storage::io::FileWriter &writer, const MultiLevelPartition &mlp)
{
    writer.WriteOne(mlp.level_data);
    writer.SerializeVector(mlp.partition);
    writer.SerializeVector(mlp.cell_to_children);
}

template <> inline void read(storage::io::FileReader &reader, CellStorage &storage)
{
    reader.DeserializeVector(storage.weights);
    reader.DeserializeVector(storage.source_boundary);
    reader.DeserializeVector(storage.destination_boundary);
    reader.DeserializeVector(storage.cells);
    reader.DeserializeVector(storage.level_to_cell_offset);
}

template <> inline void write(storage::io::FileWriter &writer, const CellStorage &storage)
{
    writer.SerializeVector(storage.weights);
    writer.SerializeVector(storage.source_boundary);
    writer.SerializeVector(storage.destination_boundary);
    writer.SerializeVector(storage.cells);
    writer.SerializeVector(storage.level_to_cell_offset);
}
}
}
}

#endif
