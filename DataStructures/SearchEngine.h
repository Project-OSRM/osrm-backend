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
#include "../Util/StrIngUtil.h"
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


typedef BinaryHeap< NodeID, int, int, _HeapData, DenseStorage< NodeID, unsigned > > _Heap;

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

    inline const void getNodeInfo(NodeID id, _Coordinate& result) const
    {
        result.lat = nodeHelpDesk->getLatitudeOfNode(id);
        result.lon = nodeHelpDesk->getLongitudeOfNode(id);
    }

    unsigned int numberOfNodes() const {
        return nodeHelpDesk->getNumberOfNodes();
    }

    unsigned int ComputeRoute(PhantomNodes * phantomNodes, vector<_PathData > * path, _Coordinate& startCoord, _Coordinate& targetCoord) {

        bool onSameEdge = false;
        bool onSameEdgeReversed = false;
        bool startReverse = false;
        bool targetReverse = false;

        _InsertedNodes _insertedNodes;
        _Heap _forwardHeap(nodeHelpDesk->getNumberOfNodes());
        _Heap _backwardHeap(nodeHelpDesk->getNumberOfNodes());

        NodeID middle = ( NodeID ) 0;
        unsigned int _upperbound = std::numeric_limits<unsigned int>::max();

        if(phantomNodes->startNode1 == UINT_MAX || phantomNodes->startNode2 == UINT_MAX)
            return _upperbound;

        if( (phantomNodes->startNode1 == phantomNodes->targetNode1 && phantomNodes->startNode2 == phantomNodes->targetNode2 ) ||
                (phantomNodes->startNode1 == phantomNodes->targetNode2 && phantomNodes->startNode2 == phantomNodes->targetNode1 ) )
        {
            bool reverse = false;
            EdgeID currentEdge = _graph->FindEdge( phantomNodes->startNode1, phantomNodes->startNode2 );
            if(currentEdge == UINT_MAX){
                currentEdge = _graph->FindEdge( phantomNodes->startNode2, phantomNodes->startNode1 );
                reverse = true;
            }

            if(currentEdge == UINT_MAX){
                return _upperbound;
            }

            if(phantomNodes->startRatio < phantomNodes->targetRatio && _graph->GetEdgeData(currentEdge).forward) {
                onSameEdge = true;
                _upperbound = 10 * ApproximateDistance(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon, phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon);
            } else if(phantomNodes->startRatio > phantomNodes->targetRatio && _graph->GetEdgeData(currentEdge).backward && !reverse)
            {
                onSameEdge = true;
                _upperbound = 10 * ApproximateDistance(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon, phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon);
            } else if(phantomNodes->startRatio < phantomNodes->targetRatio && _graph->GetEdgeData(currentEdge).backward) {
                onSameEdge = true;
                _upperbound = 10 * ApproximateDistance(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon, phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon);
            } else if(phantomNodes->startRatio > phantomNodes->targetRatio && _graph->GetEdgeData(currentEdge).forward && _graph->GetEdgeData(currentEdge).backward) {
                onSameEdge = true;
                _upperbound = 10 * ApproximateDistance(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon, phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon);
            } else if(phantomNodes->startRatio > phantomNodes->targetRatio) {
                onSameEdgeReversed = true;

                EdgeWeight w = _graph->GetEdgeData( currentEdge ).distance;
                _forwardHeap.Insert(phantomNodes->startNode2, absDouble(  w*phantomNodes->startRatio), phantomNodes->startNode2);
                _insertedNodes.ForwInsert(phantomNodes->startNode2);
                _backwardHeap.Insert(phantomNodes->startNode1, absDouble(  w-w*phantomNodes->targetRatio), phantomNodes->startNode1);
                _insertedNodes.BackInsert(phantomNodes->startNode1);
            }
        }

        if(phantomNodes->startNode1 != UINT_MAX) {
            EdgeID edge = _graph->FindEdge( phantomNodes->startNode1, phantomNodes->startNode2);
            if(edge == UINT_MAX){
                edge = _graph->FindEdge( phantomNodes->startNode2, phantomNodes->startNode1 );
                if(edge == UINT_MAX){
                    return _upperbound;
                }
                startReverse = true;
            }
            const EdgeData& ed = _graph->GetEdgeData(edge);
            EdgeWeight w = ed.distance;

            if( (ed.backward && !startReverse) || (ed.forward && startReverse) ){
                _forwardHeap.Insert(phantomNodes->startNode1, absDouble(  w*phantomNodes->startRatio), phantomNodes->startNode1);
                _insertedNodes.ForwInsert(phantomNodes->startNode1);
            }
            if( (ed.backward && startReverse) || (ed.forward && !startReverse) ) {
                _forwardHeap.Insert(phantomNodes->startNode2, absDouble(w-w*phantomNodes->startRatio), phantomNodes->startNode2);
                _insertedNodes.ForwInsert(phantomNodes->startNode2);
            }
        }
        if(phantomNodes->targetNode1 != UINT_MAX && !onSameEdgeReversed) {
            EdgeID edge = _graph->FindEdge( phantomNodes->targetNode1, phantomNodes->targetNode2);
            if(edge == UINT_MAX){
                edge = _graph->FindEdge( phantomNodes->targetNode2, phantomNodes->targetNode1 );
                targetReverse = true;
            }
            if(edge == UINT_MAX){
                return _upperbound;
            }

            const EdgeData& ed = _graph->GetEdgeData(edge);
            EdgeWeight w = ed.distance;

            if( (ed.backward && !targetReverse) || (ed.forward && targetReverse) ) {
                _backwardHeap.Insert(phantomNodes->targetNode2, absDouble(  w*phantomNodes->targetRatio), phantomNodes->targetNode2);
                _insertedNodes.BackInsert(phantomNodes->targetNode2);
            }
            if( (ed.backward && targetReverse) || (ed.forward && !targetReverse) ) {
                _backwardHeap.Insert(phantomNodes->targetNode1, absDouble(w-w*phantomNodes->targetRatio), phantomNodes->targetNode1);
                _insertedNodes.BackInsert(phantomNodes->targetNode1);
            }
        }
        //        double time = get_timestamp();

        while(_forwardHeap.Size() + _backwardHeap.Size() > 0) {
            if ( _forwardHeap.Size() > 0 ) {
                _RoutingStep( &_forwardHeap, &_backwardHeap, true, &middle, &_upperbound );
            }
            if ( _backwardHeap.Size() > 0 ) {
                _RoutingStep( &_backwardHeap, &_forwardHeap, false, &middle, &_upperbound );
            }
        }
        //        std::cout << "[debug] computing distance took " << get_timestamp() - time << std::endl;
        //        time = get_timestamp();

        if ( _upperbound == std::numeric_limits< unsigned int >::max() || onSameEdge ) {
            return _upperbound;
        }

        NodeID pathNode = middle;
        deque< NodeID > packedPath;

        while ( false == _insertedNodes.isForwardInserted(pathNode) ) {
            pathNode = _forwardHeap.GetData( pathNode ).parent;
            packedPath.push_front( pathNode );
        }
        //        NodeID realStart = pathNode;
        packedPath.push_back( middle );
        pathNode = middle;

        while ( false == _insertedNodes.isBackwardInserted(pathNode) ){
            pathNode = _backwardHeap.GetData( pathNode ).parent;
            packedPath.push_back( pathNode );
        }

        path->push_back( _PathData(packedPath[0]) );
        for(deque<NodeID>::size_type i = 0; i < packedPath.size()-1; i++) {
            _UnpackEdge(packedPath[i], packedPath[i+1], path);
        }

        packedPath.clear();
        //        std::cout << "[debug] unpacking path took " << get_timestamp() - time << std::endl;

        return _upperbound/10;
    }

    unsigned int ComputeDistanceBetweenNodes(NodeID start, NodeID target) {
        _Heap * _forwardHeap = new _Heap(_graph->GetNumberOfNodes());
        _Heap * _backwardHeap = new _Heap(_graph->GetNumberOfNodes());
        NodeID middle = ( NodeID ) 0;
        unsigned int _upperbound = std::numeric_limits<unsigned int>::max();

        _forwardHeap->Insert(start, 0, start);
        _backwardHeap->Insert(target, 0, target);

        while(_forwardHeap->Size() + _backwardHeap->Size() > 0)
        {
            if ( _forwardHeap->Size() > 0 ) {
                _RoutingStep( _forwardHeap, _backwardHeap, true, &middle, &_upperbound );
            }
            if ( _backwardHeap->Size() > 0 ) {
                _RoutingStep( _backwardHeap, _forwardHeap, false, &middle, &_upperbound );
            }
        }
        delete _forwardHeap;
        delete _backwardHeap;
        return _upperbound;
    }

    unsigned int ComputeDistanceBetweenNodesWithStats(NodeID start, NodeID target, _Statistics& stats) {
        _Heap * _forwardHeap = new _Heap(_graph->GetNumberOfNodes());
        _Heap * _backwardHeap = new _Heap(_graph->GetNumberOfNodes());
        NodeID middle = ( NodeID ) 0;
        unsigned int _upperbound = std::numeric_limits<unsigned int>::max();

        _forwardHeap->Insert(start, 0, start);
        _backwardHeap->Insert(target, 0, target);
        stats.insertedNodes += 2;

        while(_forwardHeap->Size() + _backwardHeap->Size() > 0)
        {
            if ( _forwardHeap->Size() > 0 ) {
                _RoutingStepWithStats( _forwardHeap, _backwardHeap, true, &middle, &_upperbound, stats );
            }
            if ( _backwardHeap->Size() > 0 ) {
                _RoutingStepWithStats( _backwardHeap, _forwardHeap, false, &middle, &_upperbound, stats );
            }
        }
        delete _forwardHeap;
        delete _backwardHeap;
        return _upperbound;
    }

    inline unsigned int findNearestNodeForLatLon(const _Coordinate& coord, _Coordinate& result) const
    {
        nodeHelpDesk->findNearestNodeCoordForLatLon( coord, result );
        return 0;

    }

    inline bool FindRoutingStarts(const _Coordinate start, const _Coordinate target, PhantomNodes * routingStarts)
    {
        nodeHelpDesk->FindRoutingStarts(start, target, routingStarts);
        return true;
    }

    inline NodeID GetNameIDForOriginDestinationNodeID(NodeID s, NodeID t) const {
        assert(s!=t);
        EdgeID e = _graph->FindEdge( s, t );
        if(e == UINT_MAX)
            e = _graph->FindEdge( t, s );
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

    inline std::string GetEscapedNameForNameID(const NodeID nameID) const {
        return ( nameID >= _names->size() ? std::string("") : HTMLEntitize(_names->at(nameID)) );
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

    inline void RegisterThread(const unsigned k, const unsigned v) {
        nodeHelpDesk->RegisterThread(k,v);
    }
private:

    inline void _RoutingStep(_Heap * _forwardHeap, _Heap *_backwardHeap, const bool& forwardDirection, NodeID * middle, unsigned int * _upperbound) {
        const NodeID node = _forwardHeap->DeleteMin();
        const unsigned int distance = _forwardHeap->GetKey( node );
        if ( _backwardHeap->WasInserted( node ) ) {
            const unsigned int newDistance = _backwardHeap->GetKey( node ) + distance;
            if ( newDistance < *_upperbound ) {
                *middle = node;
                *_upperbound = newDistance;
            }
        }
        if ( distance > *_upperbound ) {
            _forwardHeap->DeleteAll();
            return;
        }


        for ( typename GraphT::EdgeIterator edge = _graph->BeginEdges( node ); edge < _graph->EndEdges(node); edge++ ) {
            const EdgeData& ed = _graph->GetEdgeData(edge);
            //            if(!ed.shortcut)
            //                continue;
            const NodeID to = _graph->GetTarget(edge);
            const EdgeWeight edgeWeight = ed.distance;

            assert( edgeWeight > 0 );
            const int toDistance = distance + edgeWeight;

            //Stalling
            if(_forwardHeap->WasInserted( to )) {
                if(!forwardDirection ? ed.forward : ed.backward) {
                    if(_forwardHeap->GetKey( to ) + edgeWeight < distance) {
                        //                        std::cout << "[stalled] node " << node << std::endl;
                        return;
                    }
                }
            }

            if(forwardDirection ? ed.forward : ed.backward ) {
                //New Node discovered -> Add to Heap + Node Info Storage
                if ( !_forwardHeap->WasInserted( to ) ) {
                    _forwardHeap->Insert( to, toDistance, node );
                }
                //Found a shorter Path -> Update distance
                else if ( toDistance < _forwardHeap->GetKey( to ) ) {
                    _forwardHeap->GetData( to ).parent = node;
                    _forwardHeap->DecreaseKey( to, toDistance );
                    //new parent
                }
            }
        }
    }

    inline void _RoutingStepWithStats( _Heap * _forwardHeap, _Heap *_backwardHeap, const bool& forwardDirection, NodeID * middle, unsigned int * _upperbound, _Statistics& stats) {
        const NodeID node = _forwardHeap->DeleteMin();
        stats.deleteMins++;
        const unsigned int distance = _forwardHeap->GetKey( node );
        if ( _backwardHeap->WasInserted( node ) ) {
            const unsigned int newDistance = _backwardHeap->GetKey( node ) + distance;
            if ( newDistance < *_upperbound ) {
                *middle = node;
                *_upperbound = newDistance;
            }
        }
        if ( distance > *_upperbound ) {
            stats.meetingNodes++;
            _forwardHeap->DeleteAll();
            return;
        }


        for ( typename GraphT::EdgeIterator edge = _graph->BeginEdges( node ); edge < _graph->EndEdges(node); edge++ ) {
            const EdgeData& ed = _graph->GetEdgeData(edge);
            const NodeID to = _graph->GetTarget(edge);
            const EdgeWeight edgeWeight = ed.distance;

            assert( edgeWeight > 0 );
            const int toDistance = distance + edgeWeight;

            //Stalling
            if(_forwardHeap->WasInserted( to )) {
                if(!forwardDirection ? ed.forward : ed.backward) {
                    if(_forwardHeap->GetKey( to ) + edgeWeight < distance) {
                        stats.stalledNodes++;
                        return;
                    }
                }
            }

            if(forwardDirection ? ed.forward : ed.backward ) {
                //New Node discovered -> Add to Heap + Node Info Storage
                if ( !_forwardHeap->WasInserted( to ) ) {
                    _forwardHeap->Insert( to, toDistance, node );
                    stats.insertedNodes++;
                }
                //Found a shorter Path -> Update distance
                else if ( toDistance < _forwardHeap->GetKey( to ) ) {
                    _forwardHeap->GetData( to ).parent = node;
                    _forwardHeap->DecreaseKey( to, toDistance );
                    stats.decreasedNodes++;
                    //new parent
                }
            }
        }
    }

    bool _UnpackEdge( const NodeID source, const NodeID target, std::vector< _PathData >* path ) {
        assert(source != target);
        //find edge first.
        bool forward = true;
        typename GraphT::EdgeIterator smallestEdge = SPECIAL_EDGEID;
        EdgeWeight smallestWeight = UINT_MAX;
        for(typename GraphT::EdgeIterator eit = _graph->BeginEdges(source); eit < _graph->EndEdges(source); eit++)
        {
            const EdgeWeight weight = _graph->GetEdgeData(eit).distance;
            {
                if(_graph->GetTarget(eit) == target && weight < smallestWeight && _graph->GetEdgeData(eit).forward)
                {
                    smallestEdge = eit; smallestWeight = weight;
                }
            }
        }
        if(smallestEdge == SPECIAL_EDGEID)
        {
            for(typename GraphT::EdgeIterator eit = _graph->BeginEdges(target); eit < _graph->EndEdges(target); eit++)
            {
                const EdgeWeight weight = _graph->GetEdgeData(eit).distance;
                {
                    if(_graph->GetTarget(eit) == source && weight < smallestWeight && _graph->GetEdgeData(eit).backward)
                    {
                        smallestEdge = eit; smallestWeight = weight;
                        forward = false;
                    }
                }
            }
        }

        assert(smallestWeight != SPECIAL_EDGEID); //no edge found. This should not happen at all!

        const EdgeData& ed = _graph->GetEdgeData(smallestEdge);
        if(ed.shortcut)
        {//unpack
            const NodeID middle = ed.middleName.middle;
            _UnpackEdge(source, middle, path);
            _UnpackEdge(middle, target, path);
            return false;
        } else {
            assert(!ed.shortcut);
            path->push_back(_PathData(target) );
            return true;
        }
    }
};

#endif /* SEARCHENGINE_H_ */
