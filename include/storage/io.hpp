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
    input_stream.read(reinterpret_cast<char *>(&header.number_of_nodes), sizeof(header.number_of_nodes));
    input_stream.read(reinterpret_cast<char *>(&header.number_of_edges), sizeof(header.number_of_edges));

    BOOST_ASSERT_MSG(0 != header.number_of_nodes, "number of nodes is zero");
    // number of edges can be zero, this is the case in a few test fixtures

    return header;
}

// Needs to be called after HSGRHeader() to get the correct offset in the stream
template <typename NodeT, typename EdgeT>
void readHSGR(boost::filesystem::ifstream &input_stream,
              NodeT *node_buffer,
              std::uint32_t number_of_nodes,
              EdgeT *edge_buffer,
              std::uint32_t number_of_edges)
{
    input_stream.read(reinterpret_cast<char *>(node_buffer), number_of_nodes * sizeof(NodeT));
    input_stream.read(reinterpret_cast<char *>(edge_buffer), number_of_edges * sizeof(EdgeT));
}

// Returns the size of the timestamp in a file
inline std::uint32_t readTimestampSize(boost::filesystem::ifstream &timestamp_input_stream)
{
    timestamp_input_stream.seekg (0, timestamp_input_stream.end);
    auto length = timestamp_input_stream.tellg();
    timestamp_input_stream.seekg (0, timestamp_input_stream.beg);
    return length;
}

// Reads the timestamp in a file
inline void readTimestamp(boost::filesystem::ifstream &timestamp_input_stream, char *timestamp, std::size_t timestamp_length)
{
    timestamp_input_stream.read(timestamp, timestamp_length * sizeof(char));
}

}
}
}

#endif
