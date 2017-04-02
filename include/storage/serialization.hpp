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
}
}
}

#endif
