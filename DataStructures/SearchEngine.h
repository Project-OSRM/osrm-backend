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

#ifndef SEARCHENGINE_H
#define SEARCHENGINE_H

#include "Coordinate.h"
#include "SearchEngineData.h"
#include "PhantomNodes.h"
#include "QueryEdge.h"
#include "../RoutingAlgorithms/AlternativePathRouting.h"
#include "../RoutingAlgorithms/ShortestPathRouting.h"

#include "../Util/StringUtil.h"
#include "../typedefs.h"

#include <boost/assert.hpp>

#include <climits>
#include <string>
#include <vector>

template<class DataFacadeT>
class SearchEngine {
private:
    DataFacadeT * facade;
    SearchEngineData engine_working_data;
public:
    ShortestPathRouting<DataFacadeT> shortest_path;
    AlternativeRouting <DataFacadeT> alternative_path;

    SearchEngine( DataFacadeT * facade )
     :
        facade             (facade),
        shortest_path      (facade, engine_working_data),
        alternative_path   (facade, engine_working_data)
    {}

    ~SearchEngine() {}

};

#endif // SEARCHENGINE_H
