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

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/integer.hpp>
#include <boost/make_shared.hpp>
#include <boost/unordered_map.hpp>

#ifdef __APPLE__
    #include <gdal.h>
    #include <ogrsf_frmts.h>
#else
    #include <gdal/gdal.h>
    #include <gdal/ogrsf_frmts.h>
#endif
#include <cassert>
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
        unsigned edgeBasedNodeID;
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
        explicit TarjanStackFrame(NodeID _v, NodeID p) : v(_v), parent(p) {}
        NodeID v;
        NodeID parent;
    };

    typedef DynamicGraph<TarjanEdgeData> TarjanDynamicGraph;
    typedef TarjanDynamicGraph::InputEdge TarjanEdge;
    typedef std::pair<NodeID, NodeID> RestrictionSource;
    typedef std::pair<NodeID, bool>   RestrictionTarget;
    typedef std::vector<RestrictionTarget> EmanatingRestrictionsVector;
    typedef boost::unordered_map<RestrictionSource, unsigned > RestrictionMap;

    std::vector<NodeInfo>               inputNodeInfoList;
    unsigned numberOfTurnRestrictions;
    boost::shared_ptr<TarjanDynamicGraph>   _nodeBasedGraph;
    boost::unordered_map<NodeID, bool>          _barrierNodes;
    boost::unordered_map<NodeID, bool>          _trafficLights;

    std::vector<EmanatingRestrictionsVector> _restrictionBucketVector;
    RestrictionMap _restrictionMap;

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

    DeallocatingVector<EdgeBasedNode>   edgeBasedNodes;
