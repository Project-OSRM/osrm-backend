#ifndef OSRM_STORAGE_IO_HPP_
#define OSRM_STORAGE_IO_HPP_

#include "util/fingerprint.hpp"
#include "util/simple_logger.hpp"

#include <boost/filesystem/fstream.hpp>

#include <tuple>

namespace osrm
{
namespace storage
{
namespace io
{

inline std::size_t readPropertiesSize() { return 1; }

template <typename PropertiesT>
inline void readProperties(boost::filesystem::ifstream &properties_stream,
                           PropertiesT *properties,
                           std::size_t properties_size)
{
    properties_stream.read(reinterpret_cast<char *>(properties), properties_size);
}

#pragma pack(push, 1)
struct HSGRHeader
{
    std::uint32_t checksum;
    std::uint32_t number_of_nodes;
    std::uint32_t number_of_edges;
};
#pragma pack(pop)
static_assert(sizeof(HSGRHeader) == 12, "HSGRHeader is not packed");

// Returns the checksum and the number of nodes and number of edges
inline HSGRHeader readHSGRHeader(boost::filesystem::ifstream &input_stream)
{
    const util::FingerPrint fingerprint_valid = util::FingerPrint::GetValid();
    util::FingerPrint fingerprint_loaded;
    input_stream.read(reinterpret_cast<char *>(&fingerprint_loaded), sizeof(util::FingerPrint));
    if (!fingerprint_loaded.TestGraphUtil(fingerprint_valid))
    {
        util::SimpleLogger().Write(logWARNING) << ".hsgr was prepared with different build.\n"
                                                  "Reprocess to get rid of this warning.";
    }

    HSGRHeader header;
    input_stream.read(reinterpret_cast<char *>(&header.checksum), sizeof(header.checksum));
    input_stream.read(reinterpret_cast<char *>(&header.number_of_nodes),
                      sizeof(header.number_of_nodes));
    input_stream.read(reinterpret_cast<char *>(&header.number_of_edges),
                      sizeof(header.number_of_edges));

    BOOST_ASSERT_MSG(0 != header.number_of_nodes, "number of nodes is zero");
    // number of edges can be zero, this is the case in a few test fixtures

    return header;
}

// Needs to be called after readHSGRHeader() to get the correct offset in the stream
template <typename NodeT, typename EdgeT>
void readHSGR(boost::filesystem::ifstream &input_stream,
              NodeT node_buffer[],
              std::uint32_t number_of_nodes,
              EdgeT edge_buffer[],
              std::uint32_t number_of_edges)
{
    input_stream.read(reinterpret_cast<char *>(node_buffer), number_of_nodes * sizeof(NodeT));
    input_stream.read(reinterpret_cast<char *>(edge_buffer), number_of_edges * sizeof(EdgeT));
}
// Returns the size of the timestamp in a file
inline std::uint32_t readTimestampSize(boost::filesystem::ifstream &timestamp_input_stream)
{
    timestamp_input_stream.seekg(0, timestamp_input_stream.end);
    auto length = timestamp_input_stream.tellg();
    timestamp_input_stream.seekg(0, timestamp_input_stream.beg);
    return length;
}

// Reads the timestamp in a file
inline void readTimestamp(boost::filesystem::ifstream &timestamp_input_stream,
                          char timestamp[],
                          std::size_t timestamp_length)
{
    timestamp_input_stream.read(timestamp, timestamp_length * sizeof(char));
}

// Returns the number of edges in a .edges file
inline std::uint32_t readEdgesSize(boost::filesystem::ifstream &edges_input_stream)
{
    std::uint32_t number_of_edges;
    edges_input_stream.read((char *)&number_of_edges, sizeof(std::uint32_t));
    return number_of_edges;
}

// Reads edge data from .edge files which includes its
// geometry, name ID, turn instruction, lane data ID, travel mode, entry class ID
template <typename GeometryIDT,
          typename NameIDT,
          typename TurnInstructionT,
          typename LaneDataIDT,
          typename TravelModeT,
          typename EntryClassIDT,
          typename PreTurnBearingT,
          typename PostTurnBearingT>
void readEdgesData(boost::filesystem::ifstream &edges_input_stream,
                   GeometryIDT geometry_list[],
                   NameIDT name_id_list[],
                   TurnInstructionT turn_instruction_list[],
                   LaneDataIDT lane_data_id_list[],
                   TravelModeT travel_mode_list[],
                   EntryClassIDT entry_class_id_list[],
                   PreTurnBearingT pre_turn_bearing_list[],
                   PostTurnBearingT post_turn_bearing_list[],
                   std::uint32_t number_of_edges)
{
    extractor::OriginalEdgeData current_edge_data;
    for (std::uint32_t i = 0; i < number_of_edges; ++i)
    {
        edges_input_stream.read((char *)&(current_edge_data), sizeof(extractor::OriginalEdgeData));
        geometry_list[i] = current_edge_data.via_geometry;
        name_id_list[i] = current_edge_data.name_id;
        turn_instruction_list[i] = current_edge_data.turn_instruction;
        lane_data_id_list[i] = current_edge_data.lane_data_id;
        travel_mode_list[i] = current_edge_data.travel_mode;
        entry_class_id_list[i] = current_edge_data.entry_classid;
        pre_turn_bearing_list[i] = current_edge_data.pre_turn_bearing;
        post_turn_bearing_list[i] = current_edge_data.post_turn_bearing;
    }
}

// Returns the number of nodes in a .nodes file
inline std::uint32_t readNodesSize(boost::filesystem::ifstream &nodes_input_stream)
{
    std::uint32_t number_of_coordinates;
    nodes_input_stream.read((char *)&number_of_coordinates, sizeof(std::uint32_t));
    return number_of_coordinates;
}

// Reads coordinates and OSM node IDs from .nodes files
template <typename CoordinateT, typename OSMNodeIDVectorT>
void readNodesData(boost::filesystem::ifstream &nodes_input_stream,
                   CoordinateT coordinate_list[],
                   OSMNodeIDVectorT &osmnodeid_list,
                   std::uint32_t number_of_coordinates)
{
    extractor::QueryNode current_node;
    for (std::uint32_t i = 0; i < number_of_coordinates; ++i)
    {
        nodes_input_stream.read((char *)&current_node, sizeof(extractor::QueryNode));
        coordinate_list[i] = CoordinateT(current_node.lon, current_node.lat);
        osmnodeid_list.push_back(current_node.node_id);
        BOOST_ASSERT(coordinate_list[i].IsValid());
    }
}
}
}
}

#endif
