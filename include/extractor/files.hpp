#ifndef OSRM_EXTRACTOR_FILES_HPP
#define OSRM_EXTRACTOR_FILES_HPP

#include "extractor/edge_based_edge.hpp"
#include "extractor/node_data_container.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/query_node.hpp"
#include "extractor/serialization.hpp"
#include "extractor/turn_lane_types.hpp"

#include "util/coordinate.hpp"
#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"
#include "util/guidance/turn_lanes.hpp"
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

    storage::tar::FileWriter writer(path, storage::tar::FileWriter::GenerateFingerprint);

    serialization::write(writer, "/common/intersection_bearings", intersection_bearings);
    storage::serialization::write(writer, "/common/entry_classes", entry_classes);
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

    storage::tar::FileReader reader(path, storage::tar::FileReader::VerifyFingerprint);

    serialization::read(reader, "/common/intersection_bearings", intersection_bearings);
    storage::serialization::read(reader, "/common/entry_classes", entry_classes);
}

// reads .osrm.properties
inline void readProfileProperties(const boost::filesystem::path &path,
                                  ProfileProperties &properties)
{
    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    serialization::read(reader, "/common/properties", properties);
}

// writes .osrm.properties
inline void writeProfileProperties(const boost::filesystem::path &path,
                                   const ProfileProperties &properties)
{
    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    serialization::write(writer, "/common/properties", properties);
}

template <typename EdgeBasedEdgeVector>
void writeEdgeBasedGraph(const boost::filesystem::path &path,
                         EdgeID const number_of_edge_based_nodes,
                         const EdgeBasedEdgeVector &edge_based_edge_list,
                         const std::uint32_t connectivity_checksum)
{
    static_assert(std::is_same<typename EdgeBasedEdgeVector::value_type, EdgeBasedEdge>::value, "");

    storage::tar::FileWriter writer(path, storage::tar::FileWriter::GenerateFingerprint);

    writer.WriteElementCount64("/common/number_of_edge_based_nodes", 1);
    writer.WriteFrom("/common/number_of_edge_based_nodes", number_of_edge_based_nodes);
    storage::serialization::write(writer, "/common/edge_based_edge_list", edge_based_edge_list);
    writer.WriteElementCount64("/common/connectivity_checksum", 1);
    writer.WriteFrom("/common/connectivity_checksum", connectivity_checksum);
}

// reads .osrm.ebg file
template <typename EdgeBasedEdgeVector>
void readEdgeBasedGraph(const boost::filesystem::path &path,
                        EdgeID &number_of_edge_based_nodes,
                        EdgeBasedEdgeVector &edge_based_edge_list,
                        std::uint32_t &connectivity_checksum)
{
    static_assert(std::is_same<typename EdgeBasedEdgeVector::value_type, EdgeBasedEdge>::value, "");

    storage::tar::FileReader reader(path, storage::tar::FileReader::VerifyFingerprint);

    reader.ReadInto("/common/number_of_edge_based_nodes", number_of_edge_based_nodes);
    storage::serialization::read(reader, "/common/edge_based_edge_list", edge_based_edge_list);
    reader.ReadInto("/common/connectivity_checksum", connectivity_checksum);
}

// reads .osrm.nbg_nodes
template <typename CoordinatesT, typename PackedOSMIDsT>
inline void readNodes(const boost::filesystem::path &path,
                      CoordinatesT &coordinates,
                      PackedOSMIDsT &osm_node_ids)
{
    static_assert(std::is_same<typename CoordinatesT::value_type, util::Coordinate>::value, "");
    static_assert(std::is_same<typename PackedOSMIDsT::value_type, OSMNodeID>::value, "");

    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    storage::serialization::read(reader, "/common/nbn_data/coordinates", coordinates);
    util::serialization::read(reader, "/common/nbn_data/osm_node_ids", osm_node_ids);
}

// reads only coordinates from .osrm.nbg_nodes
template <typename CoordinatesT>
inline void readNodeCoordinates(const boost::filesystem::path &path, CoordinatesT &coordinates)
{
    static_assert(std::is_same<typename CoordinatesT::value_type, util::Coordinate>::value, "");

    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    storage::serialization::read(reader, "/common/nbn_data/coordinates", coordinates);
}

