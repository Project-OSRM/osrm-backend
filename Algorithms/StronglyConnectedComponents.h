/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.

Strongly connected components using Tarjan's Algorithm

 */

#ifndef STRONGLYCONNECTEDCOMPONENTS_H_
#define STRONGLYCONNECTEDCOMPONENTS_H_

#include "../DataStructures/Coordinate.h"
#include "../DataStructures/DeallocatingVector.h"
#include "../DataStructures/DynamicGraph.h"
#include "../DataStructures/ImportEdge.h"
#include "../DataStructures/QueryNode.h"
#include "../DataStructures/Percent.h"
#include "../DataStructures/Restriction.h"
#include "../DataStructures/TurnInstructions.h"

#include "../Util/SimpleLogger.h"

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/integer.hpp>
#include <boost/make_shared.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#ifdef __APPLE__
    #include <gdal.h>
    #include <ogrsf_frmts.h>
#else
    #include <gdal/gdal.h>
    #include <gdal/ogrsf_frmts.h>
#endif

#include <stack>
#include <vector>

class TarjanSCC {
private:

    struct TarjanNode {
        TarjanNode() : index(UINT_MAX), lowlink(UINT_MAX), onStack(false) {}
        unsigned index;
        unsigned lowlink;
        bool onStack;
    };

    struct TarjanEdgeData {
        int distance;
        unsigned nameID:31;
        bool shortcut:1;
        short type;
        bool isAccessRestricted:1;
        bool forward:1;
        bool backward:1;
        bool roundabout:1;
        bool ignoreInGrid:1;
        bool reversedEdge:1;
    };

    struct TarjanStackFrame {
        explicit TarjanStackFrame(
            NodeID v,
            NodeID parent
        ) : v(v), parent(parent) { }
        NodeID v;
        NodeID parent;
    };

    typedef DynamicGraph<TarjanEdgeData>        TarjanDynamicGraph;
    typedef TarjanDynamicGraph::InputEdge       TarjanEdge;
    typedef std::pair<NodeID, NodeID>           RestrictionSource;
    typedef std::pair<NodeID, bool>             restriction_target;
    typedef std::vector<restriction_target>      EmanatingRestrictionsVector;
    typedef boost::unordered_map<RestrictionSource, unsigned > RestrictionMap;

    std::vector<NodeInfo>                       m_coordinate_list;
    std::vector<EmanatingRestrictionsVector>    m_restriction_bucket_list;
    boost::shared_ptr<TarjanDynamicGraph>       m_node_based_graph;
    boost::unordered_set<NodeID>                m_barrier_node_list;
    boost::unordered_set<NodeID>                m_traffic_light_list;
    unsigned                                    m_restriction_counter;
    RestrictionMap                              m_restriction_map;

