#ifndef OSRM_COMPRESSED_NODE_BASED_GRAPH_READER_HPP
#define OSRM_COMPRESSED_NODE_BASED_GRAPH_READER_HPP

#include "storage/io.hpp"
#include "util/coordinate.hpp"
#include "util/typedefs.hpp"

#include <string>
#include <vector>

namespace osrm
{
namespace partition
{

struct CompressedNodeBasedGraphEdge
{
    NodeID source;
    NodeID target;
};

struct CompressedNodeBasedGraph
{
    CompressedNodeBasedGraph(storage::io::FileReader &reader)
    {
        // Reads:  | Fingerprint | #e | #n | edges | coordinates |
        // - uint64: number of edges (from, to) pairs
        // - uint64: number of nodes and therefore also coordinates
        // - (uint32_t, uint32_t): num_edges * edges
        // - (int32_t, int32_t: num_nodes * coordinates (lon, lat)
        //
        // Gets written in Extractor::WriteCompressedNodeBasedGraph

        const auto num_edges = reader.ReadElementCount64();
        const auto num_nodes = reader.ReadElementCount64();

        edges.resize(num_edges);
        coordinates.resize(num_nodes);

        reader.ReadInto(edges);
        reader.ReadInto(coordinates);
    }

    std::vector<CompressedNodeBasedGraphEdge> edges;
    std::vector<util::Coordinate> coordinates;
};

inline CompressedNodeBasedGraph LoadCompressedNodeBasedGraph(const std::string &path)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader(path, fingerprint);

    CompressedNodeBasedGraph graph{reader};
    return graph;
}

} // ns partition
} // ns osrm

#endif
