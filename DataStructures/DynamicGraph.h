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

#ifndef DYNAMICGRAPH_H
#define DYNAMICGRAPH_H

#include "../DataStructures/DeallocatingVector.h"

#include <boost/assert.hpp>
#include <boost/range/irange.hpp>

#include <cstdint>

#include <algorithm>
#include <limits>
#include <vector>

template <typename EdgeDataT> class DynamicGraph
{
  public:
    typedef decltype(boost::irange(0u,0u)) EdgeRange;
    typedef EdgeDataT EdgeData;
    typedef unsigned NodeIterator;
    typedef unsigned EdgeIterator;

    class InputEdge
    {
      public:
        NodeIterator source;
        NodeIterator target;
        EdgeDataT data;
        bool operator<(const InputEdge &right) const
        {
            if (source != right.source)
                return source < right.source;
            return target < right.target;
        }
    };

    // Constructs an empty graph with a given number of nodes.
    explicit DynamicGraph(int32_t nodes) : m_numNodes(nodes), m_numEdges(0)
    {
        m_nodes.reserve(m_numNodes);
        m_nodes.resize(m_numNodes);

        m_edges.reserve(m_numNodes * 1.1);
        m_edges.resize(m_numNodes);
    }

    template <class ContainerT> DynamicGraph(const int32_t nodes, const ContainerT &graph)
    {
        m_numNodes = nodes;
        m_numEdges = (EdgeIterator)graph.size();
        m_nodes.reserve(m_numNodes + 1);
        m_nodes.resize(m_numNodes + 1);
        EdgeIterator edge = 0;
        EdgeIterator position = 0;
        for (NodeIterator node = 0; node < m_numNodes; ++node)
        {
            EdgeIterator lastEdge = edge;
            while (edge < m_numEdges && graph[edge].source == node)
            {
                ++edge;
            }
            m_nodes[node].firstEdge = position;
            m_nodes[node].edges = edge - lastEdge;
            position += m_nodes[node].edges;
        }
        m_nodes.back().firstEdge = position;
        m_edges.reserve(position * 1.1);
        m_edges.resize(position);
        edge = 0;
        for (NodeIterator node = 0; node < m_numNodes; ++node)
        {
            for (EdgeIterator i = m_nodes[node].firstEdge,
                              e = m_nodes[node].firstEdge + m_nodes[node].edges;
                 i != e;
                 ++i)
            {
                m_edges[i].target = graph[edge].target;
                m_edges[i].data = graph[edge].data;
                BOOST_ASSERT_MSG(graph[edge].data.distance > 0, "edge distance invalid");
                ++edge;
            }
        }
    }

    ~DynamicGraph() {}

    unsigned GetNumberOfNodes() const { return m_numNodes; }

    unsigned GetNumberOfEdges() const { return m_numEdges; }

    unsigned GetOutDegree(const NodeIterator n) const { return m_nodes[n].edges; }

    unsigned GetDirectedOutDegree(const NodeIterator n) const
    {
        unsigned degree = 0;
        for(EdgeIterator edge = BeginEdges(n); edge < EndEdges(n); ++edge)
        {
            if (GetEdgeData(edge).forward)
            {
                ++degree;
            }
        }
        return degree;
    }

    NodeIterator GetTarget(const EdgeIterator e) const { return NodeIterator(m_edges[e].target); }

    void SetTarget(const EdgeIterator e, const NodeIterator n) { m_edges[e].target = n; }

    EdgeDataT &GetEdgeData(const EdgeIterator e) { return m_edges[e].data; }

    const EdgeDataT &GetEdgeData(const EdgeIterator e) const { return m_edges[e].data; }

    EdgeIterator BeginEdges(const NodeIterator n) const
    {
        return EdgeIterator(m_nodes[n].firstEdge);
    }

    EdgeIterator EndEdges(const NodeIterator n) const
    {
        return EdgeIterator(m_nodes[n].firstEdge + m_nodes[n].edges);
    }

    EdgeRange GetAdjacentEdgeRange(const NodeIterator node) const
    {
        return boost::irange(BeginEdges(node), EndEdges(node));
    }

    // adds an edge. Invalidates edge iterators for the source node
    EdgeIterator InsertEdge(const NodeIterator from, const NodeIterator to, const EdgeDataT &data)
    {
        Node &node = m_nodes[from];
        EdgeIterator newFirstEdge = node.edges + node.firstEdge;
        if (newFirstEdge >= m_edges.size() || !isDummy(newFirstEdge))
        {
            if (node.firstEdge != 0 && isDummy(node.firstEdge - 1))
            {
                node.firstEdge--;
                m_edges[node.firstEdge] = m_edges[node.firstEdge + node.edges];
            }
            else
            {
                EdgeIterator newFirstEdge = (EdgeIterator)m_edges.size();
                unsigned newSize = node.edges * 1.1 + 2;
                EdgeIterator requiredCapacity = newSize + m_edges.size();
                EdgeIterator oldCapacity = m_edges.capacity();
                if (requiredCapacity >= oldCapacity)
                {
                    m_edges.reserve(requiredCapacity * 1.1);
                }
                m_edges.resize(m_edges.size() + newSize);
                for (EdgeIterator i = 0; i < node.edges; ++i)
                {
                    m_edges[newFirstEdge + i] = m_edges[node.firstEdge + i];
                    makeDummy(node.firstEdge + i);
                }
                for (EdgeIterator i = node.edges + 1; i < newSize; ++i)
                    makeDummy(newFirstEdge + i);
                node.firstEdge = newFirstEdge;
            }
        }
        Edge &edge = m_edges[node.firstEdge + node.edges];
        edge.target = to;
        edge.data = data;
        ++m_numEdges;
        ++node.edges;
        return EdgeIterator(node.firstEdge + node.edges);
    }

    // removes an edge. Invalidates edge iterators for the source node
    void DeleteEdge(const NodeIterator source, const EdgeIterator e)
    {
        Node &node = m_nodes[source];
#pragma omp atomic
        --m_numEdges;
        --node.edges;
        BOOST_ASSERT(std::numeric_limits<unsigned>::max() != node.edges);
        const unsigned last = node.firstEdge + node.edges;
        BOOST_ASSERT(std::numeric_limits<unsigned>::max() != last);
        // swap with last edge
        m_edges[e] = m_edges[last];
        makeDummy(last);
    }

    // removes all edges (source,target)
    int32_t DeleteEdgesTo(const NodeIterator source, const NodeIterator target)
    {
        int32_t deleted = 0;
        for (EdgeIterator i = BeginEdges(source), iend = EndEdges(source); i < iend - deleted; ++i)
        {
            if (m_edges[i].target == target)
            {
                do
                {
                    deleted++;
                    m_edges[i] = m_edges[iend - deleted];
                    makeDummy(iend - deleted);
                } while (i < iend - deleted && m_edges[i].target == target);
            }
        }

#pragma omp atomic
        m_numEdges -= deleted;
        m_nodes[source].edges -= deleted;

        return deleted;
    }

    // searches for a specific edge
    EdgeIterator FindEdge(const NodeIterator from, const NodeIterator to) const
    {
        for (EdgeIterator i = BeginEdges(from), iend = EndEdges(from); i != iend; ++i)
        {
            if (to == m_edges[i].target)
            {
                return i;
            }
        }
        return EndEdges(from);
    }

  protected:
    bool isDummy(const EdgeIterator edge) const
    {
        return m_edges[edge].target == (std::numeric_limits<NodeIterator>::max)();
    }

    void makeDummy(const EdgeIterator edge)
    {
        m_edges[edge].target = (std::numeric_limits<NodeIterator>::max)();
    }

    struct Node
    {
        // index of the first edge
        EdgeIterator firstEdge;
        // amount of edges
        unsigned edges;
    };

    struct Edge
    {
        NodeIterator target;
        EdgeDataT data;
    };

    NodeIterator m_numNodes;
    EdgeIterator m_numEdges;

    std::vector<Node> m_nodes;
    DeallocatingVector<Edge> m_edges;
};

#endif // DYNAMICGRAPH_H