    struct EdgeBasedNode {
        bool operator<(const EdgeBasedNode & other) const {
            return other.id < id;
        }
        bool operator==(const EdgeBasedNode & other) const {
            return id == other.id;
        }
        NodeID id;
        int lat1;
        int lat2;
        int lon1;
        int lon2:31;
        bool belongsToTinyComponent:1;
        NodeID nameID;
        unsigned weight:31;
        bool ignoreInGrid:1;
    };

public:
    TarjanSCC(
        int number_of_nodes,
        std::vector<NodeBasedEdge> & input_edges,
        std::vector<NodeID> & bn,
        std::vector<NodeID> & tl,
        std::vector<TurnRestriction> & irs,
        std::vector<NodeInfo> & nI
    ) :
        m_coordinate_list(nI),
        m_restriction_counter(irs.size())
    {
        BOOST_FOREACH(const TurnRestriction & restriction, irs) {
            std::pair<NodeID, NodeID> restrictionSource = std::make_pair(
                restriction.fromNode, restriction.viaNode
            );
            unsigned index;
            RestrictionMap::iterator restriction_iterator = m_restriction_map.find(restrictionSource);
            if(restriction_iterator == m_restriction_map.end()) {
                index = m_restriction_bucket_list.size();
                m_restriction_bucket_list.resize(index+1);
                m_restriction_map[restrictionSource] = index;
            } else {
                index = restriction_iterator->second;
                //Map already contains an is_only_*-restriction
                if(m_restriction_bucket_list.at(index).begin()->second) {
                    continue;
                } else if(restriction.flags.isOnly) {
                    //We are going to insert an is_only_*-restriction. There can be only one.
                    m_restriction_bucket_list.at(index).clear();
                }
            }

            m_restriction_bucket_list.at(index).push_back(
                std::make_pair(restriction.toNode, restriction.flags.isOnly)
            );
        }

        m_barrier_node_list.insert(bn.begin(), bn.end());
        m_traffic_light_list.insert(tl.begin(), tl.end());

        DeallocatingVector< TarjanEdge > edge_list;
        BOOST_FOREACH(const NodeBasedEdge & input_edge, input_edges) {
            TarjanEdge edge;
            if(!input_edge.isForward()) {
                edge.source = input_edge.target();
                edge.target = input_edge.source();
                edge.data.backward = input_edge.isForward();
                edge.data.forward = input_edge.isBackward();
            } else {
                edge.source = input_edge.source();
                edge.target = input_edge.target();
                edge.data.forward = input_edge.isForward();
                edge.data.backward = input_edge.isBackward();
            }
            if(edge.source == edge.target) {
                continue;
            }

            edge.data.distance = (std::max)((int)input_edge.weight(), 1 );
            BOOST_ASSERT( edge.data.distance > 0 );
            edge.data.shortcut = false;
            edge.data.roundabout = input_edge.isRoundabout();
            edge.data.ignoreInGrid = input_edge.ignoreInGrid();
            edge.data.nameID = input_edge.name();
            edge.data.type = input_edge.type();
            edge.data.isAccessRestricted = input_edge.isAccessRestricted();
            edge.data.reversedEdge = false;
            edge_list.push_back( edge );
            if( edge.data.backward ) {
                std::swap( edge.source, edge.target );
                edge.data.forward = input_edge.isBackward();
                edge.data.backward = input_edge.isForward();
                edge.data.reversedEdge = true;
                edge_list.push_back( edge );
            }
        }
        std::vector<NodeBasedEdge>().swap(input_edges);
        BOOST_ASSERT_MSG(
            0 == input_edges.size() && 0 == input_edges.capacity(),
            "input edge vector not properly deallocated"
        );

        std::sort( edge_list.begin(), edge_list.end() );

        m_node_based_graph = boost::make_shared<TarjanDynamicGraph>(
            number_of_nodes,
            edge_list
        );
    }

    ~TarjanSCC() {
        m_node_based_graph.reset();
    }