// writes .osrm.nbg_nodes
template <typename CoordinatesT, typename PackedOSMIDsT>
inline void writeNodes(const boost::filesystem::path &path,
                       const CoordinatesT &coordinates,
                       const PackedOSMIDsT &osm_node_ids)
{
    static_assert(std::is_same<typename CoordinatesT::value_type, util::Coordinate>::value, "");
    static_assert(std::is_same<typename PackedOSMIDsT::value_type, OSMNodeID>::value, "");

    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    storage::serialization::write(writer, "/common/nbn_data/coordinates", coordinates);
    util::serialization::write(writer, "/common/nbn_data/osm_node_ids", osm_node_ids);
}

// reads .osrm.cnbg_to_ebg
inline void readNBGMapping(const boost::filesystem::path &path, std::vector<NBGToEBG> &mapping)
{
    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    storage::serialization::read(reader, "/common/cnbg_to_ebg", mapping);
}

// writes .osrm.cnbg_to_ebg
inline void writeNBGMapping(const boost::filesystem::path &path,
                            const std::vector<NBGToEBG> &mapping)
{
    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    storage::serialization::write(writer, "/common/cnbg_to_ebg", mapping);
}

// reads .osrm.datasource_names
inline void readDatasources(const boost::filesystem::path &path, Datasources &sources)
{
    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    serialization::read(reader, "/common/data_sources_names", sources);
}

// writes .osrm.datasource_names
inline void writeDatasources(const boost::filesystem::path &path, Datasources &sources)
{
    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    serialization::write(writer, "/common/data_sources_names", sources);
}

// reads .osrm.geometry
template <typename SegmentDataT>
inline void readSegmentData(const boost::filesystem::path &path, SegmentDataT &segment_data)
{
    static_assert(std::is_same<SegmentDataContainer, SegmentDataT>::value ||
                      std::is_same<SegmentDataView, SegmentDataT>::value,
                  "");
    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    serialization::read(reader, "/common/segment_data", segment_data);
}

// writes .osrm.geometry
template <typename SegmentDataT>
inline void writeSegmentData(const boost::filesystem::path &path, const SegmentDataT &segment_data)
{
    static_assert(std::is_same<SegmentDataContainer, SegmentDataT>::value ||
                      std::is_same<SegmentDataView, SegmentDataT>::value,
                  "");
    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    serialization::write(writer, "/common/segment_data", segment_data);
}

// reads .osrm.ebg_nodes
template <typename NodeDataT>
inline void readNodeData(const boost::filesystem::path &path, NodeDataT &node_data)
{
    static_assert(std::is_same<EdgeBasedNodeDataContainer, NodeDataT>::value ||
                      std::is_same<EdgeBasedNodeDataView, NodeDataT>::value ||
                      std::is_same<EdgeBasedNodeDataExternalContainer, NodeDataT>::value,
                  "");
    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    serialization::read(reader, "/common/ebg_node_data", node_data);
}

// writes .osrm.ebg_nodes
template <typename NodeDataT>
inline void writeNodeData(const boost::filesystem::path &path, const NodeDataT &node_data)
{
    static_assert(std::is_same<EdgeBasedNodeDataContainer, NodeDataT>::value ||
                      std::is_same<EdgeBasedNodeDataView, NodeDataT>::value ||
                      std::is_same<EdgeBasedNodeDataExternalContainer, NodeDataT>::value,
                  "");
    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    serialization::write(writer, "/common/ebg_node_data", node_data);
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

    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    storage::serialization::read(reader, "/common/turn_lanes/offsets", turn_offsets);
    storage::serialization::read(reader, "/common/turn_lanes/masks", turn_masks);
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

    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    storage::serialization::write(writer, "/common/turn_lanes/offsets", turn_offsets);
    storage::serialization::write(writer, "/common/turn_lanes/masks", turn_masks);
}

// reads .osrm.tld
template <typename TurnLaneDataT>
inline void readTurnLaneData(const boost::filesystem::path &path, TurnLaneDataT &turn_lane_data)
{
    static_assert(
        std::is_same<typename TurnLaneDataT::value_type, util::guidance::LaneTupleIdPair>::value,
        "");

    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    storage::serialization::read(reader, "/common/turn_lanes/data", turn_lane_data);
}

