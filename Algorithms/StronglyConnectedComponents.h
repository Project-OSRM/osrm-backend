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

#ifndef STRONGLYCONNECTEDCOMPONENTS_H_
#define STRONGLYCONNECTEDCOMPONENTS_H_

#include "../typedefs.h"
#include "../DataStructures/DeallocatingVector.h"
#include "../DataStructures/DynamicGraph.h"
#include "../DataStructures/ImportEdge.h"
#include "../DataStructures/QueryNode.h"
#include "../DataStructures/Percent.h"
#include "../DataStructures/Restriction.h"
#include "../DataStructures/TurnInstructions.h"

#include "../Util/SimpleLogger.h"
#include "../Util/StdHashExtensions.h"

#include <osrm/Coordinate.h>

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>

#ifdef __APPLE__
#include <gdal.h>
#include <ogrsf_frmts.h>
#else
#include <gdal/gdal.h>
#include <gdal/ogrsf_frmts.h>
#endif

#include <cstdint>

#include <memory>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class TarjanSCC
{
  private:
    struct TarjanNode
    {
        TarjanNode() : index(UINT_MAX), low_link(UINT_MAX), on_stack(false) {}
        unsigned index;
        unsigned low_link;
        bool on_stack;
    };

    struct TarjanEdgeData
    {
        int distance;
        unsigned name_id : 31;
        bool shortcut : 1;
        short type;
        bool forward : 1;
        bool backward : 1;
        bool reversedEdge : 1;
    };

    struct TarjanStackFrame
    {
        explicit TarjanStackFrame(NodeID v, NodeID parent) : v(v), parent(parent) {}
        NodeID v;
        NodeID parent;
    };

    typedef DynamicGraph<TarjanEdgeData> TarjanDynamicGraph;
    typedef TarjanDynamicGraph::InputEdge TarjanEdge;
    typedef std::pair<NodeID, NodeID> RestrictionSource;
    typedef std::pair<NodeID, bool> restriction_target;
    typedef std::vector<restriction_target> EmanatingRestrictionsVector;
    typedef std::unordered_map<RestrictionSource, unsigned> RestrictionMap;

    std::vector<NodeInfo> m_coordinate_list;
    std::vector<EmanatingRestrictionsVector> m_restriction_bucket_list;
    std::shared_ptr<TarjanDynamicGraph> m_node_based_graph;
    std::unordered_set<NodeID> m_barrier_node_list;
    std::unordered_set<NodeID> m_traffic_light_list;
    unsigned m_restriction_counter;
    RestrictionMap m_restriction_map;

  public:
    TarjanSCC(int number_of_nodes,
              std::vector<NodeBasedEdge> &input_edges,
              std::vector<NodeID> &bn,
              std::vector<NodeID> &tl,
              std::vector<TurnRestriction> &irs,
              std::vector<NodeInfo> &nI)
        : m_coordinate_list(nI), m_restriction_counter(irs.size())
    {
        for (const TurnRestriction &restriction : irs)
        {
            std::pair<NodeID, NodeID> restrictionSource = {restriction.fromNode,
                                                           restriction.viaNode};
            unsigned index;
            RestrictionMap::iterator restriction_iterator =
                m_restriction_map.find(restrictionSource);
            if (restriction_iterator == m_restriction_map.end())
            {
                index = m_restriction_bucket_list.size();
                m_restriction_bucket_list.resize(index + 1);
                m_restriction_map.emplace(restrictionSource, index);
            }
            else
            {
                index = restriction_iterator->second;
                // Map already contains an is_only_*-restriction
                if (m_restriction_bucket_list.at(index).begin()->second)
                {
                    continue;
                }
                else if (restriction.flags.isOnly)
                {
                    // We are going to insert an is_only_*-restriction. There can be only one.
                    m_restriction_bucket_list.at(index).clear();
                }
            }

            m_restriction_bucket_list.at(index)
                .emplace_back(restriction.toNode, restriction.flags.isOnly);
        }

        m_barrier_node_list.insert(bn.begin(), bn.end());
        m_traffic_light_list.insert(tl.begin(), tl.end());

        DeallocatingVector<TarjanEdge> edge_list;
        for (const NodeBasedEdge &input_edge : input_edges)
        {
            if (input_edge.source() == input_edge.target())
            {
                continue;
            }

            TarjanEdge edge;
            if (input_edge.isForward())
            {
                edge.source = input_edge.source();
                edge.target = input_edge.target();
                edge.data.forward = input_edge.isForward();
                edge.data.backward = input_edge.isBackward();
            }
            else
            {
                edge.source = input_edge.target();
                edge.target = input_edge.source();
                edge.data.backward = input_edge.isForward();
                edge.data.forward = input_edge.isBackward();
            }

            edge.data.distance = (std::max)((int)input_edge.weight(), 1);
            BOOST_ASSERT(edge.data.distance > 0);
            edge.data.shortcut = false;
            // edge.data.roundabout = input_edge.isRoundabout();
            // edge.data.ignoreInGrid = input_edge.ignoreInGrid();
            edge.data.name_id = input_edge.name();
            edge.data.type = input_edge.type();
            // edge.data.isAccessRestricted = input_edge.isAccessRestricted();
            edge.data.reversedEdge = false;
            edge_list.push_back(edge);
            if (edge.data.backward)
            {
                std::swap(edge.source, edge.target);
                edge.data.forward = input_edge.isBackward();
                edge.data.backward = input_edge.isForward();
                edge.data.reversedEdge = true;
                edge_list.push_back(edge);
            }
        }
        std::vector<NodeBasedEdge>().swap(input_edges);
        BOOST_ASSERT_MSG(0 == input_edges.size() && 0 == input_edges.capacity(),
                         "input edge vector not properly deallocated");

        std::sort(edge_list.begin(), edge_list.end());

        m_node_based_graph = std::make_shared<TarjanDynamicGraph>(number_of_nodes, edge_list);
    }

    ~TarjanSCC() { m_node_based_graph.reset(); }

    void Run()
    {
        // remove files from previous run if exist
        DeleteFileIfExists("component.dbf");
        DeleteFileIfExists("component.shx");
        DeleteFileIfExists("component.shp");

        Percent p(m_node_based_graph->GetNumberOfNodes());

        OGRRegisterAll();

        const char *pszDriverName = "ESRI Shapefile";
        OGRSFDriver *poDriver =
            OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(pszDriverName);
        if (NULL == poDriver)
        {
            throw OSRMException("ESRI Shapefile driver not available");
        }
        OGRDataSource *poDS = poDriver->CreateDataSource("component.shp", NULL);

        if (NULL == poDS)
        {
            throw OSRMException("Creation of output file failed");
        }

        OGRLayer *poLayer = poDS->CreateLayer("component", NULL, wkbLineString, NULL);

        if (NULL == poLayer)
        {
            throw OSRMException("Layer creation failed.");
        }

        // The following is a hack to distinguish between stuff that happens
        // before the recursive call and stuff that happens after
        std::stack<std::pair<bool, TarjanStackFrame>> recursion_stack;
        // true = stuff before, false = stuff after call
        std::stack<NodeID> tarjan_stack;
        std::vector<unsigned> components_index(m_node_based_graph->GetNumberOfNodes(), UINT_MAX);
        std::vector<NodeID> component_size_vector;
        std::vector<TarjanNode> tarjan_node_list(m_node_based_graph->GetNumberOfNodes());
        unsigned component_index = 0, size_of_current_component = 0;
        int index = 0;
        NodeID last_node = m_node_based_graph->GetNumberOfNodes();
        for (NodeID node = 0; node < last_node; ++node)
        {
            if (UINT_MAX == components_index[node])
            {
                recursion_stack.emplace(true, TarjanStackFrame(node, node));
            }

            while (!recursion_stack.empty())
            {
                const bool before_recursion = recursion_stack.top().first;
                TarjanStackFrame currentFrame = recursion_stack.top().second;
                NodeID v = currentFrame.v;
                recursion_stack.pop();

                if (before_recursion)
                {
                    // Mark frame to handle tail of recursion
                    recursion_stack.emplace(false, currentFrame);

                    // Mark essential information for SCC
                    tarjan_node_list[v].index = index;
                    tarjan_node_list[v].low_link = index;
                    tarjan_stack.push(v);
                    tarjan_node_list[v].on_stack = true;
                    ++index;

                    // Traverse outgoing edges
                    for (auto e2 : m_node_based_graph->GetAdjacentEdgeRange(v))
                    {
                        const TarjanDynamicGraph::NodeIterator vprime =
                            m_node_based_graph->GetTarget(e2);
                        if (UINT_MAX == tarjan_node_list[vprime].index)
                        {
                            recursion_stack.emplace(true, TarjanStackFrame(vprime, v));
                        }
                        else
                        {
                            if (tarjan_node_list[vprime].on_stack &&
                                tarjan_node_list[vprime].index < tarjan_node_list[v].low_link)
                            {
                                tarjan_node_list[v].low_link = tarjan_node_list[vprime].index;
                            }
                        }
                    }
                }
                else
                {
                    tarjan_node_list[currentFrame.parent].low_link =
                        std::min(tarjan_node_list[currentFrame.parent].low_link,
                                 tarjan_node_list[v].low_link);
                    // after recursion, lets do cycle checking
                    // Check if we found a cycle. This is the bottom part of the recursion
                    if (tarjan_node_list[v].low_link == tarjan_node_list[v].index)
                    {
                        NodeID vprime;
                        do
                        {
                            vprime = tarjan_stack.top();
                            tarjan_stack.pop();
                            tarjan_node_list[vprime].on_stack = false;
                            components_index[vprime] = component_index;
                            ++size_of_current_component;
                        } while (v != vprime);

                        component_size_vector.emplace_back(size_of_current_component);

                        if (size_of_current_component > 1000)
                        {
                            SimpleLogger().Write() << "large component [" << component_index
                                                   << "]=" << size_of_current_component;
                        }

                        ++component_index;
                        size_of_current_component = 0;
                    }
                }
            }
        }

        SimpleLogger().Write() << "identified: " << component_size_vector.size()
                               << " many components, marking small components";

        // TODO/C++11: prime candidate for lambda function
        // unsigned size_one_counter = 0;
        // for (unsigned i = 0, end = component_size_vector.size(); i < end; ++i)
        // {
        //     if (1 == component_size_vector[i])
        //     {
        //         ++size_one_counter;
        //     }
        // }
        unsigned size_one_counter = std::count_if(component_size_vector.begin(),
                                                  component_size_vector.end(),
                                                  [] (unsigned value) { return 1 == value;});

        SimpleLogger().Write() << "identified " << size_one_counter << " SCCs of size 1";

        uint64_t total_network_distance = 0;
        p.reinit(m_node_based_graph->GetNumberOfNodes());
        NodeID last_u_node = m_node_based_graph->GetNumberOfNodes();
        for (NodeID u = 0; u < last_u_node; ++u)
        {
            p.printIncrement();
            for (auto e1 : m_node_based_graph->GetAdjacentEdgeRange(u))
            {
                if (!m_node_based_graph->GetEdgeData(e1).reversedEdge)
                {
                    continue;
                }
                const TarjanDynamicGraph::NodeIterator v = m_node_based_graph->GetTarget(e1);

                total_network_distance +=
                    100 * FixedPointCoordinate::ApproximateDistance(m_coordinate_list[u].lat,
                                                                    m_coordinate_list[u].lon,
                                                                    m_coordinate_list[v].lat,
                                                                    m_coordinate_list[v].lon);

                if (SHRT_MAX != m_node_based_graph->GetEdgeData(e1).type)
                {
                    BOOST_ASSERT(e1 != UINT_MAX);
                    BOOST_ASSERT(u != UINT_MAX);
                    BOOST_ASSERT(v != UINT_MAX);

                    const unsigned size_of_containing_component =
                        std::min(component_size_vector[components_index[u]],
                                 component_size_vector[components_index[v]]);

                    // edges that end on bollard nodes may actually be in two distinct components
                    if (size_of_containing_component < 10)
                    {
                        OGRLineString lineString;
                        lineString.addPoint(m_coordinate_list[u].lon / COORDINATE_PRECISION,
                                            m_coordinate_list[u].lat / COORDINATE_PRECISION);
                        lineString.addPoint(m_coordinate_list[v].lon / COORDINATE_PRECISION,
                                            m_coordinate_list[v].lat / COORDINATE_PRECISION);

                        OGRFeature *poFeature = OGRFeature::CreateFeature(poLayer->GetLayerDefn());

                        poFeature->SetGeometry(&lineString);
                        if (OGRERR_NONE != poLayer->CreateFeature(poFeature))
                        {
                            throw OSRMException("Failed to create feature in shapefile.");
                        }
                        OGRFeature::DestroyFeature(poFeature);
                    }
                }
            }
        }
        OGRDataSource::DestroyDataSource(poDS);
        std::vector<NodeID>().swap(component_size_vector);
        BOOST_ASSERT_MSG(0 == component_size_vector.size() && 0 == component_size_vector.capacity(),
                         "component_size_vector not properly deallocated");

        std::vector<NodeID>().swap(components_index);
        BOOST_ASSERT_MSG(0 == components_index.size() && 0 == components_index.capacity(),
                         "icomponents_index not properly deallocated");

        SimpleLogger().Write() << "total network distance: " << (uint64_t)total_network_distance /
                                                                    100 / 1000. << " km";
    }

  private:
    unsigned CheckForEmanatingIsOnlyTurn(const NodeID u, const NodeID v) const
    {
        std::pair<NodeID, NodeID> restriction_source = {u, v};
        RestrictionMap::const_iterator restriction_iterator =
            m_restriction_map.find(restriction_source);
        if (restriction_iterator != m_restriction_map.end())
        {
            const unsigned index = restriction_iterator->second;
            for (const RestrictionSource &restriction_target : m_restriction_bucket_list.at(index))
            {
                if (restriction_target.second)
                {
                    return restriction_target.first;
                }
            }
        }
        return UINT_MAX;
    }

    bool CheckIfTurnIsRestricted(const NodeID u, const NodeID v, const NodeID w) const
    {
        // only add an edge if turn is not a U-turn except it is the end of dead-end street.
        std::pair<NodeID, NodeID> restriction_source = {u, v};
        RestrictionMap::const_iterator restriction_iterator =
            m_restriction_map.find(restriction_source);
        if (restriction_iterator != m_restriction_map.end())
        {
            const unsigned index = restriction_iterator->second;
            for (const restriction_target &restriction_target : m_restriction_bucket_list.at(index))
            {
                if (w == restriction_target.first)
                {
                    return true;
                }
            }
        }
        return false;
    }

    void DeleteFileIfExists(const std::string &file_name) const
    {
        if (boost::filesystem::exists(file_name))
        {
            boost::filesystem::remove(file_name);
        }
    }
};

#endif /* STRONGLYCONNECTEDCOMPONENTS_H_ */
