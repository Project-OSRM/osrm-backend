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
