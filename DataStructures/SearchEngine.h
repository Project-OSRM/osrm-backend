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
 */

#ifndef SEARCHENGINE_H_
#define SEARCHENGINE_H_

#include <climits>
#include <deque>

#include "BinaryHeap.h"
#include "PhantomNodes.h"
#include "../Util/StringUtil.h"
#include "../typedefs.h"

struct _HeapData {
    NodeID parent;
    _HeapData( NodeID p ) : parent(p) { }
};

struct _Statistics {
    _Statistics () : insertedNodes(0), stalledNodes(0), meetingNodes(0), deleteMins(0), decreasedNodes(0), oqf(0), eqf(0), df(0), preprocTime(0) {};
    void Reset() {
        insertedNodes = 0;
        stalledNodes = 0;
        meetingNodes = 0;
        deleteMins = 0;
        decreasedNodes = 0;
    }
    unsigned insertedNodes;
    unsigned stalledNodes;
    unsigned meetingNodes;
    unsigned deleteMins;
    unsigned decreasedNodes;
    unsigned oqf;
    unsigned eqf;
    unsigned df;
    double preprocTime;
};

struct _InsertedNodes {
    NodeID forward1;
    NodeID forward2;
    NodeID backward1;
    NodeID backward2;
    _InsertedNodes() : forward1(UINT_MAX), forward2(UINT_MAX), backward1(UINT_MAX), backward2(UINT_MAX) {};
    void BackInsert(NodeID n) {
        if(backward1 == UINT_MAX) {
            backward1 = n;
        } else {
            backward2 = n;
        }
    }
    void ForwInsert( NodeID n) {
        if(forward1 == UINT_MAX) {
            forward1 = n;
        } else {
            forward2 = n;
        }
    }

    inline bool isForwardInserted(NodeID n) {
        return forward1 == n || forward2 == n;
    }

    inline bool isBackwardInserted (NodeID n) {
        return backward1 == n || backward2 == n;
    }
};


typedef BinaryHeap< NodeID, int, int, _HeapData, DenseStorage<NodeID, NodeID>  > _Heap;

template<typename EdgeData, typename GraphT, typename NodeHelperT = NodeInformationHelpDesk>
class SearchEngine {
private:
    const GraphT * _graph;
    NodeHelperT * nodeHelpDesk;
    std::vector<string> * _names;
    inline double absDouble(double input) { if(input < 0) return input*(-1); else return input;}
public:
    SearchEngine(GraphT * g, NodeHelperT * nh, vector<string> * n = new vector<string>()) : _graph(g), nodeHelpDesk(nh), _names(n) {}
    ~SearchEngine() {}

    inline const void getCoordinatesForNodeID(NodeID id, _Coordinate& result) const {
        result.lat = nodeHelpDesk->getLatitudeOfNode(id);
        result.lon = nodeHelpDesk->getLongitudeOfNode(id);
    }

    unsigned int numberOfNodes() const {
        return nodeHelpDesk->getNumberOfNodes();
    }

