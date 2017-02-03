#ifndef OSRM_NODE_BASED_GRAPH_TO_EDGE_BASED_GRAPH_MAPPING_READER_HPP
#define OSRM_NODE_BASED_GRAPH_TO_EDGE_BASED_GRAPH_MAPPING_READER_HPP

#include "storage/io.hpp"
#include "util/typedefs.hpp"

#include <cstddef>

#include <iterator>
#include <unordered_map>
#include <vector>

#include <boost/assert.hpp>

namespace osrm
{
namespace partition
{

struct NodeBasedGraphToEdgeBasedGraphMapping
{
    NodeBasedGraphToEdgeBasedGraphMapping(storage::io::FileReader &reader)
    {
        // Reads:  | Fingerprint | #mappings | u v fwd_node bkw_node | u v fwd_node bkw_node | ..
        // - uint64: number of mappings (u, v, fwd_node, bkw_node) chunks
        // - NodeID u, NodeID v, EdgeID fwd_node, EdgeID bkw_node
        //
        // Gets written in NodeBasedGraphToEdgeBasedGraphMappingWriter

        const auto num_mappings = reader.ReadElementCount64();

        edge_based_node_to_node_based_nodes.reserve(num_mappings * 2);

        for (std::uint64_t i{0}; i < num_mappings; ++i)
        {

            const auto u = reader.ReadOne<NodeID>();            // node based graph `from` node
            const auto v = reader.ReadOne<NodeID>();            // node based graph `to` node
            const auto fwd_ebg_node = reader.ReadOne<EdgeID>(); // edge based graph forward node
            const auto bkw_ebg_node = reader.ReadOne<EdgeID>(); // edge based graph backward node

            edge_based_node_to_node_based_nodes.insert({fwd_ebg_node, {u, v}});
            edge_based_node_to_node_based_nodes.insert({bkw_ebg_node, {v, u}});
        }
    }

    struct NodeBasedNodes
    {
        NodeID u, v;
    };

    NodeBasedNodes Lookup(EdgeID edge_based_node) const
    {
        auto it = edge_based_node_to_node_based_nodes.find(edge_based_node);

        if (it != end(edge_based_node_to_node_based_nodes))
            return it->second;

        BOOST_ASSERT_MSG(false, "unable to fine edge based node, graph <-> mapping out of sync");
        return NodeBasedNodes{SPECIAL_NODEID, SPECIAL_NODEID};
    }

  private:
    std::unordered_map<EdgeID, NodeBasedNodes> edge_based_node_to_node_based_nodes;
};

inline NodeBasedGraphToEdgeBasedGraphMapping
LoadNodeBasedGraphToEdgeBasedGraphMapping(const std::string &path)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader(path, fingerprint);

    NodeBasedGraphToEdgeBasedGraphMapping mapping{reader};
    return mapping;
}

} // ns partition
} // ns osrm

#endif
