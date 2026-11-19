#ifndef OSRM_PARTITIONER_SERIALIZATION_HPP
#define OSRM_PARTITIONER_SERIALIZATION_HPP

#include "partitioner/cell_storage.hpp"
#include "partitioner/edge_based_graph.hpp"
#include "partitioner/multi_level_graph.hpp"
#include "partitioner/multi_level_partition.hpp"

#include "storage/block.hpp"
#include "storage/block_vector_encoding.hpp"
#include "storage/io.hpp"
#include "storage/serialization.hpp"
#include "storage/shared_memory_ownership.hpp"
#include "storage/tar.hpp"

namespace osrm::partitioner::serialization
{

template <storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::MultiLevelPartitionImpl<Ownership> &mlp)
{
    const bool has_partition_blocks = reader.HasEntry(name + "/partition_blocks/headers");
    if constexpr (Ownership == storage::Ownership::View)
    {
        if (!mlp.level_data || (has_partition_blocks && !mlp.partition_count))
            throw util::exception("Partition view storage is not bound for the archive layout");
    }

    reader.ReadInto(name + "/level_data", *mlp.level_data);
    if (has_partition_blocks)
    {
        if constexpr (Ownership != storage::Ownership::View)
            storage::readCompactVector(reader, name + "/partition", mlp.partition);
        else
        {
            storage::serialization::read(
                reader, name + "/partition_blocks/headers", mlp.partition_headers);
            storage::serialization::read(
                reader, name + "/partition_blocks/words", mlp.partition_words);
            reader.ReadInto(name + "/partition_blocks/count", *mlp.partition_count);
        }
    }
    else
        storage::serialization::read(reader, name + "/partition", mlp.partition);
    storage::serialization::read(reader, name + "/cell_to_children", mlp.cell_to_children);
}

template <storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::MultiLevelPartitionImpl<Ownership> &mlp)
{
    writer.WriteElementCount64(name + "/level_data", 1);
    writer.WriteFrom(name + "/level_data", *mlp.level_data);
    if (mlp.partition_count)
    {
        storage::serialization::write(
            writer, name + "/partition_blocks/headers", mlp.partition_headers);
        storage::serialization::write(
            writer, name + "/partition_blocks/words", mlp.partition_words);
        writer.WriteElementCount64(name + "/partition_blocks/count", 1);
        writer.WriteFrom(name + "/partition_blocks/count", *mlp.partition_count);
    }
    else
        storage::writeCompactVector<PartitionID>(
            writer, name + "/partition", {mlp.partition.data(), mlp.partition.size()});
    storage::serialization::write(writer, name + "/cell_to_children", mlp.cell_to_children);
}

template <storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::CellStorageImpl<Ownership> &storage)
{
    storage::serialization::read(reader, name + "/source_boundary", storage.source_boundary);
    storage::serialization::read(
        reader, name + "/destination_boundary", storage.destination_boundary);
    storage::serialization::read(reader, name + "/cells", storage.cells);
    storage::serialization::read(
        reader, name + "/level_to_cell_offset", storage.level_to_cell_offset);
}

template <storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::CellStorageImpl<Ownership> &storage)
{
    storage::serialization::write(writer, name + "/source_boundary", storage.source_boundary);
    storage::serialization::write(
        writer, name + "/destination_boundary", storage.destination_boundary);
    storage::serialization::write(writer, name + "/cells", storage.cells);
    storage::serialization::write(
        writer, name + "/level_to_cell_offset", storage.level_to_cell_offset);
}
} // namespace osrm::partitioner::serialization

#endif