    unsigned int ComputeRoute(PhantomNodes &phantomNodes, vector<_PathData > & path) {
        bool startEdgeIsReversedInGraph = false;
        bool targetEdgeIsReversed = false;

        unsigned int _upperbound = UINT_MAX;

        if(!phantomNodes.AtLeastOnePhantomNodeIsUINTMAX())
            return _upperbound;

        EdgeID sourceEdgeID = _graph->FindEdgeIndicateIfReverse( phantomNodes.startPhantom.startNode, phantomNodes.startPhantom.targetNode, startEdgeIsReversedInGraph);
        if(sourceEdgeID == UINT_MAX){
            return _upperbound;
        }

        EdgeID targetEdgeID = _graph->FindEdgeIndicateIfReverse( phantomNodes.targetPhantom.startNode, phantomNodes.targetPhantom.targetNode, targetEdgeIsReversed);
        if(targetEdgeID == UINT_MAX){
            return _upperbound;
        }


        _InsertedNodes _insertedNodes;
        _Heap _forwardHeap(nodeHelpDesk->getNumberOfNodes());
        _Heap _backwardHeap(nodeHelpDesk->getNumberOfNodes());

        NodeID middle = ( NodeID ) 0;

        if( phantomNodes.PhantomsAreOnSameEdge() ) {
            const EdgeData& currentEdgeData = _graph->GetEdgeData(sourceEdgeID);
            EdgeWeight w = currentEdgeData.distance;

            //check if target is reachable from start on same edge
            if(currentEdgeData.forward && currentEdgeData.backward) {
                _upperbound = absDouble(  w*phantomNodes.targetPhantom.ratio);
                return _upperbound/10;
            } else {
                if((startEdgeIsReversedInGraph && (phantomNodes.startPhantom.ratio > phantomNodes.targetPhantom.ratio)) || (!startEdgeIsReversedInGraph && (phantomNodes.startPhantom.ratio < phantomNodes.targetPhantom.ratio))) {
                    _backwardHeap.Insert(phantomNodes.startPhantom.startNode, absDouble(  w*phantomNodes.startPhantom.ratio), phantomNodes.startPhantom.startNode);
                    _insertedNodes.BackInsert(phantomNodes.startPhantom.startNode);
                    _forwardHeap.Insert(phantomNodes.startPhantom.targetNode, absDouble(  w-w*phantomNodes.startPhantom.ratio), phantomNodes.startPhantom.targetNode);
                    _insertedNodes.ForwInsert(phantomNodes.startPhantom.targetNode);
                } else {
                    _upperbound = absDouble(  w*phantomNodes.targetPhantom.ratio);
                    return _upperbound/10;
                }
            }
        }

        //insert start and/or target node of start edge
        const EdgeData& sourceEdgeData = _graph->GetEdgeData(sourceEdgeID);
        EdgeWeight sw = sourceEdgeData.distance;

        if( (sourceEdgeData.backward && !startEdgeIsReversedInGraph) || (sourceEdgeData.forward && startEdgeIsReversedInGraph) ){
            _forwardHeap.Insert(phantomNodes.startPhantom.startNode, absDouble(  sw*phantomNodes.startPhantom.ratio), phantomNodes.startPhantom.startNode);
            _insertedNodes.ForwInsert(phantomNodes.startPhantom.startNode);
        }
        if( (sourceEdgeData.backward && startEdgeIsReversedInGraph) || (sourceEdgeData.forward && !startEdgeIsReversedInGraph) ) {
            _forwardHeap.Insert(phantomNodes.startPhantom.targetNode, absDouble(sw-sw*phantomNodes.startPhantom.ratio), phantomNodes.startPhantom.targetNode);
            _insertedNodes.ForwInsert(phantomNodes.startPhantom.targetNode);
        }

        //insert start and/or target node of target edge id
        const EdgeData& targetEdgeData = _graph->GetEdgeData(targetEdgeID);
        EdgeWeight tw = targetEdgeData.distance;

        if( (targetEdgeData.backward && !targetEdgeIsReversed) || (targetEdgeData.forward && targetEdgeIsReversed) ) {
            _backwardHeap.Insert(phantomNodes.targetPhantom.targetNode, absDouble(  tw*phantomNodes.targetPhantom.ratio), phantomNodes.targetPhantom.targetNode);
            _insertedNodes.BackInsert(phantomNodes.targetPhantom.targetNode);
        }
        if( (targetEdgeData.backward && targetEdgeIsReversed) || (targetEdgeData.forward && !targetEdgeIsReversed) ) {
            _backwardHeap.Insert(phantomNodes.targetPhantom.startNode, absDouble(tw-tw*phantomNodes.targetPhantom.ratio), phantomNodes.targetPhantom.startNode);
            _insertedNodes.BackInsert(phantomNodes.targetPhantom.startNode);
        }


        while(_forwardHeap.Size() + _backwardHeap.Size() > 0) {
            if ( _forwardHeap.Size() > 0 ) {
                _RoutingStep( _forwardHeap, _backwardHeap, true, &middle, &_upperbound );
            }
            if ( _backwardHeap.Size() > 0 ) {
                _RoutingStep( _backwardHeap, _forwardHeap, false, &middle, &_upperbound );
            }
        }

        if ( _upperbound == UINT_MAX ) {
            return _upperbound;
        }

        NodeID pathNode = middle;
        deque< NodeID > packedPath;

        while ( false == _insertedNodes.isForwardInserted(pathNode) ) {
            pathNode = _forwardHeap.GetData( pathNode ).parent;
            packedPath.push_front( pathNode );
        }

        packedPath.push_back( middle );
        pathNode = middle;

        while ( false == _insertedNodes.isBackwardInserted(pathNode) ){
            pathNode = _backwardHeap.GetData( pathNode ).parent;
            packedPath.push_back( pathNode );
        }

        path.push_back( _PathData(packedPath[0]) );
        for(deque<NodeID>::size_type i = 0; i < packedPath.size()-1; i++) {
            _UnpackEdge(packedPath[i], packedPath[i+1], path);
        }

        packedPath.clear();
        return _upperbound/10;
    }

