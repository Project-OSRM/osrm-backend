#ifndef NODE_BASED_GRAPH_TO_EDGE_BASED_GRAPH_MAPPING_WRITER_HPP
#define NODE_BASED_GRAPH_TO_EDGE_BASED_GRAPH_MAPPING_WRITER_HPP

#include "storage/io.hpp"
#include "util/log.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <cstddef>

#include <string>

namespace osrm
{
namespace extractor
{

// Writes:  | Fingerprint | #mappings | u v fwd_node bkw_node | u v fwd_node bkw_node | ..
// - uint64: number of mappings (u, v, fwd_node bkw_node) chunks
// - NodeID u, NodeID v, EdgeID fwd_node, EdgeID bkw_node

struct NodeBasedGraphToEdgeBasedGraphMappingWriter
{
    NodeBasedGraphToEdgeBasedGraphMappingWriter(const std::string &path)
        : writer{path, storage::io::FileWriter::GenerateFingerprint}, num_written{0}
    {
        const std::uint64_t dummy{0}; // filled in later
        writer.WriteElementCount64(dummy);
    }

    void WriteMapping(NodeID u, NodeID v, EdgeID fwd_ebg_node, EdgeID bkw_ebg_node)
    {
        BOOST_ASSERT(u != SPECIAL_NODEID);
        BOOST_ASSERT(v != SPECIAL_NODEID);
        BOOST_ASSERT(fwd_ebg_node != SPECIAL_EDGEID || bkw_ebg_node != SPECIAL_EDGEID);

        writer.WriteOne(u);
        writer.WriteOne(v);
        writer.WriteOne(fwd_ebg_node);
        writer.WriteOne(bkw_ebg_node);

        num_written += 1;
    }

    ~NodeBasedGraphToEdgeBasedGraphMappingWriter()
    {
        if (num_written != 0)
        {
            writer.SkipToBeginning();
            writer.WriteOne(num_written);
        }
    }

  private:
    storage::io::FileWriter writer;
    std::uint64_t num_written;
};

} // ns extractor
} // ns osrm

#endif // NODE_BASED_GRAPH_TO_EDGE_BASED_GRAPH_MAPPING_WRITER_HPP
