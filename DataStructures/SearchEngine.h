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
#include "SimpleStack.h"

#include <boost/thread.hpp>

#include "BinaryHeap.h"
#include "NodeInformationHelpDesk.h"
#include "PhantomNodes.h"
#include "../RoutingAlgorithms/AlternativePathRouting.h"
#include "../RoutingAlgorithms/BasicRoutingInterface.h"
#include "../RoutingAlgorithms/ShortestPathRouting.h"

#include "../Util/StringUtil.h"
#include "../typedefs.h"

struct _HeapData {
	NodeID parent;
	_HeapData( NodeID p ) : parent(p) { }
};

typedef boost::thread_specific_ptr<BinaryHeap< NodeID, NodeID, int, _HeapData, UnorderedMapStorage<NodeID, int> > > SearchEngineHeapPtr;

template<class EdgeData, class GraphT>
struct SearchEngineData {
    typedef SearchEngineHeapPtr HeapPtr;
    typedef GraphT Graph;
    SearchEngineData(GraphT * g, NodeInformationHelpDesk * nh, std::vector<string> & n) :graph(g), nodeHelpDesk(nh), names(n) {}
    const GraphT * graph;
    NodeInformationHelpDesk * nodeHelpDesk;
    std::vector<string> & names;
    static HeapPtr forwardHeap;
    static HeapPtr backwardHeap;
    static HeapPtr forwardHeap2;
    static HeapPtr backwardHeap2;
    static HeapPtr forwardHeap3;
    static HeapPtr backwardHeap3;

    inline void InitializeOrClearFirstThreadLocalStorage() {
        if(!forwardHeap.get()) {
            forwardHeap.reset(new BinaryHeap< NodeID, NodeID, int, _HeapData, UnorderedMapStorage<NodeID, int> >(nodeHelpDesk->getNumberOfNodes()));
        }
        else
            forwardHeap->Clear();

        if(!backwardHeap.get()) {
            backwardHeap.reset(new BinaryHeap< NodeID, NodeID, int, _HeapData, UnorderedMapStorage<NodeID, int> >(nodeHelpDesk->getNumberOfNodes()));
        }
        else
            backwardHeap->Clear();
    }

    inline void InitializeOrClearSecondThreadLocalStorage() {
        if(!forwardHeap2.get()) {
            forwardHeap2.reset(new BinaryHeap< NodeID, NodeID, int, _HeapData, UnorderedMapStorage<NodeID, int> >(nodeHelpDesk->getNumberOfNodes()));
        }
        else
            forwardHeap2->Clear();

        if(!backwardHeap2.get()) {
            backwardHeap2.reset(new BinaryHeap< NodeID, NodeID, int, _HeapData, UnorderedMapStorage<NodeID, int> >(nodeHelpDesk->getNumberOfNodes()));
        }
        else
            backwardHeap2->Clear();
    }

    inline void InitializeOrClearThirdThreadLocalStorage() {
        if(!forwardHeap3.get()) {
            forwardHeap3.reset(new BinaryHeap< NodeID, NodeID, int, _HeapData, UnorderedMapStorage<NodeID, int> >(nodeHelpDesk->getNumberOfNodes()));
        }
        else
            forwardHeap3->Clear();

        if(!backwardHeap3.get()) {
            backwardHeap3.reset(new BinaryHeap< NodeID, NodeID, int, _HeapData, UnorderedMapStorage<NodeID, int> >(nodeHelpDesk->getNumberOfNodes()));
        }
        else
            backwardHeap3->Clear();
    }
};

template<class EdgeData, class GraphT>
class SearchEngine {
private:
    typedef SearchEngineData<EdgeData, GraphT> SearchEngineDataT;
    SearchEngineDataT _queryData;

	inline double absDouble(double input) { if(input < 0) return input*(-1); else return input;}
public:
    ShortestPathRouting<SearchEngineDataT> shortestPath;
    AlternativeRouting<SearchEngineDataT> alternativePaths;

    SearchEngine(GraphT * g, NodeInformationHelpDesk * nh, std::vector<string> & n) :
	    _queryData(g, nh, n),
	    shortestPath(_queryData),
	    alternativePaths(_queryData)
	{}
	~SearchEngine() {}

	inline const void GetCoordinatesForNodeID(NodeID id, _Coordinate& result) const {
		result.lat = _queryData.nodeHelpDesk->getLatitudeOfNode(id);
		result.lon = _queryData.nodeHelpDesk->getLongitudeOfNode(id);
	}

	inline void FindRoutingStarts(const _Coordinate & start, const _Coordinate & target, PhantomNodes & routingStarts) const {
	    _queryData.nodeHelpDesk->FindRoutingStarts(start, target, routingStarts);
	}

	inline void FindPhantomNodeForCoordinate(const _Coordinate & location, PhantomNode & result) const {
	    _queryData.nodeHelpDesk->FindPhantomNodeForCoordinate(location, result);
	}

	inline NodeID GetNameIDForOriginDestinationNodeID(const NodeID s, const NodeID t) const {
		if(s == t)
			return 0;

		EdgeID e = _queryData.graph->FindEdge(s, t);
		if(e == UINT_MAX)
			e = _queryData.graph->FindEdge( t, s );
		if(UINT_MAX == e) {
			return 0;
		}
		assert(e != UINT_MAX);
		const EdgeData ed = _queryData.graph->GetEdgeData(e);
		return ed.via;
	}

	inline std::string GetEscapedNameForNameID(const unsigned nameID) const {
	    return ((nameID >= _queryData.names.size() || nameID == 0) ? std::string("") : HTMLEntitize(_queryData.names.at(nameID)));
	}

	inline std::string GetEscapedNameForEdgeBasedEdgeID(const unsigned edgeID) const {
		const unsigned nameID = _queryData.graph->GetEdgeData(edgeID).nameID1;
		return GetEscapedNameForNameID(nameID);
	}

};

template<class EdgeData, class GraphT> SearchEngineHeapPtr SearchEngineData<EdgeData, GraphT>::forwardHeap;
template<class EdgeData, class GraphT> SearchEngineHeapPtr SearchEngineData<EdgeData, GraphT>::backwardHeap;

template<class EdgeData, class GraphT> SearchEngineHeapPtr SearchEngineData<EdgeData, GraphT>::forwardHeap2;
template<class EdgeData, class GraphT> SearchEngineHeapPtr SearchEngineData<EdgeData, GraphT>::backwardHeap2;

template<class EdgeData, class GraphT> SearchEngineHeapPtr SearchEngineData<EdgeData, GraphT>::forwardHeap3;
template<class EdgeData, class GraphT> SearchEngineHeapPtr SearchEngineData<EdgeData, GraphT>::backwardHeap3;

#endif /* SEARCHENGINE_H_ */
