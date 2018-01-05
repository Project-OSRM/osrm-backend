#ifndef OSRM_EXTRACTOR_FILES_HPP
#define OSRM_EXTRACTOR_FILES_HPP

#include "extractor/edge_based_edge.hpp"
#include "extractor/node_data_container.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/serialization.hpp"
#include "extractor/turn_lane_types.hpp"

#include "util/coordinate.hpp"
#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"
#include "util/packed_vector.hpp"
#include "util/range_table.hpp"
#include "util/serialization.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace extractor
{
namespace files
{

// writes the .osrm.icd file
template <typename IntersectionBearingsT, typename EntryClassVectorT>
inline void writeIntersections(const boost::filesystem::path &path,
                               const IntersectionBearingsT &intersection_bearings,
                               const EntryClassVectorT &entry_classes)
{
    static_assert(std::is_same<IntersectionBearingsContainer, IntersectionBearingsT>::value ||
                      std::is_same<IntersectionBearingsView, IntersectionBearingsT>::value,
                  "");

    storage::io::FileWriter writer(path, storage::io::FileWriter::GenerateFingerprint);

    serialization::write(writer, intersection_bearings);
    storage::serialization::write(writer, entry_classes);
}

// read the .osrm.icd file
template <typename IntersectionBearingsT, typename EntryClassVectorT>
inline void readIntersections(const boost::filesystem::path &path,
                              IntersectionBearingsT &intersection_bearings,
                              EntryClassVectorT &entry_classes)
{
    static_assert(std::is_same<IntersectionBearingsContainer, IntersectionBearingsT>::value ||
                      std::is_same<IntersectionBearingsView, IntersectionBearingsT>::value,
                  "");

    storage::io::FileReader reader(path, storage::io::FileReader::VerifyFingerprint);

    serialization::read(reader, intersection_bearings);
    storage::serialization::read(reader, entry_classes);
}

// reads .osrm.properties
inline void readProfileProperties(const boost::filesystem::path &path,
                                  ProfileProperties &properties)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    serialization::read(reader, properties);
}

// writes .osrm.properties
inline void writeProfileProperties(const boost::filesystem::path &path,
                                   const ProfileProperties &properties)
{
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    serialization::write(writer, properties);
}

template <typename EdgeBasedEdgeVector>
void writeEdgeBasedGraph(const boost::filesystem::path &path,
                         EdgeID const number_of_edge_based_nodes,
                         const EdgeBasedEdgeVector &edge_based_edge_list)
{
    static_assert(std::is_same<typename EdgeBasedEdgeVector::value_type, EdgeBasedEdge>::value, "");

    storage::io::FileWriter writer(path, storage::io::FileWriter::GenerateFingerprint);

    writer.WriteElementCount64(number_of_edge_based_nodes);
    storage::serialization::write(writer, edge_based_edge_list);
}

template <typename EdgeBasedEdgeVector>
void readEdgeBasedGraph(const boost::filesystem::path &path,
                        EdgeID &number_of_edge_based_nodes,
                        EdgeBasedEdgeVector &edge_based_edge_list)
{
    static_assert(std::is_same<typename EdgeBasedEdgeVector::value_type, EdgeBasedEdge>::value, "");

    storage::io::FileReader reader(path, storage::io::FileReader::VerifyFingerprint);

    number_of_edge_based_nodes = reader.ReadElementCount64();
    storage::serialization::read(reader, edge_based_edge_list);
}

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
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    serialization::read(reader, sources);
}

// writes .osrm.datasource_names
inline void writeDatasources(const boost::filesystem::path &path, Datasources &sources)
{
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
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
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
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
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    serialization::write(writer, segment_data);
}

// reads .osrm.ebg_nodes
template <typename NodeDataT>
inline void readNodeData(const boost::filesystem::path &path, NodeDataT &node_data)
{
    static_assert(std::is_same<EdgeBasedNodeDataContainer, NodeDataT>::value ||
                      std::is_same<EdgeBasedNodeDataView, NodeDataT>::value ||
                      std::is_same<EdgeBasedNodeDataExternalContainer, NodeDataT>::value,
                  "");
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader{path, fingerprint};

    serialization::read(reader, node_data);
}

// writes .osrm.ebg_nodes
template <typename NodeDataT>
inline void writeNodeData(const boost::filesystem::path &path, const NodeDataT &node_data)
{
    static_assert(std::is_same<EdgeBasedNodeDataContainer, NodeDataT>::value ||
                      std::is_same<EdgeBasedNodeDataView, NodeDataT>::value ||
                      std::is_same<EdgeBasedNodeDataExternalContainer, NodeDataT>::value,
                  "");
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    serialization::write(writer, node_data);
}

// reads .osrm.tls
template <typename OffsetsT, typename MaskT>
inline void readTurnLaneDescriptions(const boost::filesystem::path &path,
                                     OffsetsT &turn_offsets,
                                     MaskT &turn_masks)
{
    static_assert(std::is_same<typename MaskT::value_type, extractor::TurnLaneType::Mask>::value,
                  "");
    static_assert(std::is_same<typename OffsetsT::value_type, std::uint32_t>::value, "");

    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
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
    static_assert(std::is_same<typename MaskT::value_type, extractor::TurnLaneType::Mask>::value,
                  "");
    static_assert(std::is_same<typename OffsetsT::value_type, std::uint32_t>::value, "");

    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;
    storage::io::FileWriter writer{path, fingerprint};

    storage::serialization::write(writer, turn_offsets);
    storage::serialization::write(writer, turn_masks);
}
}
}
}

#endif
