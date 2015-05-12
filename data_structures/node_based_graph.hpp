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
#include "../util/simple_logger.hpp"

#include <tbb/parallel_sort.h>

#include <memory>

struct NodeBasedEdgeData
{
    NodeBasedEdgeData()
        : distance(INVALID_EDGE_WEIGHT), edgeBasedNodeID(SPECIAL_NODEID),
          nameID(std::numeric_limits<unsigned>::max()), isAccessRestricted(false), shortcut(false),
          forward(false), backward(false), roundabout(false), ignore_in_grid(false),
          travel_mode(TRAVEL_MODE_INACCESSIBLE)
    {
    }

    int distance;
    unsigned edgeBasedNodeID;
    unsigned nameID;
    bool isAccessRestricted : 1;
    bool shortcut : 1;
    bool forward : 1;
    bool backward : 1;
    bool roundabout : 1;
    bool ignore_in_grid : 1;
    TravelMode travel_mode : 4;

    void SwapDirectionFlags()
    {
        bool temp_flag = forward;
        forward = backward;
        backward = temp_flag;
    }

    bool IsCompatibleTo(const NodeBasedEdgeData &other) const
    {
        return (forward == other.forward) && (backward == other.backward) &&
               (nameID == other.nameID) && (ignore_in_grid == other.ignore_in_grid) &&
               (travel_mode == other.travel_mode);
    }
};

using NodeBasedDynamicGraph = DynamicGraph<NodeBasedEdgeData>;

// Factory method to create NodeBasedDynamicGraph from NodeBasedEdges
inline std::shared_ptr<NodeBasedDynamicGraph>
NodeBasedDynamicGraphFromImportEdges(int number_of_nodes, std::vector<NodeBasedEdge> &input_edge_list)
{
    static_assert(sizeof(NodeBasedEdgeData) == 16,
                  "changing node based edge data size changes memory consumption");

    DeallocatingVector<NodeBasedDynamicGraph::InputEdge> edges_list;
    NodeBasedDynamicGraph::InputEdge edge;
    for (const NodeBasedEdge &import_edge : input_edge_list)
    {
        BOOST_ASSERT(import_edge.forward || import_edge.backward);
        if (import_edge.forward)
        {
            edge.source = import_edge.source;
            edge.target = import_edge.target;
            edge.data.forward = import_edge.forward;
            edge.data.backward = import_edge.backward;
        }
        else
        {
            edge.source = import_edge.target;
            edge.target = import_edge.source;
            edge.data.backward = import_edge.forward;
            edge.data.forward = import_edge.backward;
        }

        if (edge.source == edge.target)
        {
            continue;
        }

        edge.data.distance = (std::max)(static_cast<int>(import_edge.weight), 1);
        BOOST_ASSERT(edge.data.distance > 0);
        edge.data.shortcut = false;
        edge.data.roundabout = import_edge.roundabout;
        edge.data.ignore_in_grid = import_edge.in_tiny_cc;
        edge.data.nameID = import_edge.name_id;
        edge.data.isAccessRestricted = import_edge.access_restricted;
        edge.data.travel_mode = import_edge.travel_mode;

        edges_list.push_back(edge);

        if (!import_edge.is_split)
        {
            using std::swap; // enable ADL
            swap(edge.source, edge.target);
            edge.data.SwapDirectionFlags();
            edges_list.push_back(edge);
        }
    }

    // sort edges by source node id
    tbb::parallel_sort(edges_list.begin(), edges_list.end());

    // this code removes multi-edges
    // my merging mutli-edges bi-directional edges can become directional again!
    // Consider the following example:
    // a --5-- b
    //  `--1--^
    // After merging we need to split {a, b, 5} into (a, b, 1) and (b, a, 5)
    NodeID edge_count = 0;
    for (NodeID i = 0; i < edges_list.size();)
    {
        const NodeID source = edges_list[i].source;
        const NodeID target = edges_list[i].target;
        // remove eigenloops
        if (source == target)
        {
            i++;
            continue;
        }
        NodeBasedDynamicGraph::InputEdge forward_edge;
        NodeBasedDynamicGraph::InputEdge reverse_edge;
        forward_edge = reverse_edge = edges_list[i];
        forward_edge.data.forward = reverse_edge.data.backward = true;
        forward_edge.data.backward = reverse_edge.data.forward = false;
        forward_edge.data.shortcut = reverse_edge.data.shortcut = false;
        forward_edge.data.distance = reverse_edge.data.distance = std::numeric_limits<int>::max();
        // remove parallel edges and set current distance values
        while (i < edges_list.size() && edges_list[i].source == source &&
               edges_list[i].target == target)
        {
            if (edges_list[i].data.forward)
            {
                forward_edge.data.distance =
                    std::min(edges_list[i].data.distance, forward_edge.data.distance);
            }
            if (edges_list[i].data.backward)
            {
                reverse_edge.data.distance =
                    std::min(edges_list[i].data.distance, reverse_edge.data.distance);
            }
            ++i;
        }
        // merge edges (s,t) and (t,s) into bidirectional edge
        if (forward_edge.data.distance == reverse_edge.data.distance)
        {
            if (static_cast<int>(forward_edge.data.distance) != std::numeric_limits<int>::max())
            {
                forward_edge.data.backward = true;
                BOOST_ASSERT(edge_count < i);
                edges_list[edge_count++] = forward_edge;
            }
        }
        else
        { // insert seperate edges
          // this case can only happen if we merged a bi-directional edge with a directional
          // edge above, this incrementing i and making it safe to overwrite the next element
          // as well
            if (static_cast<int>(forward_edge.data.distance) != std::numeric_limits<int>::max())
            {
                BOOST_ASSERT(edge_count < i);
                edges_list[edge_count++] = forward_edge;
            }
            if (static_cast<int>(reverse_edge.data.distance) != std::numeric_limits<int>::max())
            {
                BOOST_ASSERT(edge_count < i);
                edges_list[edge_count++] = reverse_edge;
            }
        }
    }
    edges_list.resize(edge_count);
    SimpleLogger().Write() << "merged " << edges_list.size() - edge_count << " edges out of "
                           << edges_list.size();

    return std::make_shared<NodeBasedDynamicGraph>(
        static_cast<NodeBasedDynamicGraph::NodeIterator>(number_of_nodes), edges_list);
}

#endif // NODE_BASED_GRAPH_HPP
