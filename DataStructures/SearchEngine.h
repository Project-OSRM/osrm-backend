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

#ifndef SEARCHENGINE_H_
#define SEARCHENGINE_H_

#include "Coordinate.h"
#include "NodeInformationHelpDesk.h"
#include "PhantomNodes.h"
#include "QueryEdge.h"
#include "SearchEngineData.h"
#include "../RoutingAlgorithms/AlternativePathRouting.h"
#include "../RoutingAlgorithms/ShortestPathRouting.h"
#include "../Server/DataStructures/QueryObjectsStorage.h"

#include "../Util/StringUtil.h"
#include "../typedefs.h"

#include <climits>
#include <string>
#include <vector>

class SearchEngine {
private:
    SearchEngineData _queryData;

public:
    ShortestPathRouting<SearchEngineData> shortestPath;
    AlternativeRouting<SearchEngineData> alternativePaths;

    SearchEngine( QueryObjectsStorage * query_objects );
	~SearchEngine();

	void GetCoordinatesForNodeID(NodeID id, FixedPointCoordinate& result) const;

    void FindPhantomNodeForCoordinate(
        const FixedPointCoordinate & location,
        PhantomNode & result,
        unsigned zoomLevel
    ) const;

    NodeID GetNameIDForOriginDestinationNodeID(
        const NodeID s, const NodeID t) const;

    std::string GetEscapedNameForNameID(const unsigned nameID) const;
};

#endif /* SEARCHENGINE_H_ */
