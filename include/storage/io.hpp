#ifndef OSRM_STORAGE_IO_HPP_
#define OSRM_STORAGE_IO_HPP_

#include "extractor/original_edge_data.hpp"
#include "extractor/query_node.hpp"
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

// Needs to be called after HSGRHeader() to get the correct offset in the stream
template <typename NodeT, typename EdgeT>
void readHSGR(boost::filesystem::ifstream &input_stream,
              NodeT node_buffer[],
              std::uint32_t number_of_nodes,
              EdgeT edge_buffer[],
              std::uint32_t number_of_edges)
{
    BOOST_ASSERT(node_buffer);
    BOOST_ASSERT(edge_buffer);
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
    BOOST_ASSERT(timestamp);
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
// Needs to be called after readEdgesSize() to get the correct offset in the stream
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
    BOOST_ASSERT(geometry_list);
    BOOST_ASSERT(name_id_list);
    BOOST_ASSERT(turn_instruction_list);
    BOOST_ASSERT(lane_data_id_list);
    BOOST_ASSERT(travel_mode_list);
    BOOST_ASSERT(entry_class_id_list);
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
// Needs to be called after readNodesSize() to get the correct offset in the stream
template <typename CoordinateT, typename OSMNodeIDVectorT>
void readNodesData(boost::filesystem::ifstream &nodes_input_stream,
                   CoordinateT coordinate_list[],
                   OSMNodeIDVectorT &osmnodeid_list,
                   std::uint32_t number_of_coordinates)
{
    BOOST_ASSERT(coordinate_list);
    extractor::QueryNode current_node;
    for (std::uint32_t i = 0; i < number_of_coordinates; ++i)
    {
        nodes_input_stream.read((char *)&current_node, sizeof(extractor::QueryNode));
        coordinate_list[i] = CoordinateT(current_node.lon, current_node.lat);
        osmnodeid_list.push_back(current_node.node_id);
        BOOST_ASSERT(coordinate_list[i].IsValid());
    }
}

// Returns the number of indexes in a .datasource_indexes file
inline std::uint64_t
readDatasourceIndexesSize(boost::filesystem::ifstream &datasource_indexes_input_stream)
{
    std::uint64_t number_of_datasource_indexes;
    datasource_indexes_input_stream.read(reinterpret_cast<char *>(&number_of_datasource_indexes),
                                         sizeof(std::uint64_t));
    return number_of_datasource_indexes;
}

// Reads datasource_indexes
// Needs to be called after readDatasourceIndexesSize() to get the correct offset in the stream
inline void readDatasourceIndexes(boost::filesystem::ifstream &datasource_indexes_input_stream,
                                  uint8_t datasource_buffer[],
                                  std::uint32_t number_of_datasource_indexes)
{
    BOOST_ASSERT(datasource_buffer);
    datasource_indexes_input_stream.read(reinterpret_cast<char *>(datasource_buffer),
                                         number_of_datasource_indexes * sizeof(std::uint8_t));
}

// Reads datasource names out of .datasource_names files and metadata such as
// the length and offset of each name
struct DatasourceNamesData
{
    std::vector<char> names;
    std::vector<std::uint32_t> offsets;
    std::vector<std::uint32_t> lengths;
};
inline DatasourceNamesData
readDatasourceNamesData(boost::filesystem::ifstream &datasource_names_input_stream)
{
    DatasourceNamesData datasource_names_data;
    if (datasource_names_input_stream)
    {
        std::string name;
        while (std::getline(datasource_names_input_stream, name))
        {
            datasource_names_data.offsets.push_back(datasource_names_data.names.size());
            datasource_names_data.lengths.push_back(name.size());
            std::copy(name.c_str(),
                      name.c_str() + name.size(),
                      std::back_inserter(datasource_names_data.names));
        }
    }
    return datasource_names_data;
}

// Returns the number of ram indexes
inline std::uint32_t readRamIndexSize(boost::filesystem::ifstream &ram_index_input_stream)
{
    std::uint32_t tree_size = 0;
    ram_index_input_stream.read((char *)&tree_size, sizeof(std::uint32_t));
    return tree_size;
}

template <typename RTreeNodeT>
void readRamIndexData(boost::filesystem::ifstream &ram_index_input_stream,
                 RTreeNodeT rtree_buffer[],
                 std::uint32_t tree_size)
{
    BOOST_ASSERT(rtree_buffer);
    ram_index_input_stream.read(reinterpret_cast<char *>(rtree_buffer), sizeof(RTreeNodeT) * tree_size);
}

}
}
}

#endif
