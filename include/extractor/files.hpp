#ifndef OSRM_EXTRACTOR_FILES_HPP
#define OSRM_EXTRACTOR_FILES_HPP

#include "extractor/guidance/turn_lane_types.hpp"
#include "extractor/serialization.hpp"

#include "util/coordinate.hpp"
#include "util/packed_vector.hpp"
#include "util/serialization.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace extractor
{
namespace files
{

// reads .osrm.nodes
template<storage::Ownership Ownership>
inline void readNodes(const boost::filesystem::path &path,
        util::ViewOrVector<util::Coordinate, Ownership> &coordinates,
        util::detail::PackedVector<OSMNodeID, Ownership> &osm_node_ids)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    storage::serialization::read(reader, coordinates);
    util::serialization::read(reader, osm_node_ids);
}

// writes .osrm.nodes
template<storage::Ownership Ownership>
inline void writeNodes(const boost::filesystem::path &path,
        const util::ViewOrVector<util::Coordinate, Ownership> &coordinates,
        const util::detail::PackedVector<OSMNodeID, Ownership> &osm_node_ids)
{
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    storage::serialization::write(writer, coordinates);
    util::serialization::write(writer, osm_node_ids);
}

// reads .osrm.cnbg_to_ebg
inline void readNBGMapping(const boost::filesystem::path &path, std::vector<NBGToEBG> &mapping)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    storage::serialization::read(reader, mapping);
}

// writes .osrm.cnbg_to_ebg
inline void writeNBGMapping(const boost::filesystem::path &path,
                            const std::vector<NBGToEBG> &mapping)
{
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    storage::serialization::write(writer, mapping);
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
template <storage::Ownership Ownership>
inline void readTurnLaneDescriptions(const boost::filesystem::path &path,
                         util::ViewOrVector<std::uint32_t, Ownership> &turn_offsets,
                         util::ViewOrVector<extractor::guidance::TurnLaneType::Mask, Ownership> &turn_masks)
{
    const auto fingerprint = storage::io::FileReader::HasNoFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    storage::serialization::read(reader, turn_offsets);
    storage::serialization::read(reader, turn_masks);
}

// writes .osrm.tls
template <storage::Ownership Ownership>
inline void writeTurnLaneDescriptions(const boost::filesystem::path &path,
                         const util::ViewOrVector<std::uint32_t, Ownership> &turn_offsets,
                         const util::ViewOrVector<extractor::guidance::TurnLaneType::Mask, Ownership> &turn_masks)
{
    const auto fingerprint = storage::io::FileWriter::HasNoFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    storage::serialization::write(writer, turn_offsets);
    storage::serialization::write(writer, turn_masks);
}

}
}
}

#endif
