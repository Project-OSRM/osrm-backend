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

#include "SearchEngine.h"

SearchEngine::SearchEngine(QueryGraph * g, NodeInformationHelpDesk * nh, std::vector<std::string> & n) :
        _queryData(g, nh, n),
        shortestPath(_queryData),
        alternativePaths(_queryData)
    {}
    SearchEngine::~SearchEngine() {}

void SearchEngine::GetCoordinatesForNodeID(NodeID id, _Coordinate& result) const {
    result.lat = _queryData.nodeHelpDesk->getLatitudeOfNode(id);
    result.lon = _queryData.nodeHelpDesk->getLongitudeOfNode(id);
}

void SearchEngine::FindPhantomNodeForCoordinate(const _Coordinate & location, PhantomNode & result, unsigned zoomLevel) const {
    _queryData.nodeHelpDesk->FindPhantomNodeForCoordinate(location, result, zoomLevel);
}

NodeID SearchEngine::GetNameIDForOriginDestinationNodeID(const NodeID s, const NodeID t) const {
    if(s == t){
        return 0;
    }
    EdgeID e = _queryData.graph->FindEdge(s, t);
    if(e == UINT_MAX) {
        e = _queryData.graph->FindEdge( t, s );
    }
    if(UINT_MAX == e) {
        return 0;
    }
    assert(e != UINT_MAX);
    const QueryEdge::EdgeData ed = _queryData.graph->GetEdgeData(e);
    return ed.id;
}

std::string SearchEngine::GetEscapedNameForNameID(const unsigned nameID) const {
    return ((nameID >= _queryData.names.size() || nameID == 0) ? std::string("") : HTMLEntitize(_queryData.names.at(nameID)));
}

SearchEngineHeapPtr SearchEngineData::forwardHeap;
SearchEngineHeapPtr SearchEngineData::backwardHeap;

SearchEngineHeapPtr SearchEngineData::forwardHeap2;
SearchEngineHeapPtr SearchEngineData::backwardHeap2;

SearchEngineHeapPtr SearchEngineData::forwardHeap3;
SearchEngineHeapPtr SearchEngineData::backwardHeap3;

 