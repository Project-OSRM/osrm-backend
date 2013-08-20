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

SearchEngine::SearchEngine( QueryObjectsStorage * query_objects ) :
    _queryData(query_objects),
    shortestPath(_queryData),
    alternativePaths(_queryData)
{}

SearchEngine::~SearchEngine() {}

void SearchEngine::GetCoordinatesForNodeID(
    NodeID id,
    FixedPointCoordinate& result
) const {
        result = _queryData.nodeHelpDesk->GetCoordinateOfNode(id);
}

void SearchEngine::FindPhantomNodeForCoordinate(
    const FixedPointCoordinate & location,
    PhantomNode & result,
    const unsigned zoomLevel
) const {
    _queryData.nodeHelpDesk->FindPhantomNodeForCoordinate(
        location,
        result, zoomLevel
    );
}

NodeID SearchEngine::GetNameIDForOriginDestinationNodeID(
    const NodeID s,
    const NodeID t
) const {
    if(s == t) {
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
    std::string result;
    _queryData.query_objects->GetName(nameID, result);
    return HTMLEntitize(result);
}

SearchEngineHeapPtr SearchEngineData::forwardHeap;
SearchEngineHeapPtr SearchEngineData::backwardHeap;

SearchEngineHeapPtr SearchEngineData::forwardHeap2;
SearchEngineHeapPtr SearchEngineData::backwardHeap2;

SearchEngineHeapPtr SearchEngineData::forwardHeap3;
SearchEngineHeapPtr SearchEngineData::backwardHeap3;
