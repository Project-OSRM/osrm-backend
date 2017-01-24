#ifndef OSRM_STORAGE_SERIALIZATION_HPP_
#define OSRM_STORAGE_SERIALIZATION_HPP_

#include "contractor/query_edge.hpp"
#include "extractor/extractor.hpp"
#include "extractor/original_edge_data.hpp"
#include "extractor/query_node.hpp"
#include "storage/io.hpp"
#include "util/exception.hpp"
#include "util/fingerprint.hpp"
#include "util/log.hpp"
#include "util/static_graph.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/seek.hpp>

#include <cerrno>
#include <cstring>
#include <tuple>
#include <type_traits>

namespace osrm
{
namespace storage
{
namespace serialization
{

// To make function calls consistent, this function returns the fixed number of properties
inline std::size_t readPropertiesCount() { return 1; }

struct HSGRHeader
{
    std::uint32_t checksum;
    std::uint64_t number_of_nodes;
    std::uint64_t number_of_edges;
};

// Reads the checksum, number of nodes and number of edges written in the header file of a `.hsgr`
// file and returns them in a HSGRHeader struct
inline HSGRHeader readHSGRHeader(io::FileReader &input_file)
{

    HSGRHeader header;
    input_file.ReadInto(header.checksum);
    input_file.ReadInto(header.number_of_nodes);
    input_file.ReadInto(header.number_of_edges);

    // If we have edges, then we must have nodes.
    // However, there can be nodes with no edges (some test cases create this)
    BOOST_ASSERT_MSG(header.number_of_edges == 0 || header.number_of_nodes > 0,
                     "edges exist, but there are no nodes");

    return header;
}

// Reads the graph data of a `.hsgr` file into memory
// Needs to be called after readHSGRHeader() to get the correct offset in the stream
using NodeT = typename util::StaticGraph<contractor::QueryEdge::EdgeData>::NodeArrayEntry;
using EdgeT = typename util::StaticGraph<contractor::QueryEdge::EdgeData>::EdgeArrayEntry;
inline void readHSGR(io::FileReader &input_file,
                     NodeT *node_buffer,
                     const std::uint64_t number_of_nodes,
                     EdgeT *edge_buffer,
                     const std::uint64_t number_of_edges)
{
    BOOST_ASSERT(node_buffer);
    BOOST_ASSERT(edge_buffer);
    input_file.ReadInto(node_buffer, number_of_nodes);
    input_file.ReadInto(edge_buffer, number_of_edges);
}

// Loads datasource_indexes from .datasource_indexes into memory
// Needs to be called after readElementCount() to get the correct offset in the stream
inline void readDatasourceIndexes(io::FileReader &datasource_indexes_file,
                                  uint8_t *datasource_buffer,
                                  const std::uint64_t number_of_datasource_indexes)
{
    BOOST_ASSERT(datasource_buffer);
    datasource_indexes_file.ReadInto(datasource_buffer, number_of_datasource_indexes);
}

// Loads edge data from .edge files into memory which includes its
// geometry, name ID, turn instruction, lane data ID, travel mode, entry class ID
// Needs to be called after readElementCount() to get the correct offset in the stream
inline void readEdges(io::FileReader &edges_input_file,
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
        edges_input_file.ReadInto(current_edge_data);

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
void readNodes(io::FileReader &nodes_file,
               util::Coordinate *coordinate_list,
               OSMNodeIDVectorT &osmnodeid_list,
               const std::uint64_t number_of_coordinates)
{
    BOOST_ASSERT(coordinate_list);
    extractor::QueryNode current_node;
    for (std::uint64_t i = 0; i < number_of_coordinates; ++i)
    {
        nodes_file.ReadInto(current_node);
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
inline DatasourceNamesData readDatasourceNames(io::FileReader &datasource_names_file)
{
    DatasourceNamesData datasource_names_data;
    std::vector<std::string> lines = datasource_names_file.ReadLines();
    for (const auto &name : lines)
    {
        datasource_names_data.offsets.push_back(datasource_names_data.names.size());
        datasource_names_data.lengths.push_back(name.size());
        std::copy(name.c_str(),
                  name.c_str() + name.size(),
                  std::back_inserter(datasource_names_data.names));
    }
    return datasource_names_data;
}
}
}
}

#endif
