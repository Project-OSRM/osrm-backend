#ifndef GRAPH_LOADER_HPP
#define GRAPH_LOADER_HPP

#include "extractor/external_memory_node.hpp"
#include "extractor/node_based_edge.hpp"
#include "extractor/query_node.hpp"
#include "extractor/restriction.hpp"
#include "storage/io.hpp"
#include "util/exception.hpp"
#include "util/fingerprint.hpp"
#include "util/log.hpp"
#include "util/packed_vector.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <tbb/parallel_sort.h>

#include <cmath>

#include <fstream>
#include <ios>
#include <unordered_set>
#include <vector>

namespace osrm
{
namespace util
{

/**
 * Reads the .restrictions file and loads it to a vector.
 * The since the restrictions reference nodes using their external node id,
 * we need to renumber it to the new internal id.
*/
inline unsigned loadRestrictionsFromFile(storage::io::FileReader &file_reader,
                                         std::vector<extractor::TurnRestriction> &restriction_list)
{
    unsigned number_of_usable_restrictions = file_reader.ReadElementCount32();
    restriction_list.resize(number_of_usable_restrictions);
    if (number_of_usable_restrictions > 0)
    {
        file_reader.ReadInto(restriction_list.data(), number_of_usable_restrictions);
    }

    return number_of_usable_restrictions;
}

/**
 * Reads the beginning of an .osrm file and produces:
 *  - barrier nodes
 *  - traffic lights
 *  - nodes indexed by their internal (non-osm) id
 */
template <typename BarrierOutIter, typename TrafficSignalsOutIter>
NodeID loadNodesFromFile(storage::io::FileReader &file_reader,
                         BarrierOutIter barriers,
                         TrafficSignalsOutIter traffic_signals,
                         std::vector<util::Coordinate> &coordinates,
                         util::PackedVector<OSMNodeID> &osm_node_ids)
{
    NodeID number_of_nodes = file_reader.ReadElementCount32();
    Log() << "Importing number_of_nodes new = " << number_of_nodes << " nodes ";

    coordinates.resize(number_of_nodes);
    osm_node_ids.reserve(number_of_nodes);

    extractor::ExternalMemoryNode current_node;
    for (NodeID i = 0; i < number_of_nodes; ++i)
    {
        file_reader.ReadInto(&current_node, 1);

        coordinates[i].lon = current_node.lon;
        coordinates[i].lat = current_node.lat;
        osm_node_ids.push_back(current_node.node_id);

        if (current_node.barrier)
        {
            *barriers = i;
            ++barriers;
        }

        if (current_node.traffic_lights)
        {
            *traffic_signals = i;
            ++traffic_signals;
        }
    }

    return number_of_nodes;
}

/**
 * Reads a .osrm file and produces the edges.
 */
inline NodeID loadEdgesFromFile(storage::io::FileReader &file_reader,
                                std::vector<extractor::NodeBasedEdge> &edge_list)
{
    EdgeID number_of_edges = file_reader.ReadElementCount32();
    BOOST_ASSERT(sizeof(EdgeID) == sizeof(number_of_edges));

    edge_list.resize(number_of_edges);
    Log() << " and " << number_of_edges << " edges ";

    file_reader.ReadInto(edge_list.data(), number_of_edges);

    BOOST_ASSERT(edge_list.size() > 0);

#ifndef NDEBUG
    Log() << "Validating loaded edges...";
    tbb::parallel_sort(
        edge_list.begin(),
        edge_list.end(),
        [](const extractor::NodeBasedEdge &lhs, const extractor::NodeBasedEdge &rhs) {
            return (lhs.source < rhs.source) ||
                   (lhs.source == rhs.source && lhs.target < rhs.target);
        });
    for (auto i = 1u; i < edge_list.size(); ++i)
    {
        const auto &edge = edge_list[i];
        const auto &prev_edge = edge_list[i - 1];

        BOOST_ASSERT_MSG(edge.weight > 0, "loaded null weight");
        BOOST_ASSERT_MSG(edge.forward, "edge must be oriented in forward direction");
        BOOST_ASSERT_MSG(edge.travel_mode != TRAVEL_MODE_INACCESSIBLE, "loaded non-accessible");

        BOOST_ASSERT_MSG(edge.source != edge.target, "loaded edges contain a loop");
        BOOST_ASSERT_MSG(edge.source != prev_edge.source || edge.target != prev_edge.target,
                         "loaded edges contain a multi edge");
    }
#endif

    Log() << "Graph loaded ok and has " << edge_list.size() << " edges";

    return number_of_edges;
}
}
}

#endif // GRAPH_LOADER_HPP
