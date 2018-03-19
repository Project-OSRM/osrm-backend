#ifndef GRAPH_LOADER_HPP
#define GRAPH_LOADER_HPP

#include "extractor/node_based_edge.hpp"
#include "extractor/packed_osm_ids.hpp"
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
#include <boost/function_output_iterator.hpp>

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
                         extractor::PackedOSMIDs &osm_node_ids)
{
    auto number_of_nodes = file_reader.ReadElementCount64();
    Log() << "Importing number_of_nodes new = " << number_of_nodes << " nodes ";

    coordinates.resize(number_of_nodes);
    osm_node_ids.reserve(number_of_nodes);

    auto index = 0;
    auto decode = [&](const extractor::QueryNode &current_node) {
        coordinates[index].lon = current_node.lon;
        coordinates[index].lat = current_node.lat;
        osm_node_ids.push_back(current_node.node_id);
        index++;
    };

    file_reader.ReadStreaming<extractor::QueryNode>(boost::make_function_output_iterator(decode),
                                                    number_of_nodes);

    auto num_barriers = file_reader.ReadElementCount64();
    file_reader.ReadStreaming<NodeID>(barriers, num_barriers);

    auto num_lights = file_reader.ReadElementCount64();
    file_reader.ReadStreaming<NodeID>(traffic_signals, num_lights);

    return number_of_nodes;
}

/**
 * Reads a .osrm file and produces the edges.
 */
inline EdgeID loadEdgesFromFile(storage::io::FileReader &file_reader,
                                std::vector<extractor::NodeBasedEdge> &edge_list)
{
    auto number_of_edges = file_reader.ReadElementCount64();

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
        BOOST_ASSERT_MSG(edge.flags.forward, "edge must be oriented in forward direction");

        BOOST_ASSERT_MSG(edge.source != edge.target, "loaded edges contain a loop");
        BOOST_ASSERT_MSG(edge.source != prev_edge.source || edge.target != prev_edge.target,
                         "loaded edges contain a multi edge");
    }
#endif

    Log() << "Graph loaded ok and has " << edge_list.size() << " edges";

    return number_of_edges;
}

inline EdgeID loadAnnotationData(storage::io::FileReader &file_reader,
                                 std::vector<extractor::NodeBasedEdgeAnnotation> &metadata)
{
    auto const meta_data_count = file_reader.ReadElementCount64();
    metadata.resize(meta_data_count);
    file_reader.ReadInto(metadata.data(), meta_data_count);
    return meta_data_count;
}
}
}

#endif // GRAPH_LOADER_HPP
