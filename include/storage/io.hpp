#ifndef OSRM_STORAGE_IO_HPP_
#define OSRM_STORAGE_IO_HPP_

#include "contractor/query_edge.hpp"
#include "extractor/extractor.hpp"
#include "extractor/original_edge_data.hpp"
#include "extractor/query_node.hpp"
#include "util/fingerprint.hpp"
#include "util/simple_logger.hpp"
#include "util/static_graph.hpp"

#include <boost/filesystem/fstream.hpp>

#include <tuple>

namespace osrm
{
namespace storage
{
namespace io
{

// Reads the count of elements that is written in the file header and returns the number
inline std::uint64_t readElementCount(boost::filesystem::ifstream &input_stream)
{
    std::uint64_t number_of_elements = 0;
    input_stream.read((char *)&number_of_elements, sizeof(std::uint64_t));
    return number_of_elements;
}

// To make function calls consistent, this function returns the fixed number of properties
inline std::size_t readPropertiesCount() { return 1; }

// Returns the number of bytes in a file
inline std::size_t readNumberOfBytes(boost::filesystem::ifstream &input_stream)
{
    input_stream.seekg(0, input_stream.end);
    auto length = input_stream.tellg();
    input_stream.seekg(0, input_stream.beg);
    return length;
}

#pragma pack(push, 1)
struct HSGRHeader
{
    std::uint32_t checksum;
    std::uint64_t number_of_nodes;
    std::uint64_t number_of_edges;
};
#pragma pack(pop)
static_assert(sizeof(HSGRHeader) == 20, "HSGRHeader is not packed");

// Reads the checksum, number of nodes and number of edges written in the header file of a `.hsgr`
// file and returns them in a HSGRHeader struct
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

// Reads the graph data of a `.hsgr` file into memory
// Needs to be called after readHSGRHeader() to get the correct offset in the stream
using NodeT = typename util::StaticGraph<contractor::QueryEdge::EdgeData>::NodeArrayEntry;
using EdgeT = typename util::StaticGraph<contractor::QueryEdge::EdgeData>::EdgeArrayEntry;
inline void readHSGR(boost::filesystem::ifstream &input_stream,
              NodeT *node_buffer,
              const std::uint64_t number_of_nodes,
              EdgeT *edge_buffer,
              const std::uint64_t number_of_edges)
{
    BOOST_ASSERT(node_buffer);
    BOOST_ASSERT(edge_buffer);
    input_stream.read(reinterpret_cast<char *>(node_buffer), number_of_nodes * sizeof(NodeT));
    input_stream.read(reinterpret_cast<char *>(edge_buffer), number_of_edges * sizeof(EdgeT));
}

// Loads properties from a `.properties` file into memory
inline void readProperties(boost::filesystem::ifstream &properties_stream,
                           extractor::ProfileProperties *properties,
                           const std::size_t properties_size)
{
    BOOST_ASSERT(properties);
    properties_stream.read(reinterpret_cast<char *>(properties), properties_size);
}

// Reads the timestamp in a `.timestamp` file
// Use readNumberOfBytes() beforehand to get the length of the file
inline void readTimestamp(boost::filesystem::ifstream &timestamp_input_stream,
                          char *timestamp,
                          const std::size_t timestamp_length)
{
    BOOST_ASSERT(timestamp);
    timestamp_input_stream.read(timestamp, timestamp_length * sizeof(char));
}

// Loads datasource_indexes from .datasource_indexes into memory
// Needs to be called after readElementCount() to get the correct offset in the stream
inline void readDatasourceIndexes(boost::filesystem::ifstream &datasource_indexes_input_stream,
                                  uint8_t *datasource_buffer,
                                  const std::uint64_t number_of_datasource_indexes)
{
    BOOST_ASSERT(datasource_buffer);
    datasource_indexes_input_stream.read(reinterpret_cast<char *>(datasource_buffer),
                                         number_of_datasource_indexes * sizeof(std::uint8_t));
}

// Loads edge data from .edge files into memory which includes its
// geometry, name ID, turn instruction, lane data ID, travel mode, entry class ID
// Needs to be called after readElementCount() to get the correct offset in the stream
inline void readEdges(boost::filesystem::ifstream &edges_input_stream,
                      GeometryID *geometry_list,
                      NameID *name_id_list,
                      extractor::guidance::TurnInstruction *turn_instruction_list,
                      LaneDataID *lane_data_id_list,
                      extractor::TravelMode *travel_mode_list,
                      EntryClassID *entry_class_id_list,
                      util::guidance::TurnBearing *pre_turn_bearing_list,
                      util::guidance::TurnBearing *post_turn_bearing_list,
                      const std::uint64_t number_of_edges)
{
    BOOST_ASSERT(geometry_list);
    BOOST_ASSERT(name_id_list);
    BOOST_ASSERT(turn_instruction_list);
    BOOST_ASSERT(lane_data_id_list);
    BOOST_ASSERT(travel_mode_list);
    BOOST_ASSERT(entry_class_id_list);
    extractor::OriginalEdgeData current_edge_data;
    for (std::uint64_t i = 0; i < number_of_edges; ++i)
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

// Loads coordinates and OSM node IDs from .nodes files into memory
// Needs to be called after readElementCount() to get the correct offset in the stream
template <typename OSMNodeIDVectorT>
void readNodes(boost::filesystem::ifstream &nodes_input_stream,
               util::Coordinate *coordinate_list,
               OSMNodeIDVectorT &osmnodeid_list,
               const std::uint64_t number_of_coordinates)
{
    BOOST_ASSERT(coordinate_list);
    extractor::QueryNode current_node;
    for (std::uint64_t i = 0; i < number_of_coordinates; ++i)
    {
        nodes_input_stream.read((char *)&current_node, sizeof(extractor::QueryNode));
        coordinate_list[i] = util::Coordinate(current_node.lon, current_node.lat);
        osmnodeid_list.push_back(current_node.node_id);
        BOOST_ASSERT(coordinate_list[i].IsValid());
    }
}


// Reads datasource names out of .datasource_names files and metadata such as
// the length and offset of each name
struct DatasourceNamesData
{
    std::vector<char> names;
    std::vector<std::size_t> offsets;
    std::vector<std::size_t> lengths;
};
inline DatasourceNamesData readDatasourceNames(boost::filesystem::ifstream &datasource_names_input_stream)
{
    DatasourceNamesData datasource_names_data;
    std::string name;
    while (std::getline(datasource_names_input_stream, name))
    {
        datasource_names_data.offsets.push_back(datasource_names_data.names.size());
        datasource_names_data.lengths.push_back(name.size());
        std::copy(name.c_str(),
                  name.c_str() + name.size(),
                  std::back_inserter(datasource_names_data.names));
    }
    return datasource_names_data;
}

// Loads ram indexes of R-Trees from `.ramIndex` files into memory
// Needs to be called after readElementCount() to get the correct offset in the stream
// template <bool UseSharedMemory>
// NB Cannot be written without templated type because of cyclic depencies between
// `static_rtree.hpp` and `io.hpp`
template <typename RTreeNodeT>
void readRamIndex(boost::filesystem::ifstream &ram_index_input_stream,
                  RTreeNodeT *rtree_buffer,
                  const std::uint64_t tree_size)
{
    BOOST_ASSERT(rtree_buffer);
    ram_index_input_stream.read(reinterpret_cast<char *>(rtree_buffer),
                                sizeof(RTreeNodeT) * tree_size);
}

}
}
}

#endif
