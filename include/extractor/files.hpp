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
template <typename CoordinatesT, typename PackedOSMIDsT>
inline void readNodes(const boost::filesystem::path &path,
                      CoordinatesT &coordinates,
                      PackedOSMIDsT &osm_node_ids)
{
    static_assert(std::is_same<typename CoordinatesT::value_type, util::Coordinate>::value, "");
    static_assert(std::is_same<typename PackedOSMIDsT::value_type, OSMNodeID>::value, "");

    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    storage::serialization::read(reader, coordinates);
    util::serialization::read(reader, osm_node_ids);
}

// writes .osrm.nodes
template <typename CoordinatesT, typename PackedOSMIDsT>
inline void writeNodes(const boost::filesystem::path &path,
                       const CoordinatesT &coordinates,
                       const PackedOSMIDsT &osm_node_ids)
{
    static_assert(std::is_same<typename CoordinatesT::value_type, util::Coordinate>::value, "");
    static_assert(std::is_same<typename PackedOSMIDsT::value_type, OSMNodeID>::value, "");

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
template <typename SegmentDataT>
inline void readSegmentData(const boost::filesystem::path &path, SegmentDataT &segment_data)
{
    static_assert(std::is_same<SegmentDataContainer, SegmentDataT>::value ||
                      std::is_same<SegmentDataView, SegmentDataT>::value,
                  "");
    const auto fingerprint = storage::io::FileReader::HasNoFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    serialization::read(reader, segment_data);
}

// writes .osrm.geometry
template <typename SegmentDataT>
inline void writeSegmentData(const boost::filesystem::path &path, const SegmentDataT &segment_data)
{
    static_assert(std::is_same<SegmentDataContainer, SegmentDataT>::value ||
                      std::is_same<SegmentDataView, SegmentDataT>::value,
                  "");
    const auto fingerprint = storage::io::FileWriter::HasNoFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    serialization::write(writer, segment_data);
}

// reads .osrm.edges
template <typename TurnDataT>
inline void readTurnData(const boost::filesystem::path &path, TurnDataT &turn_data)
{
    static_assert(std::is_same<TurnDataContainer, TurnDataT>::value ||
                      std::is_same<TurnDataView, TurnDataT>::value ||
                      std::is_same<TurnDataExternalContainer, TurnDataT>::value,
                  "");
    const auto fingerprint = storage::io::FileReader::HasNoFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    serialization::read(reader, turn_data);
}

// writes .osrm.edges
template <typename TurnDataT>
inline void writeTurnData(const boost::filesystem::path &path, const TurnDataT &turn_data)
{
    static_assert(std::is_same<TurnDataContainer, TurnDataT>::value ||
                      std::is_same<TurnDataView, TurnDataT>::value ||
                      std::is_same<TurnDataExternalContainer, TurnDataT>::value,
                  "");
    const auto fingerprint = storage::io::FileWriter::HasNoFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    serialization::write(writer, turn_data);
}

// reads .osrm.tls
template <typename OffsetsT, typename MaskT>
inline void readTurnLaneDescriptions(const boost::filesystem::path &path,
                                     OffsetsT &turn_offsets,
                                     MaskT &turn_masks)
{
    static_assert(
        std::is_same<typename MaskT::value_type, extractor::guidance::TurnLaneType::Mask>::value,
        "");
    static_assert(std::is_same<typename OffsetsT::value_type, std::uint32_t>::value, "");

    const auto fingerprint = storage::io::FileReader::HasNoFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    storage::serialization::read(reader, turn_offsets);
    storage::serialization::read(reader, turn_masks);
}

// writes .osrm.tls
template <typename OffsetsT, typename MaskT>
inline void writeTurnLaneDescriptions(const boost::filesystem::path &path,
                                      const OffsetsT &turn_offsets,
                                      const MaskT &turn_masks)
{
    static_assert(
        std::is_same<typename MaskT::value_type, extractor::guidance::TurnLaneType::Mask>::value,
        "");
    static_assert(std::is_same<typename OffsetsT::value_type, std::uint32_t>::value, "");

    const auto fingerprint = storage::io::FileWriter::HasNoFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    storage::serialization::write(writer, turn_offsets);
    storage::serialization::write(writer, turn_masks);
}
}
}
}

#endif
