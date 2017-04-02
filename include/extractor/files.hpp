#ifndef OSRM_EXTRACTOR_FILES_HPP
#define OSRM_EXTRACTOR_FILES_HPP

#include "extractor/guidance/turn_lane_types.hpp"
#include "extractor/seralization.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace extractor
{
namespace files
{

// reads .osrm.cnbg_to_ebg
inline void readNBGMapping(const boost::filesystem::path &path, std::vector<NBGToEBG> &mapping)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    serialization::read(reader, mapping);
}

// writes .osrm.cnbg_to_ebg
inline void writeNBGMapping(const boost::filesystem::path &path,
                            const std::vector<NBGToEBG> &mapping)
{
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    serialization::write(writer, mapping);
}

// reads .osrm.datasource_names
inline void readDatasources(const boost::filesystem::path &path, Datasources &sources)
{
    const auto fingerprint = storage::io::FileReader::HasNoFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    serialization::read(reader, sources);
}

// writes .osrm.datasource_names
inline void writeDatasources(const boost::filesystem::path &path, Datasources &sources)
{
    const auto fingerprint = storage::io::FileWriter::HasNoFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    serialization::write(writer, sources);
}

// reads .osrm.geometry
template <storage::Ownership Ownership>
inline void readSegmentData(const boost::filesystem::path &path,
                            detail::SegmentDataContainerImpl<Ownership> &segment_data)
{
    const auto fingerprint = storage::io::FileReader::HasNoFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    serialization::read(reader, segment_data);
}

// writes .osrm.geometry
template <storage::Ownership Ownership>
inline void writeSegmentData(const boost::filesystem::path &path,
                             const detail::SegmentDataContainerImpl<Ownership> &segment_data)
{
    const auto fingerprint = storage::io::FileWriter::HasNoFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    serialization::write(writer, segment_data);
}

// reads .osrm.edges
template <storage::Ownership Ownership>
inline void readTurnData(const boost::filesystem::path &path,
                         detail::TurnDataContainerImpl<Ownership> &turn_data)
{
    const auto fingerprint = storage::io::FileReader::HasNoFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    serialization::read(reader, turn_data);
}

// writes .osrm.edges
template <storage::Ownership Ownership>
inline void writeTurnData(const boost::filesystem::path &path,
                          const detail::TurnDataContainerImpl<Ownership> &turn_data)
{
    const auto fingerprint = storage::io::FileWriter::HasNoFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    serialization::write(writer, turn_data);
}

// reads .osrm.tls
template <bool UseShareMemory>
inline void readTurnLaneDescriptions(const boost::filesystem::path &path,
                         typename util::ShM<std::uint32_t, UseShareMemory>::vector &turn_offsets,
                         typename util::ShM<extractor::guidance::TurnLaneType::Mask, UseShareMemory>::vector &turn_masks)
{
    const auto fingerprint = storage::io::FileReader::HasNoFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    reader.DeserializeVector(turn_offsets);
    reader.DeserializeVector(turn_masks);
}

// writes .osrm.tls
template <bool UseShareMemory>
inline void writeTurnLaneDescriptions(const boost::filesystem::path &path,
                         const typename util::ShM<std::uint32_t, UseShareMemory>::vector &turn_offsets,
                         const typename util::ShM<extractor::guidance::TurnLaneType::Mask, UseShareMemory>::vector &turn_masks)
{
    const auto fingerprint = storage::io::FileWriter::HasNoFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    writer.SerializeVector(turn_offsets);
    writer.SerializeVector(turn_masks);
}

}
}
}

#endif