// writes .osrm.tld
template <typename TurnLaneDataT>
inline void writeTurnLaneData(const boost::filesystem::path &path,
                              const TurnLaneDataT &turn_lane_data)
{
    static_assert(
        std::is_same<typename TurnLaneDataT::value_type, util::guidance::LaneTupleIdPair>::value,
        "");

    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    storage::serialization::write(writer, "/common/turn_lanes/data", turn_lane_data);
}

// reads .osrm.timestamp
template <typename TimestampDataT>
inline void readTimestamp(const boost::filesystem::path &path, TimestampDataT &timestamp)
{
    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    storage::serialization::read(reader, "/common/timestamp", timestamp);
}

// writes .osrm.timestamp
template <typename TimestampDataT>
inline void writeTimestamp(const boost::filesystem::path &path, const TimestampDataT &timestamp)
{
    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    storage::serialization::write(writer, "/common/timestamp", timestamp);
}

// reads .osrm.maneuver_overrides
template <typename StorageManeuverOverrideT, typename NodeSequencesT>
inline void readManeuverOverrides(const boost::filesystem::path &path,
                                  StorageManeuverOverrideT &maneuver_overrides,
                                  NodeSequencesT &node_sequences)
{
    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    storage::serialization::read(
        reader, "/common/maneuver_overrides/overrides", maneuver_overrides);
    storage::serialization::read(
        reader, "/common/maneuver_overrides/node_sequences", node_sequences);
}

// writes .osrm.maneuver_overrides
inline void writeManeuverOverrides(const boost::filesystem::path &path,
                                   const std::vector<StorageManeuverOverride> &maneuver_overrides,
                                   const std::vector<NodeID> &node_sequences)
{
    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    storage::serialization::write(
        writer, "/common/maneuver_overrides/overrides", maneuver_overrides);
    storage::serialization::write(
        writer, "/common/maneuver_overrides/node_sequences", node_sequences);
}

// writes .osrm.turn_weight_penalties
template <typename TurnPenaltyT>
inline void writeTurnWeightPenalty(const boost::filesystem::path &path,
                                   const TurnPenaltyT &turn_penalty)
{
    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    storage::serialization::write(writer, "/common/turn_penalty/weight", turn_penalty);
}

// read .osrm.turn_weight_penalties
template <typename TurnPenaltyT>
inline void readTurnWeightPenalty(const boost::filesystem::path &path, TurnPenaltyT &turn_penalty)
{
    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    storage::serialization::read(reader, "/common/turn_penalty/weight", turn_penalty);
}

// writes .osrm.turn_duration_penalties
template <typename TurnPenaltyT>
inline void writeTurnDurationPenalty(const boost::filesystem::path &path,
                                     const TurnPenaltyT &turn_penalty)
{
    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    storage::serialization::write(writer, "/common/turn_penalty/duration", turn_penalty);
}

// read .osrm.turn_weight_penalties
template <typename TurnPenaltyT>
inline void readTurnDurationPenalty(const boost::filesystem::path &path, TurnPenaltyT &turn_penalty)
{
    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    storage::serialization::read(reader, "/common/turn_penalty/duration", turn_penalty);
}

// writes .osrm.restrictions
template <typename ConditionalRestrictionsT>
inline void writeConditionalRestrictions(const boost::filesystem::path &path,
                                         const ConditionalRestrictionsT &conditional_restrictions)
{
    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    serialization::write(writer, "/common/conditional_restrictions", conditional_restrictions);
}

// read .osrm.restrictions
template <typename ConditionalRestrictionsT>
inline void readConditionalRestrictions(const boost::filesystem::path &path,
                                        ConditionalRestrictionsT &conditional_restrictions)
{
    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    serialization::read(reader, "/common/conditional_restrictions", conditional_restrictions);
}

