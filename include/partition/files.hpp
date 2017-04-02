#ifndef OSRM_PARTITION_SERILIZATION_HPP
#define OSRM_PARTITION_SERILIZATION_HPP

#include "partition/serialization.hpp"

#include "storage/io.hpp"

namespace osrm
{
namespace partition
{
namespace files
{

// reads .osrm.mldgr file
template <typename EdgeDataT, storage::Ownership Ownership>
inline void readGraph(const boost::filesystem::path &path,
                      MultiLevelGraph<EdgeDataT, Ownership> &graph)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    serialization::read(reader, graph);
}

// writes .osrm.mldgr file
template <typename EdgeDataT, storage::Ownership Ownership>
inline void writeGraph(const boost::filesystem::path &path,
                       const MultiLevelGraph<EdgeDataT, Ownership> &graph)
{
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    serialization::write(writer, graph);
}

// read .osrm.partition file
template<storage::Ownership Ownership>
inline void readPartition(const boost::filesystem::path &path, detail::MultiLevelPartitionImpl<Ownership> &mlp)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    serialization::read(reader, mlp);
}

// writes .osrm.partition file
template<storage::Ownership Ownership>
inline void writePartition(const boost::filesystem::path &path, const detail::MultiLevelPartitionImpl<Ownership> &mlp)
{
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    serialization::write(writer, mlp);
}

// reads .osrm.cells file
template<storage::Ownership Ownership>
inline void readCells(const boost::filesystem::path &path, detail::CellStorageImpl<Ownership> &storage)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    serialization::read(reader, storage);
}

// writes .osrm.cells file
template<storage::Ownership Ownership>
inline void writeCells(const boost::filesystem::path &path, const detail::CellStorageImpl<Ownership> &storage)
{
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    serialization::write(writer, storage);
}
}
}
}

#endif
