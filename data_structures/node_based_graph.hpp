/*

Copyright (c) 2015, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef NODE_BASED_GRAPH_HPP
#define NODE_BASED_GRAPH_HPP

#include "dynamic_graph.hpp"
#include "import_edge.hpp"
#include "../util/graph_utils.hpp"

#include <tbb/parallel_sort.h>

#include <memory>

struct NodeBasedEdgeData
{
    NodeBasedEdgeData()
        : distance(INVALID_EDGE_WEIGHT), edge_id(SPECIAL_NODEID),
          name_id(std::numeric_limits<unsigned>::max()), access_restricted(false),
          reversed(false), roundabout(false), travel_mode(TRAVEL_MODE_INACCESSIBLE)
    {
    }

    NodeBasedEdgeData(int distance, unsigned edge_id, unsigned name_id,
            bool access_restricted, bool reversed,
            bool roundabout, TravelMode travel_mode)
        : distance(distance), edge_id(edge_id), name_id(name_id),
          access_restricted(access_restricted), reversed(reversed),
          roundabout(roundabout), travel_mode(travel_mode)
    {
    }

    int distance;
    unsigned edge_id;
    unsigned name_id;
    bool access_restricted : 1;
    bool reversed : 1;
    bool roundabout : 1;
    TravelMode travel_mode : 4;

    bool IsCompatibleTo(const NodeBasedEdgeData &other) const
    {
        return (reversed == other.reversed) && (name_id == other.name_id) &&
               (travel_mode == other.travel_mode);
    }
};

using NodeBasedDynamicGraph = DynamicGraph<NodeBasedEdgeData>;

/// Factory method to create NodeBasedDynamicGraph from NodeBasedEdges
/// The since DynamicGraph expects directed edges, we need to insert
/// two edges for undirected edges.
inline std::shared_ptr<NodeBasedDynamicGraph>
NodeBasedDynamicGraphFromEdges(std::size_t number_of_nodes, const std::vector<NodeBasedEdge> &input_edge_list)
{
    auto edges_list = directedEdgesFromCompressed<NodeBasedDynamicGraph::InputEdge>(input_edge_list,
        [](NodeBasedDynamicGraph::InputEdge& output_edge, const NodeBasedEdge& input_edge)
        {
            output_edge.data.distance = static_cast<int>(input_edge.weight);
            BOOST_ASSERT(output_edge.data.distance > 0);

            output_edge.data.roundabout = input_edge.roundabout;
            output_edge.data.name_id = input_edge.name_id;
            output_edge.data.access_restricted = input_edge.access_restricted;
            output_edge.data.travel_mode = input_edge.travel_mode;
        }
    );

    tbb::parallel_sort(edges_list.begin(), edges_list.end());

    auto graph = std::make_shared<NodeBasedDynamicGraph>(
        static_cast<NodeBasedDynamicGraph::NodeIterator>(number_of_nodes), edges_list);

    return graph;
}

#endif // NODE_BASED_GRAPH_HPP