// reads .osrm file which is a temporary file of osrm-extract
template <typename BarrierOutIter, typename TrafficSignalsOutIter, typename PackedOSMIDsT>
void readRawNBGraph(const boost::filesystem::path &path,
                    BarrierOutIter barriers,
                    TrafficSignalsOutIter traffic_signals,
                    std::vector<util::Coordinate> &coordinates,
                    PackedOSMIDsT &osm_node_ids,
                    std::vector<extractor::NodeBasedEdge> &edge_list,
                    std::vector<extractor::NodeBasedEdgeAnnotation> &annotations)
{
    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    auto number_of_nodes = reader.ReadElementCount64("/extractor/nodes");
    coordinates.resize(number_of_nodes);
    osm_node_ids.reserve(number_of_nodes);
    auto index = 0;
    auto decode = [&](const auto &current_node) {
        coordinates[index].lon = current_node.lon;
        coordinates[index].lat = current_node.lat;
        osm_node_ids.push_back(current_node.node_id);
        index++;
    };
    reader.ReadStreaming<extractor::QueryNode>("/extractor/nodes",
                                               boost::make_function_output_iterator(decode));

    reader.ReadStreaming<NodeID>("/extractor/barriers", barriers);

    reader.ReadStreaming<NodeID>("/extractor/traffic_lights", traffic_signals);

    storage::serialization::read(reader, "/extractor/edges", edge_list);
    storage::serialization::read(reader, "/extractor/annotations", annotations);
}

template <typename NameTableT>
void readNames(const boost::filesystem::path &path, NameTableT &table)
{
    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    serialization::read(reader, "/common/names", table);
}

template <typename NameTableT>
void writeNames(const boost::filesystem::path &path, const NameTableT &table)
{
    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    serialization::write(writer, "/common/names", table);
}

template <typename NodeWeightsVectorT>
void readEdgeBasedNodeWeights(const boost::filesystem::path &path, NodeWeightsVectorT &weights)
{
    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    storage::serialization::read(reader, "/extractor/edge_based_node_weights", weights);
}

template <typename NodeDistancesVectorT>
void readEdgeBasedNodeDistances(const boost::filesystem::path &path,
                                NodeDistancesVectorT &distances)
{
    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    storage::serialization::read(reader, "/extractor/edge_based_node_distances", distances);
}

template <typename NodeWeightsVectorT, typename NodeDurationsVectorT, typename NodeDistancesVectorT>
void writeEdgeBasedNodeWeightsDurationsDistances(const boost::filesystem::path &path,
                                                 const NodeWeightsVectorT &weights,
                                                 const NodeDurationsVectorT &durations,
                                                 const NodeDistancesVectorT &distances)
{
    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    storage::serialization::write(writer, "/extractor/edge_based_node_weights", weights);
    storage::serialization::write(writer, "/extractor/edge_based_node_durations", durations);
    storage::serialization::write(writer, "/extractor/edge_based_node_distances", distances);
}

template <typename NodeWeightsVectorT, typename NodeDurationsVectorT>
void readEdgeBasedNodeWeightsDurations(const boost::filesystem::path &path,
                                       NodeWeightsVectorT &weights,
                                       NodeDurationsVectorT &durations)
{
    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    storage::serialization::read(reader, "/extractor/edge_based_node_weights", weights);
    storage::serialization::read(reader, "/extractor/edge_based_node_durations", durations);
}

template <typename NodeWeightsVectorT, typename NodeDurationsVectorT>
void writeEdgeBasedNodeWeightsDurations(const boost::filesystem::path &path,
                                        const NodeWeightsVectorT &weights,
                                        const NodeDurationsVectorT &durations)
{
    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    storage::serialization::write(writer, "/extractor/edge_based_node_weights", weights);
    storage::serialization::write(writer, "/extractor/edge_based_node_durations", durations);
}

template <typename RTreeT>
void writeRamIndex(const boost::filesystem::path &path, const RTreeT &rtree)
{
    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    util::serialization::write(writer, "/common/rtree", rtree);
}

template <typename RTreeT> void readRamIndex(const boost::filesystem::path &path, RTreeT &rtree)
{
    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    util::serialization::read(reader, "/common/rtree", rtree);
}

template <typename EdgeListT>
void writeCompressedNodeBasedGraph(const boost::filesystem::path &path, const EdgeListT &edge_list)
{
    const auto fingerprint = storage::tar::FileWriter::GenerateFingerprint;
    storage::tar::FileWriter writer{path, fingerprint};

    storage::serialization::write(writer, "/extractor/cnbg", edge_list);
}

template <typename EdgeListT>
void readCompressedNodeBasedGraph(const boost::filesystem::path &path, EdgeListT &edge_list)
{
    const auto fingerprint = storage::tar::FileReader::VerifyFingerprint;
    storage::tar::FileReader reader{path, fingerprint};

    storage::serialization::read(reader, "/extractor/cnbg", edge_list);
}
}
}
}

#endif
