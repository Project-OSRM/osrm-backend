#ifndef OSRM_PARTITION_IO_HPP
#define OSRM_PARTITION_IO_HPP

#include "storage/io.hpp"
#include "util/multi_level_partition.hpp"
#include "util/cell_storage.hpp"

namespace osrm
{
namespace partition
{
namespace io
{

template <>
inline void write(const boost::filesystem::path &path, const util::MultiLevelPartition &mlp)
{
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    writer.WriteOne(mlp.level_data);
    writer.SerializeVector(mlp.partition);
    writer.SerializeVector(mlp.cell_to_children);
}

template <>
inline void write(const boost::filesystem::path &path, const util::CellStorage &storage)
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
