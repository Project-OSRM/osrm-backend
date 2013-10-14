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
