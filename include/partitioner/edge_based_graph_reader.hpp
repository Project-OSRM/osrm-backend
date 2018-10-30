#ifndef OSRM_PARTITIONER_EDGE_BASED_GRAPH_READER_HPP
#define OSRM_PARTITIONER_EDGE_BASED_GRAPH_READER_HPP

#include "partitioner/edge_based_graph.hpp"

#include "extractor/edge_based_edge.hpp"
#include "extractor/files.hpp"
#include "storage/io.hpp"
#include "util/coordinate.hpp"
#include "util/dynamic_graph.hpp"
#include "util/typedefs.hpp"

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>

#include <cstdint>

#include <algorithm>
#include <iterator>
#include <memory>
#include <vector>

namespace osrm
{
namespace partitioner
{

// Bidirectional (s,t) to (s,t) and (t,s)
inline std::vector<extractor::EdgeBasedEdge>
splitBidirectionalEdges(const std::vector<extractor::EdgeBasedEdge> &edges)
{
    std::vector<extractor::EdgeBasedEdge> directed;
    directed.reserve(edges.size() * 2);

    for (const auto &edge : edges)
    {
        if (edge.data.weight == INVALID_EDGE_WEIGHT)
            continue;

        directed.emplace_back(edge.source,
                              edge.target,
                              edge.data.turn_id,
                              std::max(edge.data.weight, 1),
                              edge.data.duration,
                              edge.data.distance,
                              edge.data.forward,
                              edge.data.backward);

        directed.emplace_back(edge.target,
                              edge.source,
                              edge.data.turn_id,
                              std::max(edge.data.weight, 1),
                              edge.data.duration,
                              edge.data.distance,
                              edge.data.backward,
                              edge.data.forward);
    }

    return directed;
}

template <typename OutputEdgeT>
std::vector<OutputEdgeT> prepareEdgesForUsageInGraph(std::vector<extractor::EdgeBasedEdge> edges)
{
    // sort into blocks of edges with same source + target
    // the we partition by the forward flag to sort all edges with a forward direction first.
    // the we sort by weight to ensure the first forward edge is the smallest forward edge
    std::sort(begin(edges), end(edges), [](const auto &lhs, const auto &rhs) {
        return std::tie(lhs.source, lhs.target, rhs.data.forward, lhs.data.weight) <
               std::tie(rhs.source, rhs.target, lhs.data.forward, rhs.data.weight);
    });

    std::vector<OutputEdgeT> output_edges;
    output_edges.reserve(edges.size());

    for (auto begin_interval = edges.begin(); begin_interval != edges.end();)
    {
        const NodeID source = begin_interval->source;
        const NodeID target = begin_interval->target;

        auto end_interval =
            std::find_if_not(begin_interval, edges.end(), [source, target](const auto &edge) {
                return std::tie(edge.source, edge.target) == std::tie(source, target);
            });
        BOOST_ASSERT(begin_interval != end_interval);

        // remove eigenloops
        if (source == target)
        {
            begin_interval = end_interval;
            continue;
        }

        BOOST_ASSERT_MSG(begin_interval->data.forward != begin_interval->data.backward,
                         "The forward and backward flag need to be mutally exclusive");

        // find smallest backward edge and check if we can merge
        auto first_backward = std::find_if(
            begin_interval, end_interval, [](const auto &edge) { return edge.data.backward; });

        // thanks to the sorting we know this is the smallest backward edge
        // and there is no forward edge
        if (begin_interval == first_backward)
        {
            output_edges.push_back(OutputEdgeT{source, target, first_backward->data});
        }
        // only a forward edge, thanks to the sorting this is the smallest
        else if (first_backward == end_interval)
        {
            output_edges.push_back(OutputEdgeT{source, target, begin_interval->data});
        }
        // we have both a forward and a backward edge, we need to evaluate
        // if we can merge them
        else
        {
            BOOST_ASSERT(begin_interval->data.forward);
            BOOST_ASSERT(first_backward->data.backward);
            BOOST_ASSERT(first_backward != end_interval);

            // same weight, so we can just merge them
            if (begin_interval->data.weight == first_backward->data.weight)
            {
                OutputEdgeT merged{source, target, begin_interval->data};
                merged.data.backward = true;
                output_edges.push_back(std::move(merged));
            }
            // we need to insert separate forward and reverse edges
            else
            {
                output_edges.push_back(OutputEdgeT{source, target, begin_interval->data});
                output_edges.push_back(OutputEdgeT{source, target, first_backward->data});
            }
        }

        begin_interval = end_interval;
    }

    return output_edges;
}

inline std::vector<extractor::EdgeBasedEdge>
graphToEdges(const DynamicEdgeBasedGraph &edge_based_graph)
{
    auto range = tbb::blocked_range<NodeID>(0, edge_based_graph.GetNumberOfNodes());
    auto max_turn_id =
        tbb::parallel_reduce(range,
                             NodeID{0},
                             [&edge_based_graph](const auto range, NodeID initial) {
                                 NodeID max_turn_id = initial;
                                 for (auto node = range.begin(); node < range.end(); ++node)
                                 {
                                     for (auto edge : edge_based_graph.GetAdjacentEdgeRange(node))
                                     {
                                         const auto &data = edge_based_graph.GetEdgeData(edge);
                                         max_turn_id = std::max(max_turn_id, data.turn_id);
                                     }
                                 }
                                 return max_turn_id;
                             },
                             [](const NodeID lhs, const NodeID rhs) { return std::max(lhs, rhs); });

    std::vector<extractor::EdgeBasedEdge> edges(max_turn_id + 1);
    tbb::parallel_for(range, [&](const auto range) {
        for (auto node = range.begin(); node < range.end(); ++node)
        {
            for (auto edge : edge_based_graph.GetAdjacentEdgeRange(node))
            {
                const auto &data = edge_based_graph.GetEdgeData(edge);
                // we only need to save the forward edges, since the read method will
                // convert from forward to bi-directional edges again
                if (data.forward)
                {
                    auto target = edge_based_graph.GetTarget(edge);
                    BOOST_ASSERT(data.turn_id <= max_turn_id);
                    edges[data.turn_id] = extractor::EdgeBasedEdge{node, target, data};
                    // only save the forward edge
                    edges[data.turn_id].data.forward = true;
                    edges[data.turn_id].data.backward = false;
                }
            }
        }
    });

    return edges;
}

inline DynamicEdgeBasedGraph LoadEdgeBasedGraph(const boost::filesystem::path &path)
{
    EdgeID number_of_edge_based_nodes;
    std::vector<extractor::EdgeBasedEdge> edges;
    std::uint32_t checksum;
    extractor::files::readEdgeBasedGraph(path, number_of_edge_based_nodes, edges, checksum);

    auto directed = splitBidirectionalEdges(edges);
    auto tidied = prepareEdgesForUsageInGraph<DynamicEdgeBasedGraphEdge>(std::move(directed));

    return DynamicEdgeBasedGraph(number_of_edge_based_nodes, std::move(tidied), checksum);
}

} // ns partition
} // ns osrm

#endif