public:
    TarjanSCC(
        int nodes,
        std::vector<NodeBasedEdge> & inputEdges,
        std::vector<NodeID> & bn,
        std::vector<NodeID> & tl,
        std::vector<TurnRestriction> & irs,
        std::vector<NodeInfo> & nI
    ) :
        inputNodeInfoList(nI),
        numberOfTurnRestrictions(irs.size())
    {
        BOOST_FOREACH(const TurnRestriction & restriction, irs) {
            std::pair<NodeID, NodeID> restrictionSource = std::make_pair(
                restriction.fromNode, restriction.viaNode
            );
            unsigned index;
            RestrictionMap::iterator restrIter = _restrictionMap.find(restrictionSource);
            if(restrIter == _restrictionMap.end()) {
                index = _restrictionBucketVector.size();
                _restrictionBucketVector.resize(index+1);
                _restrictionMap[restrictionSource] = index;
            } else {
                index = restrIter->second;
                //Map already contains an is_only_*-restriction
                if(_restrictionBucketVector.at(index).begin()->second)
                    continue;
                else if(restriction.flags.isOnly){
                    //We are going to insert an is_only_*-restriction. There can be only one.
                    _restrictionBucketVector.at(index).clear();
                }
            }

            _restrictionBucketVector.at(index).push_back(
                std::make_pair(restriction.toNode, restriction.flags.isOnly)
            );
        }

        BOOST_FOREACH(NodeID id, bn) {
            _barrierNodes[id] = true;
        }
        BOOST_FOREACH(NodeID id, tl) {
            _trafficLights[id] = true;
        }

        DeallocatingVector< TarjanEdge > edges;
        for ( std::vector< NodeBasedEdge >::const_iterator i = inputEdges.begin(); i != inputEdges.end(); ++i ) {

            TarjanEdge edge;
            if(!i->isForward()) {
                edge.source = i->target();
                edge.target = i->source();
                edge.data.backward = i->isForward();
                edge.data.forward = i->isBackward();
            } else {
                edge.source = i->source();
                edge.target = i->target();
                edge.data.forward = i->isForward();
                edge.data.backward = i->isBackward();
            }
            if(edge.source == edge.target)
                continue;

            edge.data.distance = (std::max)((int)i->weight(), 1 );
            assert( edge.data.distance > 0 );
            edge.data.shortcut = false;
            edge.data.roundabout = i->isRoundabout();
            edge.data.ignoreInGrid = i->ignoreInGrid();
            edge.data.nameID = i->name();
            edge.data.type = i->type();
            edge.data.isAccessRestricted = i->isAccessRestricted();
            edge.data.edgeBasedNodeID = edges.size();
            edge.data.reversedEdge = false;
            edges.push_back( edge );
            if( edge.data.backward ) {
                std::swap( edge.source, edge.target );
                edge.data.forward = i->isBackward();
                edge.data.backward = i->isForward();
                edge.data.edgeBasedNodeID = edges.size();
                edge.data.reversedEdge = true;
                edges.push_back( edge );
            }
        }
        std::vector<NodeBasedEdge>().swap(inputEdges);
        std::sort( edges.begin(), edges.end() );
        _nodeBasedGraph = boost::make_shared<TarjanDynamicGraph>( nodes, edges );
    }

    void Run() {
        //remove files from previous run if exist
        DeleteFileIfExists("component.dbf");
        DeleteFileIfExists("component.shx");
        DeleteFileIfExists("component.shp");

        Percent p(_nodeBasedGraph->GetNumberOfNodes());

        const char *pszDriverName = "ESRI Shapefile";
        OGRSFDriver *poDriver;

        OGRRegisterAll();

        poDriver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(
                pszDriverName );
        if( poDriver == NULL )
        {
            printf( "%s driver not available.\n", pszDriverName );
            exit( 1 );
        }
        OGRDataSource *poDS;

        poDS = poDriver->CreateDataSource( "component.shp", NULL );
        if( poDS == NULL ) {
            printf( "Creation of output file failed.\n" );
            exit( 1 );
        }

        OGRLayer *poLayer;

        poLayer = poDS->CreateLayer( "component", NULL, wkbLineString, NULL );
        if( poLayer == NULL ) {
            printf( "Layer creation failed.\n" );
            exit( 1 );
        }


        //The following is a hack to distinguish between stuff that happens before the recursive call and stuff that happens after
        std::stack<std::pair<bool, TarjanStackFrame> > recursionStack; //true = stuff before, false = stuff after call
        std::stack<NodeID> tarjanStack;
        std::vector<unsigned> componentsIndex(_nodeBasedGraph->GetNumberOfNodes(), UINT_MAX);
        std::vector<NodeID> vectorOfComponentSizes;
        std::vector<TarjanNode> tarjanNodes(_nodeBasedGraph->GetNumberOfNodes());
        unsigned currentComponent = 0, sizeOfCurrentComponent = 0;
        int index = 0;
        for(NodeID node = 0, endNodes = _nodeBasedGraph->GetNumberOfNodes(); node < endNodes; ++node) {
            if(UINT_MAX == componentsIndex[node]) {
                recursionStack.push(std::make_pair(true, TarjanStackFrame(node,node)) );
            }

            while(!recursionStack.empty()) {
                bool beforeRecursion = recursionStack.top().first;
                TarjanStackFrame currentFrame = recursionStack.top().second;
                NodeID v = currentFrame.v;
//                SimpleLogger().Write() << "popping node " << v << (beforeRecursion ? " before " : " after ") << "recursion";
                recursionStack.pop();

                if(beforeRecursion) {
                    //Mark frame to handle tail of recursion
                    recursionStack.push(std::make_pair(false, currentFrame));

                    //Mark essential information for SCC
                    tarjanNodes[v].index = index;
                    tarjanNodes[v].lowlink = index;
                    tarjanStack.push(v);
                    tarjanNodes[v].onStack = true;
                    ++index;
//                    SimpleLogger().Write() << "pushing " << v << " onto tarjan stack, idx[" << v << "]=" << tarjanNodes[v].index << ", lowlink["<< v << "]=" << tarjanNodes[v].lowlink;

                    //Traverse outgoing edges
                    for(TarjanDynamicGraph::EdgeIterator e2 = _nodeBasedGraph->BeginEdges(v); e2 < _nodeBasedGraph->EndEdges(v); ++e2) {
                        TarjanDynamicGraph::NodeIterator vprime = _nodeBasedGraph->GetTarget(e2);
//                        SimpleLogger().Write() << "traversing edge (" << v << "," << vprime << ")";
                        if(UINT_MAX == tarjanNodes[vprime].index) {

                            recursionStack.push(std::make_pair(true,TarjanStackFrame(vprime, v)));
                        } else {
//                            SimpleLogger().Write() << "Node " << vprime << " is already explored";
                            if(tarjanNodes[vprime].onStack) {
                                unsigned newLowlink = std::min(tarjanNodes[v].lowlink, tarjanNodes[vprime].index);
//                                SimpleLogger().Write() << "Setting lowlink[" << v << "] from " << tarjanNodes[v].lowlink << " to " << newLowlink;
                                tarjanNodes[v].lowlink = newLowlink;
//                            } else {
//                                SimpleLogger().Write() << "But node " << vprime << " is not on stack";
                            }
                        }
                    }
                } else {

//                    SimpleLogger().Write() << "we are at the end of recursion and checking node " << v;
                    { // setting lowlink in its own scope so it does not pollute namespace
                        //                        NodeID parent = (UINT_MAX == tarjanNodes[v].parent ? v : tarjanNodes[v].parent );
//                        SimpleLogger().Write() << "parent=" << currentFrame.parent;
//                        SimpleLogger().Write() << "tarjanNodes[" << v  << "].lowlink=" << tarjanNodes[v].lowlink << ", tarjanNodes[" << currentFrame.parent << "].lowlink=" << tarjanNodes[currentFrame.parent].lowlink;
                        //Note the index shift by 1 compared to the recursive version
                        tarjanNodes[currentFrame.parent].lowlink = std::min(tarjanNodes[currentFrame.parent].lowlink, tarjanNodes[v].lowlink);
//                        SimpleLogger().Write() << "Setting tarjanNodes[" << currentFrame.parent  <<"].lowlink=" << tarjanNodes[currentFrame.parent].lowlink;
                    }
//                    SimpleLogger().Write() << "tarjanNodes[" << v  << "].lowlink=" << tarjanNodes[v].lowlink << ", tarjanNodes[" << v  << "].index=" << tarjanNodes[v].index;

                    //after recursion, lets do cycle checking
                    //Check if we found a cycle. This is the bottom part of the recursion
                    if(tarjanNodes[v].lowlink == tarjanNodes[v].index) {
                        NodeID vprime;
                        do {
//                            SimpleLogger().Write() << "identified component " << currentComponent << ": " << tarjanStack.top();
                            vprime = tarjanStack.top(); tarjanStack.pop();
                            tarjanNodes[vprime].onStack = false;
                            componentsIndex[vprime] = currentComponent;
                            ++sizeOfCurrentComponent;
                        } while( v != vprime);
                        vectorOfComponentSizes.push_back(sizeOfCurrentComponent);
                        if(sizeOfCurrentComponent > 1000)
                            SimpleLogger().Write() << "large component [" << currentComponent << "]=" << sizeOfCurrentComponent;
                        ++currentComponent;
                        sizeOfCurrentComponent = 0;
                    }
                }
            }
        }

        SimpleLogger().Write() << "identified: " << vectorOfComponentSizes.size() << " many components, marking small components";

        int singleCounter = 0;
        for(unsigned i = 0; i < vectorOfComponentSizes.size(); ++i){
            if(1 == vectorOfComponentSizes[i])
                ++singleCounter;
        }
        SimpleLogger().Write() << "identified " << singleCounter << " SCCs of size 1";
        uint64_t total_network_distance = 0;
        p.reinit(_nodeBasedGraph->GetNumberOfNodes());
        for(TarjanDynamicGraph::NodeIterator u = 0; u < _nodeBasedGraph->GetNumberOfNodes(); ++u ) {
            p.printIncrement();
            for(TarjanDynamicGraph::EdgeIterator e1 = _nodeBasedGraph->BeginEdges(u); e1 < _nodeBasedGraph->EndEdges(u); ++e1) {
                if(_nodeBasedGraph->GetEdgeData(e1).reversedEdge) {
                    continue;
                }
                TarjanDynamicGraph::NodeIterator v = _nodeBasedGraph->GetTarget(e1);

                total_network_distance += 100*ApproximateDistance(
                        inputNodeInfoList[u].lat,
                        inputNodeInfoList[u].lon,
                        inputNodeInfoList[v].lat,
                        inputNodeInfoList[v].lon
                );

                if(_nodeBasedGraph->GetEdgeData(e1).type != SHRT_MAX) {
                    assert(e1 != UINT_MAX);
                    assert(u != UINT_MAX);
                    assert(v != UINT_MAX);

                    //edges that end on bollard nodes may actually be in two distinct components
                    if(std::min(vectorOfComponentSizes[componentsIndex[u]], vectorOfComponentSizes[componentsIndex[v]]) < 10) {

                        //INFO("(" << inputNodeInfoList[u].lat/COORDINATE_PRECISION << ";" << inputNodeInfoList[u].lon/COORDINATE_PRECISION << ") -> (" << inputNodeInfoList[v].lat/COORDINATE_PRECISION << ";" << inputNodeInfoList[v].lon/COORDINATE_PRECISION << ")");
                        OGRLineString lineString;
                        lineString.addPoint(inputNodeInfoList[u].lon/COORDINATE_PRECISION, inputNodeInfoList[u].lat/COORDINATE_PRECISION);
                        lineString.addPoint(inputNodeInfoList[v].lon/COORDINATE_PRECISION, inputNodeInfoList[v].lat/COORDINATE_PRECISION);
                        OGRFeature *poFeature;
                        poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
                        poFeature->SetGeometry( &lineString );
                        if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE ) {
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
        std::vector<NodeID>().swap(vectorOfComponentSizes);
        std::vector<NodeID>().swap(componentsIndex);
        SimpleLogger().Write() << "total network distance: " << (uint64_t)total_network_distance/100/1000. << " km";
    }
private:
    unsigned CheckForEmanatingIsOnlyTurn(const NodeID u, const NodeID v) const {
        std::pair < NodeID, NodeID > restrictionSource = std::make_pair(u, v);
        RestrictionMap::const_iterator restrIter = _restrictionMap.find(restrictionSource);
        if (restrIter != _restrictionMap.end()) {
            unsigned index = restrIter->second;
            BOOST_FOREACH(RestrictionSource restrictionTarget, _restrictionBucketVector.at(index)) {
                if(restrictionTarget.second) {
                    return restrictionTarget.first;
                }
            }
        }
        return UINT_MAX;
    }
    bool CheckIfTurnIsRestricted(const NodeID u, const NodeID v, const NodeID w) const {
        //only add an edge if turn is not a U-turn except it is the end of dead-end street.
        std::pair < NodeID, NodeID > restrictionSource = std::make_pair(u, v);
        RestrictionMap::const_iterator restrIter = _restrictionMap.find(restrictionSource);
        if (restrIter != _restrictionMap.end()) {
            unsigned index = restrIter->second;
            BOOST_FOREACH(RestrictionTarget restrictionTarget, _restrictionBucketVector.at(index)) {
                if(w == restrictionTarget.first)
                    return true;
            }
        }
        return false;
    }

    void DeleteFileIfExists(const std::string file_name) const {
        if (boost::filesystem::exists(file_name) ) {
            boost::filesystem::remove(file_name);
        }
    }
};

#endif /* STRONGLYCONNECTEDCOMPONENTS_H_ */