    unsigned int ComputeDistanceBetweenNodes(NodeID start, NodeID target) {
        _Heap  _forwardHeap(_graph->GetNumberOfNodes());
        _Heap  _backwardHeap(_graph->GetNumberOfNodes());
        NodeID middle(UINT_MAX);
        unsigned int _upperbound = UINT_MAX;

        _forwardHeap.Insert(start, 0, start);
        _backwardHeap.Insert(target, 0, target);

        while(_forwardHeap.Size() + _backwardHeap.Size() > 0) {
            if ( _forwardHeap.Size() > 0 ) {
                _RoutingStep( _forwardHeap, _backwardHeap, true, &middle, &_upperbound );
            }
            if ( _backwardHeap.Size() > 0 ) {
                _RoutingStep( _backwardHeap, _forwardHeap, false, &middle, &_upperbound );
            }
        }
        return _upperbound;
    }

    unsigned int ComputeDistanceBetweenNodesWithStats(NodeID start, NodeID target, _Statistics& stats) {
        _Heap  _forwardHeap(_graph->GetNumberOfNodes());
        _Heap  _backwardHeap(_graph->GetNumberOfNodes());
        NodeID middle(UINT_MAX);
        unsigned int _upperbound = UINT_MAX;

        _forwardHeap.Insert(start, 0, start);
        _backwardHeap.Insert(target, 0, target);
        stats.insertedNodes += 2;

        while(_forwardHeap.Size() + _backwardHeap.Size() > 0) {
            if ( _forwardHeap.Size() > 0 ) {
                _RoutingStepWithStats( _forwardHeap, _backwardHeap, true, &middle, &_upperbound, stats );
            }
            if ( _backwardHeap.Size() > 0 ) {
                _RoutingStepWithStats( _backwardHeap, _forwardHeap, false, &middle, &_upperbound, stats );
            }
        }
        return _upperbound;
    }

    inline unsigned int findNearestNodeForLatLon(const _Coordinate& coord, _Coordinate& result) const
    {
        nodeHelpDesk->findNearestNodeCoordForLatLon( coord, result );
        return 0;

    }

    inline bool FindRoutingStarts(const _Coordinate &start, const _Coordinate &target, PhantomNodes & routingStarts)  {
        nodeHelpDesk->FindRoutingStarts(start, target, routingStarts);
        return true;
    }

    inline bool FindPhantomNodeForCoordinate(const _Coordinate &location, PhantomNode & result)  {
        return nodeHelpDesk->FindPhantomNodeForCoordinate(location, result);
    }

    inline NodeID GetNameIDForOriginDestinationNodeID(NodeID s, NodeID t) const {
        //INFO("Getting nameID for s=" << s << " and t=" << t);
        if(s==t)
            return 0;
        EdgeID e = _graph->FindEdge( s, t );
        if(e == UINT_MAX)
            e = _graph->FindEdge( t, s );
        if(UINT_MAX == e) {
            // INFO("edge not found for start " << s << ", target " << t)
            return 0;
        }
        assert(e != UINT_MAX);
        const EdgeData ed = _graph->GetEdgeData(e);
        return ed.middleName.nameID;
    }

