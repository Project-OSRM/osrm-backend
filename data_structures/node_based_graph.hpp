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

inline bool validateNeighborHood(const NodeBasedDynamicGraph& graph, const NodeID source)
{
    for (auto edge = graph.BeginEdges(source); edge < graph.EndEdges(source); ++edge)
    {
        const auto& data = graph.GetEdgeData(edge);
        if (!data.forward && !data.backward)
        {
            SimpleLogger().Write(logWARNING) << "Invalid edge directions";
            return false;
        }

        auto target = graph.GetTarget(edge);
        if (target == SPECIAL_NODEID)
        {
            SimpleLogger().Write(logWARNING) << "Invalid edge target";
            return false;
        }

        bool found_reverse = false;
        for (auto rev_edge = graph.BeginEdges(target); rev_edge < graph.EndEdges(target); ++rev_edge)
        {
            auto rev_target = graph.GetTarget(rev_edge);
            if (rev_target == SPECIAL_NODEID)
            {
                SimpleLogger().Write(logWARNING) << "Invalid reverse edge target";
                return false;
            }

            if (rev_target != source)
            {
                continue;
            }

            if (found_reverse)
            {
                SimpleLogger().Write(logWARNING) << "Found more than one reverse edge";
                return false;
            }

            const auto& rev_data = graph.GetEdgeData(rev_edge);

            // edge is incoming, this must be an outgoing edge
            if (data.backward && !rev_data.forward)
            {
                SimpleLogger().Write(logWARNING) << "Found no outgoing edge to an incoming edge!";
                return false;
            }

            // edge is bi-directional, reverse must be as well
            if (data.forward && data.backward && (!rev_data.forward || !rev_data.backward))
            {
                SimpleLogger().Write(logWARNING) << "Found bi-directional edge that is not bi-directional to both ends";
                return false;
            }

            found_reverse = true;

        }

        if (!found_reverse)
        {
            SimpleLogger().Write(logWARNING) << "Could not find reverse edge";
            return false;
        }
    }

    return true;
}

// This function checks if the overal graph is undirected (has an edge in each direction).
inline bool validateNodeBasedGraph(const NodeBasedDynamicGraph& graph)
{
    for (auto source = 0u; source < graph.GetNumberOfNodes(); ++source)
    {
        if (!validateNeighborHood(graph, source))
        {
            return false;
        }
    }

    return true;
}

// Factory method to create NodeBasedDynamicGraph from NodeBasedEdges
// The since DynamicGraph expects directed edges, we need to insert
// two edges for undirected edges.
inline std::shared_ptr<NodeBasedDynamicGraph>
NodeBasedDynamicGraphFromImportEdges(int number_of_nodes, std::vector<NodeBasedEdge> &input_edge_list)
{
    static_assert(sizeof(NodeBasedEdgeData) == 16,
                  "changing node based edge data size changes memory consumption");

    DeallocatingVector<NodeBasedDynamicGraph::InputEdge> edges_list;
    NodeBasedDynamicGraph::InputEdge edge;

    // Since DynamicGraph assumes directed edges we have to make sure we transformed
    // the compressed edge format into single directed edges. We do this to make sure
    // every node also knows its incoming edges, not only its outgoing edges and use the backward=true
    // flag to indicate which is which.
    //
    // We do the transformation in the following way:
    //
    // if the edge (a, b) is split:
    //   1. this edge must be in only one direction, so its a --> b
    //   2. there must be another directed edge b --> a somewhere in the data
    // if the edge (a, b) is not split:
    //   1. this edge be on of a --> b od a <-> b
    //      (a <-- b gets reducted to b --> a)
    //   2. a --> b will be transformed to a --> b and b <-- a
    //   3. a <-> b will be transformed to a <-> b and b <-> a (I think a --> b and b <-- a would work as well though)
    for (const NodeBasedEdge &import_edge : input_edge_list)
    {
        // edges that are not forward get converted by flipping the end points
        BOOST_ASSERT(import_edge.forward);

        if (import_edge.forward)
        {
            edge.source = import_edge.source;
            edge.target = import_edge.target;
            edge.data.forward = import_edge.forward;
            edge.data.backward = import_edge.backward;
        }

        BOOST_ASSERT(edge.source != edge.target);

        edge.data.distance = static_cast<int>(import_edge.weight);
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

    tbb::parallel_sort(edges_list.begin(), edges_list.end());

    auto graph = std::make_shared<NodeBasedDynamicGraph>(
        static_cast<NodeBasedDynamicGraph::NodeIterator>(number_of_nodes), edges_list);


#ifndef NDEBUG
    BOOST_ASSERT(validateNodeBasedGraph(*graph));
#endif

    return graph;
}

#endif // NODE_BASED_GRAPH_HPP
