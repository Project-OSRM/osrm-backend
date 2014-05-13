/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
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

#ifndef STATIC_GRAPH_H
#define STATIC_GRAPH_H

#include "../DataStructures/Percent.h"
#include "../DataStructures/SharedMemoryVectorWrapper.h"
#include "../Util/SimpleLogger.h"
#include "../typedefs.h"

#include <boost/range/irange.hpp>

#include <algorithm>
#include <limits>
#include <vector>

template <typename EdgeDataT, bool UseSharedMemory = false> class StaticGraph
{
  public:
    typedef decltype(boost::irange(0u,0u)) EdgeRange;
    typedef NodeID NodeIterator;
    typedef NodeID EdgeIterator;
    typedef EdgeDataT EdgeData;
    class InputEdge
    {
      public:
        EdgeDataT data;
        NodeIterator source;
        NodeIterator target;
        bool operator<(const InputEdge &right) const
        {
            if (source != right.source)
            {
                return source < right.source;
            }
            return target < right.target;
        }
    };

    struct NodeArrayEntry
    {
        // index of the first edge
        EdgeIterator first_edge;
    };

    struct EdgeArrayEntry
    {
        NodeID target;
        EdgeDataT data;
    };

    EdgeRange GetAdjacentEdgeRange(const NodeID node) const
    {
        return boost::irange(BeginEdges(node), EndEdges(node));
    }

    StaticGraph(const int nodes, std::vector<InputEdge> &graph)
    {
        std::sort(graph.begin(), graph.end());
        number_of_nodes = nodes;
        number_of_edges = (EdgeIterator)graph.size();
        node_array.resize(number_of_nodes + 1);
        EdgeIterator edge = 0;
        EdgeIterator position = 0;
        for (NodeIterator node = 0; node <= number_of_nodes; ++node)
        {
            EdgeIterator last_edge = edge;
            while (edge < number_of_edges && graph[edge].source == node)
            {
                ++edge;
            }
            node_array[node].first_edge = position; //=edge
            position += edge - last_edge;           // remove
        }
        edge_array.resize(position); //(edge)
        edge = 0;
        for (NodeIterator node = 0; node < number_of_nodes; ++node)
        {
            EdgeIterator e = node_array[node + 1].first_edge;
            for (EdgeIterator i = node_array[node].first_edge; i != e; ++i)
            {
                edge_array[i].target = graph[edge].target;
                edge_array[i].data = graph[edge].data;
                assert(edge_array[i].data.distance > 0);
                edge++;
            }
        }
    }

    StaticGraph(typename ShM<NodeArrayEntry, UseSharedMemory>::vector &nodes,
                typename ShM<EdgeArrayEntry, UseSharedMemory>::vector &edges)
    {
        number_of_nodes = nodes.size() - 1;
        number_of_edges = edges.size();

        node_array.swap(nodes);
        edge_array.swap(edges);

#ifndef NDEBUG
        Percent p(GetNumberOfNodes());
        for (unsigned u = 0; u < GetNumberOfNodes(); ++u)
        {
            for (auto eid : GetAdjacentEdgeRange(u))
            {
                const unsigned v = GetTarget(eid);
                const EdgeData &data = GetEdgeData(eid);
                if (data.shortcut)
                {
                    const EdgeID first_edge_id = FindEdgeInEitherDirection(u, data.id);
                    if (SPECIAL_EDGEID == first_edge_id)
                    {
                        SimpleLogger().Write(logWARNING) << "cannot find first segment of edge ("
                                                         << u << "," << data.id << "," << v
                                                         << "), eid: " << eid;
                        BOOST_ASSERT(false);
                    }
                    const EdgeID second_edge_id = FindEdgeInEitherDirection(data.id, v);
                    if (SPECIAL_EDGEID == second_edge_id)
                    {
                        SimpleLogger().Write(logWARNING) << "cannot find second segment of edge ("
                                                         << u << "," << data.id << "," << v
                                                         << "), eid: " << eid;
                        BOOST_ASSERT(false);
                    }
                }
            }
            p.printIncrement();
        }
#endif
    }

    unsigned GetNumberOfNodes() const { return number_of_nodes -1; }

    unsigned GetNumberOfEdges() const { return number_of_edges; }

    unsigned GetOutDegree(const NodeIterator n) const { return BeginEdges(n) - EndEdges(n) - 1; }

    inline NodeIterator GetTarget(const EdgeIterator e) const
    {
        return NodeIterator(edge_array[e].target);
    }

    inline EdgeDataT &GetEdgeData(const EdgeIterator e) { return edge_array[e].data; }

    const EdgeDataT &GetEdgeData(const EdgeIterator e) const { return edge_array[e].data; }

    EdgeIterator BeginEdges(const NodeIterator n) const
    {
        return EdgeIterator(node_array.at(n).first_edge);
    }

    EdgeIterator EndEdges(const NodeIterator n) const
    {
        return EdgeIterator(node_array.at(n + 1).first_edge);
    }

    // searches for a specific edge
    EdgeIterator FindEdge(const NodeIterator from, const NodeIterator to) const
    {
        EdgeIterator smallest_edge = SPECIAL_EDGEID;
        EdgeWeight smallest_weight = INVALID_EDGE_WEIGHT;
        for (auto edge : GetAdjacentEdgeRange(from))
        {
            const NodeID target = GetTarget(edge);
            const EdgeWeight weight = GetEdgeData(edge).distance;
            if (target == to && weight < smallest_weight)
            {
                smallest_edge = edge;
                smallest_weight = weight;
            }
        }
        return smallest_edge;
    }

    EdgeIterator FindEdgeInEitherDirection(const NodeIterator from, const NodeIterator to) const
    {
        EdgeIterator tmp = FindEdge(from, to);
        return (SPECIAL_NODEID != tmp ? tmp : FindEdge(to, from));
    }

    EdgeIterator
    FindEdgeIndicateIfReverse(const NodeIterator from, const NodeIterator to, bool &result) const
    {
        EdgeIterator current_iterator = FindEdge(from, to);
        if (SPECIAL_NODEID == current_iterator)
        {
            current_iterator = FindEdge(to, from);
            if (SPECIAL_NODEID != current_iterator)
            {
                result = true;
            }
        }
        return current_iterator;
    }

  private:
    NodeIterator number_of_nodes;
    EdgeIterator number_of_edges;

    typename ShM<NodeArrayEntry, UseSharedMemory>::vector node_array;
    typename ShM<EdgeArrayEntry, UseSharedMemory>::vector edge_array;
};

#endif // STATIC_GRAPH_H