    inline NodeID GetWeightForOriginDestinationNodeID(NodeID s, NodeID t) const {
        assert(s!=t);
        EdgeID e = _graph->FindEdge( s, t );
        if(e == UINT_MAX)
            e = _graph->FindEdge( t, s );
        assert(e != UINT_MAX);
        const EdgeData ed = _graph->GetEdgeData(e);
        return ed.distance;
    }

    inline std::string &GetUnescapedNameForNameID(const NodeID nameID) const {
        return (nameID >= _names->size() ? _names->at(0) : _names->at(nameID) );
    }

    inline std::string GetEscapedNameForOriginDestinationNodeID(NodeID s, NodeID t) const {
        NodeID nameID = GetNameIDForOriginDestinationNodeID(s, t);
        return ( GetEscapedNameForNameID(nameID) );
    }

    inline std::string GetEscapedNameForNameID(const NodeID nameID) const {
        return ( (nameID >= _names->size() || nameID == 0) ? std::string("") : HTMLEntitize(_names->at(nameID)) );
    }

    inline short GetTypeOfEdgeForOriginDestinationNodeID(NodeID s, NodeID t) const {
        assert(s!=t);
        EdgeID e = _graph->FindEdge( s, t );
        if(e == UINT_MAX)
            e = _graph->FindEdge( t, s );
        assert(e != UINT_MAX);
        const EdgeData ed = _graph->GetEdgeData(e);
        return ed.type;
    }

    //    inline void RegisterThread(const unsigned k, const unsigned v) {
    //        nodeHelpDesk->RegisterThread(k,v);
    //    }
private:

    inline void _RoutingStep(_Heap& _forwardHeap, _Heap &_backwardHeap, const bool& forwardDirection, NodeID * middle, unsigned int * _upperbound) {
        const NodeID node = _forwardHeap.DeleteMin();
        const unsigned int distance = _forwardHeap.GetKey( node );
        if ( _backwardHeap.WasInserted( node ) ) {
            const unsigned int newDistance = _backwardHeap.GetKey( node ) + distance;
            if ( newDistance < *_upperbound ) {
                *middle = node;
                *_upperbound = newDistance;
            }
        }
        if ( distance > *_upperbound ) {
            _forwardHeap.DeleteAll();
            return;
        }


        for ( typename GraphT::EdgeIterator edge = _graph->BeginEdges( node ); edge < _graph->EndEdges(node); edge++ ) {
            const NodeID to = _graph->GetTarget(edge);
            const EdgeWeight edgeWeight = _graph->GetEdgeData(edge).distance;

            assert( edgeWeight > 0 );

            //Stalling
            bool backwardDirectionFlag = (!forwardDirection) ? _graph->GetEdgeData(edge).forward : _graph->GetEdgeData(edge).backward;
            if(_forwardHeap.WasInserted( to )) {
                if(backwardDirectionFlag) {
                    if(_forwardHeap.GetKey( to ) + edgeWeight < distance) {
                        return;
                    }
                }
            }
        }
        for ( typename GraphT::EdgeIterator edge = _graph->BeginEdges( node ); edge < _graph->EndEdges(node); edge++ ) {
            const NodeID to = _graph->GetTarget(edge);
            const EdgeWeight edgeWeight = _graph->GetEdgeData(edge).distance;

            assert( edgeWeight > 0 );
            const int toDistance = distance + edgeWeight;


            bool forwardDirectionFlag = (forwardDirection ? _graph->GetEdgeData(edge).forward : _graph->GetEdgeData(edge).backward );
            if(forwardDirectionFlag) {
                //New Node discovered -> Add to Heap + Node Info Storage
                if ( !_forwardHeap.WasInserted( to ) ) {
                    _forwardHeap.Insert( to, toDistance, node );
                }
                //Found a shorter Path -> Update distance
                else if ( toDistance < _forwardHeap.GetKey( to ) ) {
                    _forwardHeap.GetData( to ).parent = node;
                    _forwardHeap.DecreaseKey( to, toDistance );
                    //new parent
                }
            }
        }
    }

