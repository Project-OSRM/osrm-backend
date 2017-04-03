#ifndef OSRM_PARTITION_IO_HPP
#define OSRM_PARTITION_IO_HPP

#include "partition/cell_storage.hpp"
#include "partition/edge_based_graph.hpp"
#include "partition/multi_level_graph.hpp"
#include "partition/multi_level_partition.hpp"

#include "storage/io.hpp"
#include "storage/shared_memory.hpp"

namespace osrm
{
namespace partition
{
namespace io
{

template <typename EdgeDataT, osrm::storage::Ownership Ownership>
inline void read(const boost::filesystem::path &path,
                 MultiLevelGraph<EdgeDataT, Ownership> &graph)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    reader.DeserializeVector(graph.node_array);
    reader.DeserializeVector(graph.edge_array);
    reader.DeserializeVector(graph.edge_to_level);
}

template <typename EdgeDataT, osrm::storage::Ownership Ownership>
inline void write(const boost::filesystem::path &path,
                  const MultiLevelGraph<EdgeDataT, Ownership> &graph)
{
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    writer.SerializeVector(graph.node_array);
    writer.SerializeVector(graph.edge_array);
    writer.SerializeVector(graph.node_to_edge_offset);
}

template <> inline void read(const boost::filesystem::path &path, MultiLevelPartition &mlp)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    reader.ReadInto<MultiLevelPartition::LevelData>(mlp.level_data);
    reader.DeserializeVector(mlp.partition);
    reader.DeserializeVector(mlp.cell_to_children);
}

template <> inline void write(const boost::filesystem::path &path, const MultiLevelPartition &mlp)
{
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    writer.WriteOne(mlp.level_data);
    writer.SerializeVector(mlp.partition);
    writer.SerializeVector(mlp.cell_to_children);
}

template <> inline void read(const boost::filesystem::path &path, CellStorage &storage)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    reader.DeserializeVector(storage.weights);
    reader.DeserializeVector(storage.source_boundary);
    reader.DeserializeVector(storage.destination_boundary);
    reader.DeserializeVector(storage.cells);
    reader.DeserializeVector(storage.level_to_cell_offset);
}

template <> inline void write(const boost::filesystem::path &path, const CellStorage &storage)
{
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

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