    void Run() {
        //remove files from previous run if exist
        DeleteFileIfExists("component.dbf");
        DeleteFileIfExists("component.shx");
        DeleteFileIfExists("component.shp");

        Percent p(m_node_based_graph->GetNumberOfNodes());

        OGRRegisterAll();

        const char *pszDriverName = "ESRI Shapefile";
        OGRSFDriver * poDriver = OGRSFDriverRegistrar::GetRegistrar()->
            GetDriverByName( pszDriverName );
        if( NULL == poDriver ) {
            throw OSRMException("ESRI Shapefile driver not available");
        }
        OGRDataSource * poDS = poDriver->CreateDataSource(
            "component.shp",
            NULL
        );

        if( NULL == poDS ) {
            throw OSRMException("Creation of output file failed");
        }

        OGRLayer * poLayer = poDS->CreateLayer(
            "component",
            NULL,
            wkbLineString,
            NULL
        );

        if( NULL == poLayer ) {
            throw OSRMException("Layer creation failed.");
        }

        //The following is a hack to distinguish between stuff that happens
        //before the recursive call and stuff that happens after
        std::stack<std::pair<bool, TarjanStackFrame> > recursion_stack;
        //true = stuff before, false = stuff after call
        std::stack<NodeID> tarjan_stack;
        std::vector<unsigned> components_index(
            m_node_based_graph->GetNumberOfNodes(),
            UINT_MAX
        );
        std::vector<NodeID> component_size_vector;
        std::vector<TarjanNode> tarjan_node_list(
            m_node_based_graph->GetNumberOfNodes()
        );
        unsigned component_index = 0, size_of_current_component = 0;
        int index = 0;
        for(
            NodeID node = 0, last_node = m_node_based_graph->GetNumberOfNodes();
            node < last_node;
            ++node
        ) {
            if(UINT_MAX == components_index[node]) {
                recursion_stack.push(
                    std::make_pair(true, TarjanStackFrame(node,node))
                );
            }

            while(!recursion_stack.empty()) {
                bool before_recursion = recursion_stack.top().first;
                TarjanStackFrame currentFrame = recursion_stack.top().second;
                NodeID v = currentFrame.v;
                recursion_stack.pop();

                if(before_recursion) {
                    //Mark frame to handle tail of recursion
                    recursion_stack.push(std::make_pair(false, currentFrame));

                    //Mark essential information for SCC
                    tarjan_node_list[v].index = index;
                    tarjan_node_list[v].lowlink = index;
                    tarjan_stack.push(v);
                    tarjan_node_list[v].onStack = true;
                    ++index;

                    //Traverse outgoing edges
                    for(
                        TarjanDynamicGraph::EdgeIterator e2 = m_node_based_graph->BeginEdges(v);
                        e2 < m_node_based_graph->EndEdges(v);
                        ++e2
                    ) {
                        const TarjanDynamicGraph::NodeIterator vprime =
                            m_node_based_graph->GetTarget(e2);
                        if(UINT_MAX == tarjan_node_list[vprime].index) {
                            recursion_stack.push(
                                std::make_pair(
                                    true,
                                    TarjanStackFrame(vprime, v)
                                )
                            );
                        } else {
                            if(
                                tarjan_node_list[vprime].onStack &&
                                tarjan_node_list[vprime].index < tarjan_node_list[v].lowlink
                            ) {
                                tarjan_node_list[v].lowlink = tarjan_node_list[vprime].index;
                            }
                        }
                    }
                } else {
                    tarjan_node_list[currentFrame.parent].lowlink =
                        std::min(
                            tarjan_node_list[currentFrame.parent].lowlink,
                            tarjan_node_list[v].lowlink
                        );
                    //after recursion, lets do cycle checking
                    //Check if we found a cycle. This is the bottom part of the recursion
                    if(tarjan_node_list[v].lowlink == tarjan_node_list[v].index) {
                        NodeID vprime;
                        do {
                            vprime = tarjan_stack.top(); tarjan_stack.pop();
                            tarjan_node_list[vprime].onStack = false;
                            components_index[vprime] = component_index;
                            ++size_of_current_component;
                        } while( v != vprime);

                        component_size_vector.push_back(size_of_current_component);

                        if(size_of_current_component > 1000) {
                            SimpleLogger().Write() <<
                            "large component [" << component_index << "]=" <<
                            size_of_current_component;
                        }

                        ++component_index;
                        size_of_current_component = 0;
                    }
                }
            }
        }

        SimpleLogger().Write() <<
            "identified: " << component_size_vector.size() <<
            " many components, marking small components";

        unsigned size_one_counter = 0;
        for(unsigned i = 0, end = component_size_vector.size(); i < end; ++i){
            if(1 == component_size_vector[i]) {
                ++size_one_counter;
            }
        }

        SimpleLogger().Write() <<
            "identified " << size_one_counter << " SCCs of size 1";

        uint64_t total_network_distance = 0;
        p.reinit(m_node_based_graph->GetNumberOfNodes());
        for(
            TarjanDynamicGraph::NodeIterator u = 0, last_u_node = m_node_based_graph->GetNumberOfNodes();
            u < last_u_node;
            ++u
         ) {
            p.printIncrement();
            for(
                TarjanDynamicGraph::EdgeIterator e1 = m_node_based_graph->BeginEdges(u), last_edge = m_node_based_graph->EndEdges(u);
                e1 < last_edge;
                ++e1
            ) {
                if(!m_node_based_graph->GetEdgeData(e1).reversedEdge) {
                    continue;
                }
                const TarjanDynamicGraph::NodeIterator v = m_node_based_graph->GetTarget(e1);

                total_network_distance += 100*ApproximateDistance(
                        m_coordinate_list[u].lat,
                        m_coordinate_list[u].lon,
                        m_coordinate_list[v].lat,
                        m_coordinate_list[v].lon
                );

                if( SHRT_MAX != m_node_based_graph->GetEdgeData(e1).type ) {
                    BOOST_ASSERT(e1 != UINT_MAX);
                    BOOST_ASSERT(u != UINT_MAX);
                    BOOST_ASSERT(v != UINT_MAX);

                    const unsigned size_of_containing_component =
                        std::min(
                            component_size_vector[components_index[u]],
                            component_size_vector[components_index[v]]
                        );

                    //edges that end on bollard nodes may actually be in two distinct components
                    if(size_of_containing_component < 10) {
                        OGRLineString lineString;
                        lineString.addPoint(
                            m_coordinate_list[u].lon/COORDINATE_PRECISION,
                            m_coordinate_list[u].lat/COORDINATE_PRECISION
                        );
                        lineString.addPoint(
                            m_coordinate_list[v].lon/COORDINATE_PRECISION,
                            m_coordinate_list[v].lat/COORDINATE_PRECISION
                        );

                        OGRFeature * poFeature = OGRFeature::CreateFeature(
                            poLayer->GetLayerDefn()
                        );

                        poFeature->SetGeometry( &lineString );
                        if( OGRERR_NONE != poLayer->CreateFeature(poFeature) ) {
                            throw OSRMException(
                                "Failed to create feature in shapefile."
                            );
                        }
                        OGRFeature::DestroyFeature( poFeature );
                    }
                }
            }
        }
        OGRDataSource::DestroyDataSource( poDS );
        std::vector<NodeID>().swap(component_size_vector);
        BOOST_ASSERT_MSG(
            0 == component_size_vector.size() &&
            0 == component_size_vector.capacity(),
            "component_size_vector not properly deallocated"
        );

        std::vector<NodeID>().swap(components_index);
        BOOST_ASSERT_MSG(
            0 == components_index.size() && 0 == components_index.capacity(),
            "icomponents_index not properly deallocated"
        );

        SimpleLogger().Write()
            << "total network distance: " <<
             (uint64_t)total_network_distance/100/1000. <<
            " km";
    }

private:
    unsigned CheckForEmanatingIsOnlyTurn(const NodeID u, const NodeID v) const {
        std::pair < NodeID, NodeID > restriction_source = std::make_pair(u, v);
        RestrictionMap::const_iterator restriction_iterator = m_restriction_map.find(restriction_source);
        if (restriction_iterator != m_restriction_map.end()) {
            const unsigned index = restriction_iterator->second;
            BOOST_FOREACH(
                const RestrictionSource & restriction_target,
                m_restriction_bucket_list.at(index)
            ) {
                if(restriction_target.second) {
                    return restriction_target.first;
                }
            }
        }
        return UINT_MAX;
    }

    bool CheckIfTurnIsRestricted(
        const NodeID u,
        const NodeID v,
        const NodeID w
    ) const {
        //only add an edge if turn is not a U-turn except it is the end of dead-end street.
        std::pair < NodeID, NodeID > restriction_source = std::make_pair(u, v);
        RestrictionMap::const_iterator restriction_iterator = m_restriction_map.find(restriction_source);
        if (restriction_iterator != m_restriction_map.end()) {
            const unsigned index = restriction_iterator->second;
            BOOST_FOREACH(
                const restriction_target & restriction_target,
                m_restriction_bucket_list.at(index)
            ) {
                if(w == restriction_target.first) {
                    return true;
                }
            }
        }
        return false;
    }

    void DeleteFileIfExists(const std::string & file_name) const {
        if (boost::filesystem::exists(file_name) ) {
            boost::filesystem::remove(file_name);
        }
    }
};

#endif /* STRONGLYCONNECTEDCOMPONENTS_H_ */