    inline void _RoutingStepWithStats( _Heap& _forwardHeap, _Heap &_backwardHeap, const bool& forwardDirection, NodeID * middle, unsigned int * _upperbound, _Statistics& stats) {
        const NodeID node = _forwardHeap.DeleteMin();
        stats.deleteMins++;
        const unsigned int distance = _forwardHeap.GetKey( node );
        if ( _backwardHeap.WasInserted( node ) ) {
            const unsigned int newDistance = _backwardHeap.GetKey( node ) + distance;
            if ( newDistance < *_upperbound ) {
                *middle = node;
                *_upperbound = newDistance;
            }
        }
        if ( distance > *_upperbound ) {
            stats.meetingNodes++;
            _forwardHeap.DeleteAll();
            return;
        }


        for ( typename GraphT::EdgeIterator edge = _graph->BeginEdges( node ); edge < _graph->EndEdges(node); edge++ ) {
            const EdgeData& ed = _graph->GetEdgeData(edge);
            const NodeID to = _graph->GetTarget(edge);
            const EdgeWeight edgeWeight = ed.distance;

            assert( edgeWeight > 0 );
            const int toDistance = distance + edgeWeight;

            //Stalling
            if(_forwardHeap.WasInserted( to )) {
                if(!forwardDirection ? ed.forward : ed.backward) {
                    if(_forwardHeap.GetKey( to ) + edgeWeight < distance) {
                        stats.stalledNodes++;
                        return;
                    }
                }
            }

            if(forwardDirection ? ed.forward : ed.backward ) {
                //New Node discovered -> Add to Heap + Node Info Storage
                if ( !_forwardHeap.WasInserted( to ) ) {
                    _forwardHeap.Insert( to, toDistance, node );
                    stats.insertedNodes++;
                }
                //Found a shorter Path -> Update distance
                else if ( toDistance < _forwardHeap.GetKey( to ) ) {
                    _forwardHeap.GetData( to ).parent = node;
                    _forwardHeap.DecreaseKey( to, toDistance );
                    stats.decreasedNodes++;
                    //new parent
                }
            }
        }
    }

    inline bool _UnpackEdge( const NodeID source, const NodeID target, std::vector< _PathData >& path ) {
        assert(source != target);
        //find edge first.
        bool forward = true;
        typename GraphT::EdgeIterator smallestEdge = SPECIAL_EDGEID;
        EdgeWeight smallestWeight = UINT_MAX;
        for(typename GraphT::EdgeIterator eit = _graph->BeginEdges(source); eit < _graph->EndEdges(source); eit++) {
            const EdgeWeight weight = _graph->GetEdgeData(eit).distance;
            if(_graph->GetTarget(eit) == target && weight < smallestWeight && _graph->GetEdgeData(eit).forward) {
                smallestEdge = eit; smallestWeight = weight;
            }
        }
        if(smallestEdge == SPECIAL_EDGEID) {
            for(typename GraphT::EdgeIterator eit = _graph->BeginEdges(target); eit < _graph->EndEdges(target); eit++) {
                const EdgeWeight weight = _graph->GetEdgeData(eit).distance;
                if(_graph->GetTarget(eit) == source && weight < smallestWeight && _graph->GetEdgeData(eit).backward) {
                    smallestEdge = eit; smallestWeight = weight;
                    forward = false;
                }
            }
        }

        assert(smallestWeight != SPECIAL_EDGEID); //no edge found. This should not happen at all!

        const EdgeData& ed = _graph->GetEdgeData(smallestEdge);
        if(ed.shortcut) {//unpack
            const NodeID middle = ed.middleName.middle;
            _UnpackEdge(source, middle, path);
            _UnpackEdge(middle, target, path);
            return false;
        } else {
            assert(!ed.shortcut);
            path.push_back(_PathData(target) );
            return true;
        }
    }
};

#endif /* SEARCHENGINE_H_ */
